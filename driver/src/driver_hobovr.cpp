// SPDX-License-Identifier: GPL-2.0-only

// driver_hobovr.cpp - main driver source file

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>


//#include "openvr.h"
#include <openvr_driver.h>
//#include "openvr_capi.h"
#include "driverlog.h"

#include <chrono>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include "ref/receiver_win.h"

#elif defined(__linux__)
#include "ref/receiver_linux.h"
#define _stricmp strcasecmp

#endif

#if defined(_WINDOWS)
#include <windows.h>
#endif



#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <variant>

using namespace vr;

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec(dllexport)
#define HMD_DLL_IMPORT extern "C" __declspec(dllimport)
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C"
#else
#error "Unsupported Platform."
#endif

// devices
#include "headsets.h"
#include "controllers.h"
#include "trackers.h"

#include "tracking_references.h"

#include "addons.h"

#include "hobovr_defines.h"

#include "hobovr_device_base.h"
#include "hobovr_components.h"

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


class CServerDriver_hobovr : public IServerTrackedDeviceProvider, public SockReceiver::Callback {
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
	virtual void RunFrame();
	void OnPacket(char* buff, int len);

private:
	void SlowUpdateThread();
	static void SlowUpdateThreadEnter(CServerDriver_hobovr *ptr) {
		ptr->SlowUpdateThread();
	}

	void UpdateServerDeviceList();

	std::vector<HobovrDeviceStorageNode_t> m_vDevices;
	std::vector<HobovrDeviceStorageNode_t> m_vStandbyDevices;

	std::shared_ptr<SockReceiver::DriverReceiver> m_pSocketComm;
	std::shared_ptr<HobovrTrackingRef_SettManager> m_pSettManTref;

	bool m_bDeviceListSyncEvent = false;


	// slower thread stuff
	bool m_bSlowUpdateThreadIsAlive;
	std::thread* m_ptSlowUpdateThread;
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

	std::string uduThing;
	char buf[1024];
	vr::VRSettings()->GetString(
		k_pch_Hobovr_Section,
		k_pch_Hobovr_UduDeviceManifestList_String,
		buf,
		sizeof(buf)
	);
	uduThing = buf;
	DriverLog("driver: udu settings: '%s'\n", uduThing.c_str());

	// udu setting parse is done by SockReceiver
	try{
		m_pSocketComm = std::make_shared<SockReceiver::DriverReceiver>(uduThing);
		m_pSocketComm->start();

	} catch (...){
		DriverLog("m_pSocketComm broke on create or broke on start, either way you're fucked\n");
		DriverLog("check if the server is running...\n");
		DriverLog("... 10061 means \"couldn't connect to server\"...(^_^;)..\n");
		return VRInitError_Init_WebServerFailed;

	}

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int gaze_master_counter = 0;
	int controller_hs = 1;

	// add new devices based on the udu parse 
	for (std::string i:m_pSocketComm->m_vsDevice_list) {
		if (i == "h") {
			HeadsetDriver* temp = new HeadsetDriver(
				"h" + std::to_string(counter_hmd)
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
				temp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_HMD,
				temp
			);

			m_vDevices.push_back({EHobovrDeviceNodeTypes::hmd, temp});

		  counter_hmd++;

		} else if (i == "c") {
			ControllerDriver* temp = new ControllerDriver(
				controller_hs,
				"c" + std::to_string(counter_cntrlr),
				m_pSocketComm
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
						temp->GetSerialNumber().c_str(),
						vr::TrackedDeviceClass_Controller,
						temp);
			m_vDevices.push_back({EHobovrDeviceNodeTypes::controller, temp});

			controller_hs = (controller_hs) ? 0 : 1;
			counter_cntrlr++;

		} else if (i == "t") {
			TrackerDriver* temp = new TrackerDriver(
				"t" + std::to_string(counter_trkr),
				m_pSocketComm
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
				temp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_GenericTracker,
				temp
			);

			m_vDevices.push_back({EHobovrDeviceNodeTypes::tracker, temp});

			counter_trkr++;

		} else if (i == "g") {

			GazeMasterDriver* temp = new GazeMasterDriver(
				"g" + std::to_string(gaze_master_counter)
			);

			vr::VRServerDriverHost()->TrackedDeviceAdded(
						temp->GetSerialNumber().c_str(),
						vr::TrackedDeviceClass_GenericTracker,
						temp);
			m_vDevices.push_back({EHobovrDeviceNodeTypes::gaze_master, temp});

			gaze_master_counter++;

		} else {
		  DriverLog("driver: unsupported device type: %s", i.c_str());
		  return VRInitError_VendorSpecific_HmdFound_ConfigFailedSanityCheck;
		}
	}

	// start listening for device data
	m_pSocketComm->setCallback(this);

