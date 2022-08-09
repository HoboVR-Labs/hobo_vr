// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#pragma once

#ifndef VR_DEVICE_BASE_H
#define VR_DEVICE_BASE_H

#include "hobovr_components.h"
#include "packets.h"

#include <lazy_sockets.h>

#include <string.h>

#include <openvr_driver.h>
#include "driverlog.h"

#ifdef WIN
	#define snprintf _snprintf
	#define vsnprintf _vsnprintf
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
#endif // #ifdef WIN

namespace hobovr {
	static const char *const k_pch_Hobovr_PoseTimeOffset_Float = "PoseTimeOffset";
	static const char *const k_pch_Hobovr_UpdateUrl_String = "ManualUpdateURL";

	enum EHobovrCompType
	{
		EHobovrComp_Invalid = 0,
		EHobovrComp_ExtendedDisplay = 100, // HobovrExtendedDisplayComponent component, use only with vr::IVRDisplayComponent_Version
		EHobovrComp_DriverDirectMode = 150, // HobovrDriverDirectModeComponent component, use only with vr::IVRDriverDirectModeComponent_Version
		EHobovrComp_Camera = 200, // HobovrCameraComponent component, use only with vr::IVRCameraComponent_Version
		EHobovrComp_VirtualDisplay = 250, // HobovrVirtualDisplayComponent component, use only with vr::IVRVirtualDisplay_Version
	};

	struct HobovrComponent_t
	{
		EHobovrCompType type;
		const char* tag; // for component search (e.g. vr::IVRDisplayComponent_Version)
		void* ptr_handle;
	};

	// for now this will never signal for updates, this same function will be executed for all derived device classes on Activate
	// you can implement your own version/update check here
	// this needs to be thread safe, it will be run in a slow thread, about every 5 seconds
	inline bool checkForDeviceUpdates(const std::string deviceSerial) {
		(void)deviceSerial; // because its not used and i cant remove this argument
		return false; // true steamvr will signal an update, false not, will always return false for now
	}



	// implement device charge update functionality here
	// this is called on each device charge update event
	// should return values within [0, 1] range, 1 is full, 0 is empty
	// this will receive the serial of the device on charge update event
	inline float GetDeviceCharge(const std::string deviceSerial) {
		(void)deviceSerial; // because its not used and i cant remove this argument
		return 1.0; // return permanent full charge for now
	}
	// NOTE: this function needs to be thread safe, it will be ran every 5 seconds


	// implement device charging indication management functionality here
	// this will be called on each device charge update event
	// this function should manage the indication of weather the device is charging or not
	// this will receive the serial of the device on charge update event
	inline bool ManageDeviceCharging(const std::string deviceSerial) {
		(void)deviceSerial; // because its not used and i cant remove this argument
		// true steamvr will signal device is charging, false not
		return false; // will always be false for now
	}
	// NOTE: this function needs to be thread safe, it will be ran every 5 seconds


	using tcp_socket = lsc::LSocket<AF_INET, SOCK_STREAM, 0>;
	using tcp_receiver_loop = lsc::ThreadedRecvLoop<AF_INET, SOCK_STREAM, 0, PacketEndTag>;

	///////////////////////////////////////////////////////////////////////////
	// Device interface class
	///////////////////////////////////////////////////////////////////////////

	class IHobovrDevice: public vr::ITrackedDeviceServerDriver {
	public:
		virtual ~IHobovrDevice() = default;
		virtual std::string GetSerialNumber() const = 0;
		virtual void PowerOff() = 0;
		virtual void PowerOn() = 0;
		virtual void ProcessEvent(const vr::VREvent_t &vrEvent) = 0;
		virtual void UpdateDeviceBatteryCharge() = 0;
		virtual void CheckForUpdates() = 0;
		virtual void UpdateSectionSettings() = 0;

		///////////////////////////////////////////////////////////////////////
		// these methods *have* to be overridden by the latest child
		///////////////////////////////////////////////////////////////////////

		virtual void UpdateState(void* packet) = 0;
		virtual size_t GetPacketSize() = 0;
	};

	///////////////////////////////////////////////////////////////////////////
	// Devices should publicly inherit this
	///////////////////////////////////////////////////////////////////////////

