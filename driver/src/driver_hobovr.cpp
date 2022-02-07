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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <variant>

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

// devices
#include "hobovr_defines.h"

#include "hobovr_device_base.h"
#include "hobovr_components.h"

#include "headsets.h"
#include "controllers.h"
#include "trackers.h"
#include "addons.h"

#include "tracking_references.h"

#include "util.h"


//-----------------------------------------------------------------------------
// Purpose: serverDriver
//-----------------------------------------------------------------------------

enum EHobovrDeviceNodeTypes {
	hmd = 0,
	controller = 1,
	tracker = 2,
	gaze_master = 3
};


struct HobovrDeviceStorageNode_t {
	EHobovrDeviceNodeTypes type;
	void* handle;
};


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
	void SlowUpdateThread();
	static void SlowUpdateThreadEnter(CServerDriver_hobovr* ptr) {
		ptr->SlowUpdateThread();
	}

	void FastThread();
	static void FastThreadEnter(CServerDriver_hobovr* ptr) {
		ptr->FastThread();
	}

	void UpdateServerDeviceList();

	std::vector<HobovrDeviceStorageNode_t> m_vDevices;
	std::vector<HobovrDeviceStorageNode_t> m_vStandbyDevices;

	size_t m_luPacketSize = 16;
	PacketEndTag m_tag = {'\t', '\r', '\n'};
	std::shared_ptr<hobovr::tcp_socket> m_lscSocket;
	std::shared_ptr<hobovr::tcp_receiver_loop> m_pReceiver;
	std::shared_ptr<HobovrTrackingRef_SettManager> m_pSettManTref;

	bool m_bDeviceListSyncEvent = false;

	// slower thread stuff
	bool m_bThreadAlive;
	std::unique_ptr<std::thread> m_ptSlowUpdateThread;
	std::unique_ptr<std::thread> m_ptFastThread;
};

// yes
EVRInitError CServerDriver_hobovr::Init(vr::IVRDriverContext *pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());

	DriverLog("driver: version: %d.%d.%d %s \n",
		hobovr::k_nHobovrVersionMajor,
		hobovr::k_nHobovrVersionMinor,
		hobovr::k_nHobovrVersionBuild,
		hobovr::k_sHobovrVersionGG.c_str()
	);

	// time to init the connection to the poser :P
	m_lscSocket = std::make_shared<hobovr::tcp_socket>();

	int res = m_lscSocket->Connect("127.0.0.1", 6969);
	if (res) {
		DriverLog("driver: failed to connect: errno=%d", lerrno);
		return VRInitError_IPC_ServerInitFailed;
	}

	// send an id message saying this is a tracking socket
	m_lscSocket->Send(KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage));

	m_pReceiver = std::make_unique<hobovr::tcp_receiver_loop>(
		m_lscSocket,
		m_tag,
		std::bind(&CServerDriver_hobovr::OnPacket, this, std::placeholders::_1, std::placeholders::_2),
		16
	);
	m_pReceiver->Start();

	// misc slow and fast threads
	m_bThreadAlive = true;
	m_ptSlowUpdateThread = std::make_unique<std::thread>(this->SlowUpdateThreadEnter, this);
	m_ptFastThread = std::make_unique<std::thread>(this->FastThreadEnter, this);

	return VRInitError_None;
}

