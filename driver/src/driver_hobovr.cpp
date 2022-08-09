// SPDX-License-Identifier: GPL-2.0-only

// driver_hobovr.cpp - main driver source file

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>


//#include "openvr.h"
#include <openvr_driver.h>
//#include "openvr_capi.h"
#include "driverlog.h"

#include <lazy_sockets.h>

#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <unordered_map>

using namespace vr;

#if defined(_WINDOWS)
	#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
	#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)
#elif defined(LINUX)
	#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
	#define HMD_DLL_IMPORT extern "C"
#else
	#error "Unsupported Platform."
#endif

// timer
#include "timer.h"

// devices
#include "device_factory.h"

#include "hobovr_defines.h"

#include "hobovr_device_base.h"
#include "hobovr_components.h"

#include "tracking_references.h"

#include "util.h"


///////////////////////////////////////////////////////////////////////////////
// Purpose: serverDriver
///////////////////////////////////////////////////////////////////////////////

class CServerDriver_hobovr : public IServerTrackedDeviceProvider {
public:
	CServerDriver_hobovr() {}
	virtual EVRInitError Init(vr::IVRDriverContext *pDriverContext);
	virtual void Cleanup();
	virtual const char *const *GetInterfaceVersions() {
		return vr::k_InterfaceVersions;
	}

	virtual bool ShouldBlockStandbyMode() { return false; }

	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
	virtual void RunFrame() {}
	void OnPacket(void* buff, size_t len);

private:
	// void InternalThread();
	void UpdateServerDeviceList();

	std::vector<std::unique_ptr<hobovr::IHobovrDevice>> mvDevices;
	std::vector<std::unique_ptr<hobovr::IHobovrDevice>> mvOffDevices;

	size_t muInternalBufferSize = 16;

	std::shared_ptr<hobovr::tcp_socket> mlSocket;
	std::unique_ptr<hobovr::tcp_receiver_loop> mpReceiver;
	std::unique_ptr<hobovr::Timer> mpTimer;

	std::unique_ptr<HobovrTrackingRef_SettManager> mpSettingsManager;

	std::atomic<bool> mbUduEvent;
};

///////////////////////////////////////////////////////////////////////////////
// yes
///////////////////////////////////////////////////////////////////////////////

EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("driver: version: %d.%d.%d %s \n",
		hobovr::k_nHobovrVersionMajor,
		hobovr::k_nHobovrVersionMinor,
		hobovr::k_nHobovrVersionBuild,
		hobovr::k_sHobovrVersionGG.c_str()
	);

	// ensure we can activate our hmd later on
	bool need_restart = vr::VRSettings()->GetBool("steamvr", "requireHmd");
	if (need_restart) {
		vr::VRSettings()->SetBool(
			k_pch_Robotic_Persistence_Section,
			k_pch_Robotic_recover_requireHMD_Bool,
			true
		);
		DriverLog("driver: 'requireHmd' is enabled, need a restart...\n");
		vr::VRSettings()->SetBool("steamvr", "requireHmd", false);
		return VRInitError_Init_Retry;
	}

	// time to init the connection to the poser :P
	mlSocket = std::make_shared<hobovr::tcp_socket>();

	int res = mlSocket->Connect("127.0.0.1", 6969);
	if (res) {
		DriverLog("driver: failed to connect: errno=%s", lsc::getString(lerrno).c_str());
		return VRInitError_IPC_ServerInitFailed;
	}

	DriverLog("driver: tracking socket fd=%d", (int)mlSocket->GetHandle());

	// send an id message saying this is a tracking socket
	res = mlSocket->Send(KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage));
	if (res < 0) {
        DriverLog("driver: failed to send id message: errno=%d\n", lerrno);
        return vr::VRInitError_IPC_ConnectFailed;
    }

	mpReceiver = std::make_unique<hobovr::tcp_receiver_loop>(
		mlSocket,
		g_EndTag,
		std::bind(&CServerDriver_hobovr::OnPacket, this, std::placeholders::_1, std::placeholders::_2),
		16
	);
	mpReceiver->Start();

	// create manager now
	mpSettingsManager = std::make_unique<HobovrTrackingRef_SettManager>("trsm0");
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		mpSettingsManager->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_TrackingReference,
		mpSettingsManager.get()
	);

	// misc start timer
	mpTimer = std::make_unique<hobovr::Timer>();
	// and add some timed events
	// equivalent of the old slow thread
	mpTimer->registerTimer(
		std::chrono::seconds(1),
		[this]() {
			for (auto &i : mvDevices){
				i->UpdateDeviceBatteryCharge();
				i->CheckForUpdates();
			}
			mpSettingsManager->UpdatePose();
		}
	);
	// equivalent of the old fast thread
	mpTimer->registerTimer(
		std::chrono::milliseconds(1),
		[this]() {
			// fast part
			// check of incoming udu events
			if (mpSettingsManager->GetUduEvent() && !mbUduEvent.load()) {
				DriverLog("udu change event");
				muInternalBufferSize = util::udu2sizet(
					mpSettingsManager->GetUduBuffer()
				);		

				// pause packet processing
				mbUduEvent.store(true);
				UpdateServerDeviceList();
				mpReceiver->ReallocInternalBuffer(muInternalBufferSize);
				// resume packet processing
				mbUduEvent.store(false);
				DriverLog("udu change event end");
			}

			vr::VREvent_t vrEvent;
			while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
				for (auto& i : mvDevices) {
					i->ProcessEvent(vrEvent);
				}

			}
		}
	);

	return VRInitError_None;
}

///////////////////////////////////////////////////////////////////////////////