	template<bool UseHaptics, bool HasBattery>
	class HobovrDevice: public IHobovrDevice {
	public:
		inline HobovrDevice(
			const std::string& serial,
			const std::string& model,
			const std::string& renderModelPath,
			const std::string& bindingPath,
			std::shared_ptr<tcp_socket> commSocket=nullptr
		): m_pBrodcastSocket(commSocket),
			m_sRenderModelPath(renderModelPath),
			m_sBindPath(bindingPath),
			m_sSerialNumber(serial),
			m_sModelNumber(model) {

			m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
			m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

			m_fPoseTimeOffset = vr::VRSettings()->GetFloat(k_pch_Hobovr_Section, k_pch_Hobovr_PoseTimeOffset_Float);
			char buff[1024];
			vr::VRSettings()->GetString(k_pch_Hobovr_Section, k_pch_Hobovr_UpdateUrl_String, buff, sizeof(buff));
			m_sUpdateUrl = buff;

			DriverLog("device: created\n");
			DriverLog("device: breed: %s\n", model.c_str());
			DriverLog("device: serial: %s\n", m_sSerialNumber.c_str());
			DriverLog("device: model: %s\n", m_sModelNumber.c_str());
			DriverLog("device: pose time offset: %f\n", m_fPoseTimeOffset);

			if (m_pBrodcastSocket == nullptr && UseHaptics)
				DriverLog("no socket provided and haptics are enabled, this device will break on haptics\n");

			// m_Pose.result = TrackingResult_Running_OK;
			// m_Pose.poseTimeOffset = (double)m_fPoseTimeOffset;
			// m_Pose.qWorldFromDriverRotation = HmdQuaternion_Init(1, 0, 0, 0);
			// m_Pose.qDriverFromHeadRotation = HmdQuaternion_Init(1, 0, 0, 0);
			// m_Pose.qRotation = HmdQuaternion_Init(1, 0, 0, 0);
			// m_Pose.vecPosition[0] = 0.;
			// m_Pose.vecPosition[1] = 0.;
			// m_Pose.vecPosition[2] = 0.;
			// m_Pose.willDriftInYaw = true;
			// m_Pose.deviceIsConnected = true;
			// m_Pose.poseIsValid = true;
			// m_Pose.shouldApplyHeadModel = false;
		}

		inline virtual ~HobovrDevice() {
			for (auto &i : m_vComponents)
				free(i.ptr_handle);

			m_vComponents.clear();
			DriverLog("device: with serial %s yeeted out of existence\n", m_sSerialNumber.c_str());

		}

		///////////////////////////////////////////////////////////////////////
		// ITrackedDeviceServerDriver methods
		///////////////////////////////////////////////////////////////////////

		inline virtual vr::EVRInitError Activate(
			vr::TrackedDeviceIndex_t unObjectId
		) override {

			m_unObjectId = unObjectId;
			m_ulPropertyContainer =
					vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

			vr::VRProperties()->SetStringProperty(
				m_ulPropertyContainer,
				vr::Prop_ModelNumber_String,
				m_sModelNumber.c_str()
			);
			vr::VRProperties()->SetStringProperty(
				m_ulPropertyContainer,
				vr::Prop_RenderModelName_String,
				m_sRenderModelPath.c_str()
			);

			// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
			vr::VRProperties()->SetUint64Property(
				m_ulPropertyContainer,
				vr::Prop_CurrentUniverseId_Uint64,
				2
			);

			vr::VRProperties()->SetStringProperty(
				m_ulPropertyContainer,
				vr::Prop_InputProfilePath_String,
				m_sBindPath.c_str()
			);

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_DeviceCanPowerOff_Bool,
				true
			);

			DriverLog("device: activated\n");
			DriverLog("device: serial: %s\n", m_sSerialNumber.c_str());
			DriverLog("device: render model path: \"%s\"\n", m_sRenderModelPath.c_str());
			DriverLog("device: input binding path: \"%s\"\n", m_sBindPath.c_str());