void CServerDriver_hobovr::OnPacket(void* buff, size_t len) {

	if (len != m_luPacketSize) {
		// tell the poser it fucked up
		// TODO: make different responses for different fuck ups...
		// 							and detect different fuck ups
		HoboVR_RespBufSize_t expected_size = {(uint32_t)m_luPacketSize};

		HoboVR_PoserResp_t resp{
			EPoserRespType_badDeviceList,
			(HoboVR_RespData_t&)expected_size,
			m_tag
		};

		m_lscSocket->Send(
			&resp,
			sizeof(resp)
		);
		// GOD FUCKING FINALLY

		// so logs in steamvr take ages to complete... TOO BAD!
		DriverLog("driver: posers are getting ignored~ expected %d bytes, got %d bytes~\n",
			(int)m_luPacketSize,
			(int)len
		);

		return; // do nothing on bad messages
	}

	if (m_bDeviceListSyncEvent)
		return; // sync in progress, do nothing


	size_t buff_offset = 0;

	// need that + op no cap
	char* buff2 = (char*)buff;

	for (size_t i=0; i < m_vDevices.size(); i++){
		switch (m_vDevices[i].type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)m_vDevices[i].handle;
				device->UpdateState(buff2 + buff_offset);

				buff_offset += device->GetPacketSize();
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)m_vDevices[i].handle;
				device->UpdateState(buff2 + buff_offset);

				buff_offset += device->GetPacketSize();
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)m_vDevices[i].handle;
				device->UpdateState(buff2 + buff_offset);

				buff_offset += device->GetPacketSize();
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)m_vDevices[i].handle;
				device->UpdateState(buff2 + buff_offset);

				buff_offset += device->GetPacketSize();
				break;
			}
		}
	}

	// DriverLog("got data: %f %f %f %d", meh[0], meh[1], meh[2], len);


}

void CServerDriver_hobovr::Cleanup() {
	DriverLog("driver cleanup called");
	// set slow/fast thread alive signals to exit
	m_bThreadAlive = false;

	// send a "driver exit" notification to the poser
	HoboVR_RespReserved_t fake_data = {};
	HoboVR_PoserResp_t resp{
		EPoserRespType_driverShutdown,
		(HoboVR_RespData_t&)fake_data,
		m_tag
	};
	m_lscSocket->Send(&resp, sizeof(resp));

	// release ownership to signal the receiver thread to finish
	m_lscSocket.reset();

	m_ptSlowUpdateThread->join();
	m_ptFastThread->join();

	// call stop to make sure the receiver thread has joined
	m_pReceiver->Stop();

	for (auto& i : m_vDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
				free(device);
				break;
			}
		}
	}

	for (auto& i : m_vStandbyDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
				free(device);
				break;
			}
		}
	}

	m_vDevices.clear();
	m_vStandbyDevices.clear();

	CleanupDriverLog();
	VR_CLEANUP_SERVER_DRIVER_CONTEXT();
}

void CServerDriver_hobovr::FastThread() {
	DriverLog("FastThread started");

	// settings manager, yes its created by the fast thread now, TOO BAD!
	m_pSettManTref = std::make_shared<HobovrTrackingRef_SettManager>("trsm0");
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pSettManTref->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_TrackingReference,
		m_pSettManTref.get()
	);

	while (m_bThreadAlive) {
		// check of incoming udu events
		if (m_pSettManTref->GetUduEvent()) {
			DriverLog("udu change event");
			m_luPacketSize = util::udu2sizet(
				m_pSettManTref->GetUduBuffer()
			);		

			m_bDeviceListSyncEvent = true;
			m_pReceiver->ReallocInternalBuffer(m_luPacketSize);
			UpdateServerDeviceList();
			m_bDeviceListSyncEvent = false;
		}

		vr::VREvent_t vrEvent;
		while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {

			for (auto& i : m_vDevices) {
				switch (i.type) {
					case EHobovrDeviceNodeTypes::hmd: {
						HeadsetDriver* device = (HeadsetDriver*)i.handle;
						device->ProcessEvent(vrEvent);
						break;
					}

					case EHobovrDeviceNodeTypes::controller: {
						ControllerDriver* device = (ControllerDriver*)i.handle;
						device->ProcessEvent(vrEvent);
						break;
					}

					case EHobovrDeviceNodeTypes::tracker: {
						TrackerDriver* device = (TrackerDriver*)i.handle;
						device->ProcessEvent(vrEvent);
						break;
					}

					case EHobovrDeviceNodeTypes::gaze_master: {
						GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
						device->ProcessEvent(vrEvent);
						break;
					}
				}
			}

		}

		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	}
	DriverLog("FastThread ended");
}