void CServerDriver_hobovr::OnPacket(void* buff, size_t len) {
	if (mbUduEvent.load())
		return; // sync in progress, do nothing

	if (len != muInternalBufferSize) {
		// tell the poser it fucked up
		// TODO: make different responses for different fuck ups...
		// 							and detect different fuck ups
		HoboVR_RespBufSize_t expected_size = {(uint32_t)muInternalBufferSize};

		HoboVR_PoserResp_t resp{
			EPoserRespType_badDeviceList,
			(HoboVR_RespData_t&)expected_size,
			g_EndTag
		};

		mlSocket->Send(
			&resp,
			sizeof(resp)
		);
		// GOD FUCKING FINALLY

		// so logs in steamvr take ages to complete... TOO BAD!
		DriverLog("driver: posers are getting ignored~ expected %d bytes, got %d bytes~\n",
			(int)muInternalBufferSize,
			(int)len
		);

		return; // do nothing on bad messages
	}


	size_t buff_offset = 0;

	// need that + op no cap
	char* buff2 = (char*)buff;

	for (auto& i : mvDevices){
		i->UpdateState(buff2 + buff_offset);
		buff_offset += i->GetPacketSize();
	}

	DebugDriverLog("got data: %d %p '%c%c%c%c'", len, buff2, buff2[0], buff2[1], buff2[2], buff2[3]);


}

///////////////////////////////////////////////////////////////////////////////

void CServerDriver_hobovr::Cleanup() {
	DriverLog("driver cleanup called");
	// set slow/fast thread alive signals to exit
	mpTimer.reset();

	// send a "driver exit" notification to the poser
	HoboVR_RespReserved_t fake_data = {};
	HoboVR_PoserResp_t resp{
		EPoserRespType_driverShutdown,
		(HoboVR_RespData_t&)fake_data,
		g_EndTag
	};
	mlSocket->Send(&resp, sizeof(resp));

	// release ownership to signal the receiver thread to finish
	mlSocket.reset();

	// call stop to make sure the receiver thread has joined
	mpReceiver->Stop();

	mvDevices.clear();
	mvOffDevices.clear();


	if (
		vr::VRSettings()->GetBool(
			k_pch_Robotic_Persistence_Section,
			k_pch_Robotic_recover_requireHMD_Bool
		)
	) {
		vr::VRSettings()->SetBool(
			k_pch_Robotic_Persistence_Section,
			k_pch_Robotic_recover_requireHMD_Bool,
			false
		);
		vr::VRSettings()->SetBool("steamvr", "requireHmd", true);
		DriverLog("driver: recovered requireHmd...\n");
	}

	CleanupDriverLog();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

///////////////////////////////////////////////////////////////////////////////
// Updates currently running devices
///////////////////////////////////////////////////////////////////////////////

void CServerDriver_hobovr::UpdateServerDeviceList() {

	// temporarily move all devices into the off list
	std::move(mvDevices.begin(), mvDevices.end(), std::back_inserter(mvOffDevices));

	mvDevices.clear();

	// assemble a list of new active devices
	std::vector<hobovr::IHobovrDevice> tmp;

	std::unordered_map<char, int> counters;

	// fill map with zeros for appropriate keys
	for (char i : "htcg")
		counters[i] = 0;

	for (char i : mpSettingsManager->GetUduBuffer()) {
		std::string target_serial = std::string(&i, 1) + std::to_string(counters[i]);
		DriverLog("%s: looking for '%s'", __FUNCTION__, target_serial.c_str());
		// first try to find an old device that had this serial
		auto res = std::find_if(
			mvOffDevices.begin(),
			mvOffDevices.end(),
			[&target_serial](const std::unique_ptr<hobovr::IHobovrDevice>& val) {
				bool res = target_serial == val->GetSerialNumber();
				DebugDriverLog(
					"\t'%s' == '%s' => %d",
					target_serial.c_str(),
					val->GetSerialNumber().c_str(),
					(int)(res)
				);
				return res;
			}
		);

		// old device found, no need to create a new one
		if (res != mvOffDevices.end()) {
			DriverLog("%s: '%s' found, moving to the next...", __FUNCTION__, target_serial.c_str());
			auto device = std::move(*res);

			// push device to the on list
			mvDevices.push_back(std::move(device));

			// remove device from the off list
			mvOffDevices.erase(res);

			counters[i]++;
			continue;
		}

		// DriverLog("%s: %p == %p => %d", __FUNCTION__, res, mvOffDevices.end(), res != mvOffDevices.end());
		DriverLog("%s: '%s' not found...", __FUNCTION__, target_serial.c_str());

		// couldn't find an old device, time to make a new one
		std::unique_ptr<hobovr::IHobovrDevice> new_device = hobovr::createDeviceFactory(
			i,
			counters[i],
			mlSocket
		);

		if (!new_device) {
			DriverLog("%s: failed to create device '%s', trying again...", __FUNCTION__, target_serial.c_str());
			continue;
		}

		// push device to the on list
		mvDevices.push_back(std::move(new_device));

		DriverLog("%s: New device '%s': %p", __FUNCTION__, target_serial.c_str(), mvDevices.back().get());

		counters[i]++;
	}

	// power off unused devices
	for (auto& i : mvOffDevices)
		i->PowerOff();


}

CServerDriver_hobovr g_hobovrServerDriver;

///////////////////////////////////////////////////////////////////////////////
// Purpose: driverFactory
///////////////////////////////////////////////////////////////////////////////

HMD_DLL_EXPORT void *HmdDriverFactory(
	const char *pInterfaceName,
	int *pReturnCode
) {

	if (0 == strcmp(IServerTrackedDeviceProvider_Version, pInterfaceName)) {
		return &g_hobovrServerDriver;
	}

	if (pReturnCode)
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}