	// misc slow update thread
	m_bSlowUpdateThreadIsAlive = true;
	m_ptSlowUpdateThread = new std::thread(this->SlowUpdateThreadEnter, this);
	if (!m_bSlowUpdateThreadIsAlive) {
		DriverLog("driver: slow update thread broke on start\n");
		return VRInitError_IPC_Failed;
	}

	// settings manager
	m_pSettManTref = std::make_shared<HobovrTrackingRef_SettManager>("trsm0");
	vr::VRServerDriverHost()->TrackedDeviceAdded(
		m_pSettManTref->GetSerialNumber().c_str(),
		vr::TrackedDeviceClass_TrackingReference,
		m_pSettManTref.get()
	);

	m_pSettManTref->UpdatePose();

	return VRInitError_None;
}

void CServerDriver_hobovr::Cleanup() {
	DriverLog("driver cleanup called");
	m_pSocketComm->stop();
	m_bSlowUpdateThreadIsAlive = false;
	m_ptSlowUpdateThread->join();

	for (auto& i : m_vDevices) {
		switch (i.type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)i.handle;
				device->PowerOff();
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)i.handle;
				device->PowerOff();
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)i.handle;
				device->PowerOff();
				free(device);
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)i.handle;
				device->PowerOff();
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

void CServerDriver_hobovr::OnPacket(char* buff, int len) {
  if (len == (m_pSocketComm->m_iExpectedMessageSize+3) && !m_bDeviceListSyncEvent)
  {
	// float* temp= (float*)buff;
	uint32_t buff_offset = 0;

	for (size_t i=0; i < m_vDevices.size(); i++){
		switch (m_vDevices[i].type) {
			case EHobovrDeviceNodeTypes::hmd: {
				HeadsetDriver* device = (HeadsetDriver*)m_vDevices[i].handle;
				device->UpdateState(buff + buff_offset);

				buff_offset += device->get_packet_size();
				break;
			}

			case EHobovrDeviceNodeTypes::controller: {
				ControllerDriver* device = (ControllerDriver*)m_vDevices[i].handle;
				device->UpdateState(buff + buff_offset);

				buff_offset += device->get_packet_size();
				break;
			}

			case EHobovrDeviceNodeTypes::tracker: {
				TrackerDriver* device = (TrackerDriver*)m_vDevices[i].handle;
				device->UpdateState(buff + buff_offset);

				buff_offset += device->get_packet_size();
				break;
			}

			case EHobovrDeviceNodeTypes::gaze_master: {
				GazeMasterDriver* device = (GazeMasterDriver*)m_vDevices[i].handle;
				device->UpdateState(buff + buff_offset);

				buff_offset += device->get_packet_size();
				break;
			}
		}

	}

  } else {
	DriverLog("driver: posers are getting inored~\n\texpected %d bytes, got %d bytes~\n",
		(m_pSocketComm->m_iExpectedMessageSize+3),
		len
	);
  }

}

void CServerDriver_hobovr::RunFrame() {
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

		if (vrEvent.eventType == EHoboVR_VendorEvents::EHoboVR_UduChange) {
			DriverLog("udu change event");
			std::string newUduString = "";
			auto uduBufferCopy = m_pSettManTref->m_vpUduChangeBuffer;

			for (auto i : uduBufferCopy) {
				DriverLog("pair: (%s, %d)", i.first.c_str(), i.second);
				newUduString += i.first + " ";
			}

			m_bDeviceListSyncEvent = true;
			m_pSocketComm->UpdateParams(newUduString);
			UpdateServerDeviceList();
			m_bDeviceListSyncEvent = false;
		}
	}
}

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

	auto uduBufferCopy = m_pSettManTref->m_vpUduChangeBuffer;

	int counter_hmd = 0;
	int counter_cntrlr = 0;
	int counter_trkr = 0;
	int gaze_master_counter = 0;

	int controller_hs = 1;

	for (auto i : uduBufferCopy) {
		if (i.first == "h") {
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
		} else if (i.first == "c") {
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
					m_pSocketComm
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

		} else if (i.first == "t") {
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
				TrackerDriver* temp = new TrackerDriver("t" + std::to_string(counter_trkr), m_pSocketComm);

				vr::VRServerDriverHost()->TrackedDeviceAdded(
					temp->GetSerialNumber().c_str(),
					vr::TrackedDeviceClass_GenericTracker,
					temp
				);
				m_vDevices.push_back({EHobovrDeviceNodeTypes::tracker, temp});

			}

			counter_trkr++;
		} else if (i.first == "g") {
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

	m_pSettManTref->m_vpUduChangeBuffer.clear();
}

void CServerDriver_hobovr::SlowUpdateThread() {
	DriverLog("driver: slow update thread started\n");

	while (m_bSlowUpdateThreadIsAlive){
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
	m_bSlowUpdateThreadIsAlive = false;

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