// Updates currently running devices
// TODO: do that without powering off all devices first...
void CServerDriver_hobovr::UpdateServerDeviceList() {
	for (auto i : m_vDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				device->PowerOff();
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				device->PowerOff();
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				device->PowerOff();
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
				device->PowerOff();
				break;
			}
		}
		m_vStandbyDevices.push_back(i);
	}

	m_vDevices.clear();

	auto uduBufferCopy = m_pSettManTref->GetUduBuffer();

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int gaze_master_counter = 0;

	int controller_hs = 1;

	for (char i : uduBufferCopy) {
		if (i == 'h') {
			auto target = "h" + std::to_string(counter_hmd);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::gaze_master:
						return ((GazeMasterDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((HeadsetDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				HeadsetDriver* temp = new HeadsetDriver(
					"h" + std::to_string(counter_hmd)
				);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_HMD,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::hmd, temp});
			}

			counter_hmd++;
		} else if (i == 'c') {
			auto target = "c" + std::to_string(counter_cntrlr);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::gaze_master:
						return ((GazeMasterDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((ControllerDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				ControllerDriver* temp = new ControllerDriver(
					controller_hs,
					"c" + std::to_string(counter_cntrlr),
					m_lscSocket
				);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_Controller,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::controller, temp});

			}

			controller_hs = (controller_hs) ? 0 : 1;
			counter_cntrlr++;

		} else if (i == 't') {
			auto target = "t" + std::to_string(counter_trkr);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::gaze_master:
						return ((GazeMasterDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((TrackerDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			} else {
				TrackerDriver* temp = new TrackerDriver(
					"t" + std::to_string(counter_trkr),
					m_lscSocket
				);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_GenericTracker,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::tracker, temp});

			}

			counter_trkr++;
		} else if (i == 'g') {
			auto target = "g" + std::to_string(gaze_master_counter);
			auto key = [target](HobovrDeviceStorageNode_t d)->bool {
				switch (d.type) {
					case EHobovrDeviceNodeTypes::hmd:
						return ((HeadsetDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::controller:
						return ((ControllerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::tracker:
						return ((TrackerDriver*)d.handle)->GetSerialNumber() == target;

					case EHobovrDeviceNodeTypes::gaze_master:
						return ((GazeMasterDriver*)d.handle)->GetSerialNumber() == target;
				}
				return false;
			};

			auto res = std::find_if(m_vStandbyDevices.begin(), m_vStandbyDevices.end(), key);
			if (res != m_vStandbyDevices.end()) {
				((GazeMasterDriver*)((*res).handle))->PowerOn();
				m_vDevices.push_back(*res);
				m_vStandbyDevices.erase(res);
			}

			else {
				GazeMasterDriver* temp = new GazeMasterDriver("g" + std::to_string(gaze_master_counter));

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_GenericTracker,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::gaze_master, temp});

			}
		}
	}

}

void CServerDriver_hobovr::SlowUpdateThread() {
	DriverLog("driver: slow update thread started\n");

	while (m_bThreadAlive){
		for (auto &i : m_vDevices){
			switch (i.type) {
				case EHobovrDeviceNodeTypes::hmd: {
					HeadsetDriver* device = (HeadsetDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}

				case EHobovrDeviceNodeTypes::controller: {
					ControllerDriver* device = (ControllerDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}

				case EHobovrDeviceNodeTypes::tracker: {
					TrackerDriver* device = (TrackerDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}

				case EHobovrDeviceNodeTypes::gaze_master: {
					GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
					device->UpdateDeviceBatteryCharge();
					device->CheckForUpdates();
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));

		m_pSettManTref->UpdatePose();

	}
	DriverLog("driver: slow update thread closed\n");

}

CServerDriver_hobovr g_hobovrServerDriver;

//-----------------------------------------------------------------------------
// Purpose: driverFactory
//-----------------------------------------------------------------------------
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