			if constexpr(UseHaptics) {
				DriverLog("device: haptics added\n");
				vr::VRDriverInput()->CreateHapticComponent(
					m_ulPropertyContainer,
					"/output/haptic",
					&m_compHaptic
				);
			}

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_Identifiable_Bool,
				UseHaptics
			);


			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_DeviceProvidesBatteryStatus_Bool,
				HasBattery
			);

			if constexpr(HasBattery) {
				m_fDeviceCharge = GetDeviceCharge(m_sSerialNumber);
				vr::VRProperties()->SetBoolProperty(
					m_ulPropertyContainer,
					vr::Prop_DeviceIsCharging_Bool,
					false
				);

				vr::VRProperties()->SetFloatProperty(
					m_ulPropertyContainer,
					vr::Prop_DeviceBatteryPercentage_Float,
					m_fDeviceCharge
				);
				DriverLog(
					"device: has battery, current charge: %.3f%",
					m_fDeviceCharge*100
				);
			}

			vr::VRProperties()->SetStringProperty(
				m_ulPropertyContainer,
				vr::Prop_Firmware_ManualUpdateURL_String,
				m_sUpdateUrl.c_str()
			);

			bool shouldUpdate = checkForDeviceUpdates(m_sSerialNumber);

			if (shouldUpdate)
				DriverLog("device: update available!\n");

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_Firmware_UpdateAvailable_Bool,
				shouldUpdate
			);

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_Firmware_ManualUpdate_Bool,
				shouldUpdate
			);

			return vr::VRInitError_None;
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void Deactivate() override {
			DriverLog("device: \"%s\" deactivated\n", m_sSerialNumber.c_str());
			PowerOff();
			m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void EnterStandby() override {}

		///////////////////////////////////////////////////////////////////////

		/* debug request from a client, TODO: uh... actually implement this? */
		inline virtual void DebugRequest(
			const char *pchRequest,
			char *pchResponseBuffer,
			uint32_t unResponseBufferSize
		) override {
			DriverLog(
				"device: \"%s\" got a debug request: \"%s\"",
				m_sSerialNumber.c_str(),
				pchRequest
			);

			if (unResponseBufferSize >= 1)
				pchResponseBuffer[0] = 0;
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual vr::DriverPose_t GetPose() override {
			vr::DriverPose_t tmp;
			tmp.poseIsValid = true;
			tmp.deviceIsConnected = true;
			tmp.result = vr::TrackingResult_Running_OK;
			return tmp;
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void *GetComponent(
			const char *pchComponentNameAndVersion
		) override {
			for (auto &i : m_vComponents) {
				if (!_stricmp(pchComponentNameAndVersion, i.tag)){
					DriverLog("%s: request for \"%s\": component found",
						m_sSerialNumber.c_str(),
						pchComponentNameAndVersion
					);

					return i.ptr_handle;
				}
			}

			DriverLog("%s: request for \"%s\": component not found",
				m_sSerialNumber.c_str(),
				pchComponentNameAndVersion
			);

			return NULL;
		}

		///////////////////////////////////////////////////////////////////////
		// HoboVR device methods
		///////////////////////////////////////////////////////////////////////

		inline virtual std::string GetSerialNumber() const override {
			return m_sSerialNumber;
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void PowerOff() override {
			// signal device is "aliven't"
			vr::DriverPose_t pose;
			pose.poseTimeOffset = 0;
			pose.poseIsValid = false;
			pose.deviceIsConnected = false;
			if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
				vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
						m_unObjectId, pose, sizeof(pose));
			}
			DriverLog("device: '%s' disconnected", m_sSerialNumber.c_str());
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void PowerOn() override {
			// signal device is "alive"
			vr::DriverPose_t pose;
			pose.poseTimeOffset = 0;
			pose.poseIsValid = true;
			pose.deviceIsConnected = true;
			if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
				vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
						m_unObjectId, pose, sizeof(pose));
			}
			DriverLog("device: '%s' connected", m_sSerialNumber.c_str());
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void ProcessEvent(const vr::VREvent_t &vrEvent) override {
			if constexpr(UseHaptics)
			{
				if (vrEvent.eventType == vr::VREvent_Input_HapticVibration) {
					if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic) {
							// haptic!
							HoboVR_HapticResponse_t msg;
							// copy the device name
							memset(msg.name, 0, sizeof(msg.name));
							memcpy(
								msg.name,
								m_sSerialNumber.c_str(),
// a long ass expression of terribleness, but its to avoid a memory leak fast
m_sSerialNumber.size() * !!(m_sSerialNumber.size() < 10) + 10 * !(m_sSerialNumber.size() < 10)
							);

							msg.duration_seconds = vrEvent.data.hapticVibration.fDurationSeconds;
							msg.frequency = vrEvent.data.hapticVibration.fFrequency;
							msg.amplitude = vrEvent.data.hapticVibration.fAmplitude;

							HoboVR_PoserResp_t resp {
								EPoserRespType_haptics,
								(HoboVR_RespData_t&)msg,
								g_EndTag
							};

							m_pBrodcastSocket->Send(
								&resp,
								sizeof(resp)
							);
					}
				}
			}

			switch (vrEvent.eventType) {
				case vr::VREvent_OtherSectionSettingChanged: {
					// handle component settings update
					for (auto &i : m_vComponents) {
							switch(i.type){
								case EHobovrComp_ExtendedDisplay:
									((HobovrExtendedDisplayComponent*)i.ptr_handle)->ReloadSectionSettings();
									break;

								case EHobovrComp_DriverDirectMode:
									((HobovrDriverDirectModeComponent*)i.ptr_handle)->ReloadSectionSettings();
									break;

								case EHobovrComp_Camera:
									((HobovrCameraComponent*)i.ptr_handle)->ReloadSectionSettings();
									break;

								case EHobovrComp_VirtualDisplay:
									((HobovrVirtualDisplayComponent*)i.ptr_handle)->ReloadSectionSettings();
									break;


								default:
									DriverLog(
										"%s: invalid display component encountered on event update",
										m_sSerialNumber.c_str()
									);

							}
					}
					// handle device settings update
					UpdateSectionSettings();
					// DriverLog("device '%s': section settings changed", m_sSerialNumber.c_str());
				} break;
			}
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void UpdateDeviceBatteryCharge() override {
			if constexpr(HasBattery) {
				float fNewCharge = GetDeviceCharge(m_sSerialNumber);

				if (fNewCharge != m_fDeviceCharge){
					m_fDeviceCharge = fNewCharge;
					vr::VRProperties()->SetFloatProperty(
						m_ulPropertyContainer,
						vr::Prop_DeviceBatteryPercentage_Float,
						m_fDeviceCharge
					);

					DriverLog(
						"device: \"%s\" battery charge updated: %f",
						m_sSerialNumber,
						m_fDeviceCharge
					);
				}


				bool bNewIsCharging = ManageDeviceCharging(m_sSerialNumber);
				if (bNewIsCharging != m_bDeviceIsCharging) {
					m_bDeviceIsCharging = bNewIsCharging;
					vr::VRProperties()->SetBoolProperty(
						m_ulPropertyContainer,
						vr::Prop_DeviceIsCharging_Bool,
						m_bDeviceIsCharging
					);

					DriverLog(
						"device: \"%s\" is charging: %d",
						m_bDeviceIsCharging
					);
				}
			}
		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void CheckForUpdates() override {
			bool shouldUpdate = checkForDeviceUpdates(m_sSerialNumber);

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_Firmware_UpdateAvailable_Bool,
				shouldUpdate
			);

			vr::VRProperties()->SetBoolProperty(
				m_ulPropertyContainer,
				vr::Prop_Firmware_ManualUpdate_Bool,
				shouldUpdate
			);

		}

		///////////////////////////////////////////////////////////////////////

		inline virtual void UpdateSectionSettings() override {};

		///////////////////////////////////////////////////////////////////////

	protected:
		// openvr api stuff
		// DO NOT TOUCH THIS, parent will handle this, use it as read only!
		vr::TrackedDeviceIndex_t m_unObjectId;
		vr::PropertyContainerHandle_t m_ulPropertyContainer;

		// components that this device has, should be populated in the constructor of the derived class
		std::vector<HobovrComponent_t> m_vComponents;

		// time offset of the pose, set trough the config
		float m_fPoseTimeOffset;

		// hobovr stuff
		std::shared_ptr<tcp_socket> m_pBrodcastSocket;

	private:
		// openvr api stuff that i don't trust you to touch
		vr::VRInputComponentHandle_t m_compHaptic; // haptics, used if UseHaptics is true

		// device charge, 0-none, 1-full, only used if HasBattery is true
		float m_fDeviceCharge;
		// is device charing, 0-no, 1-yes, only used if HasBattery is true
		bool m_bDeviceIsCharging;

		// path to the device's render model
		std::string m_sRenderModelPath;
		// path to the device's input bindings
		std::string m_sBindPath;
		// url to which steamvr will redirect if checkForDeviceUpdates returns true on Activate, set trough the config
		std::string m_sUpdateUrl;
		// steamvr uses this to identify devices, no need for you to touch this after init
		std::string m_sSerialNumber;
		// steamvr uses this to identify devices, no need for you to touch this after init
		std::string m_sModelNumber;
	};
}

#endif // VR_DEVICE_BASE_H