// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "device_factory.h"

#include <openvr_driver.h>
#include <memory>

#include "driverlog.h"

// devices
#include "headsets.h"
#include "controllers.h"
#include "trackers.h"
#include "addons.h"


std::unique_ptr<hobovr::IHobovrDevice> hobovr::createDeviceFactory(
	char type,
	int number,
	std::shared_ptr<hobovr::tcp_socket> soc
) {

	switch(type) {
		case 'h': {
			DriverLog("%s: Creating Headset device: h%d", __FUNCTION__, number);

			auto tmp = std::make_unique<HeadsetDriver>(type + std::to_string(number));

			bool resp = vr::VRServerDriverHost()->TrackedDeviceAdded(
				tmp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_HMD,
				tmp.get()
			);

			if (!resp) {
				DriverLog("%s: Failed to create device");
				return nullptr;
			}

			return tmp;
		}

		case 'c': {
			DriverLog("%s: Creating Controller device: c%d", __FUNCTION__, number);

			auto tmp = std::make_unique<ControllerDriver>(
				!!(number % 2),
				type + std::to_string(number),
				soc
			);

			bool resp = vr::VRServerDriverHost()->TrackedDeviceAdded(
				tmp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_Controller,
				tmp.get()
			);

			if (!resp) {
				DriverLog("%s: Failed to create device");
				return nullptr;
			}

			return tmp;	
		}

		case 't': {
			DriverLog("%s: Creating Tracker device: t%d", __FUNCTION__, number);
			
			auto tmp = std::make_unique<TrackerDriver>(
				type + std::to_string(number),
				soc
			);

			bool resp = vr::VRServerDriverHost()->TrackedDeviceAdded(
				tmp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_GenericTracker,
				tmp.get()
			);

			if (!resp) {
				DriverLog("%s: Failed to create device");
				return nullptr;
			}

			return tmp;
		}

		case 'g': {
			DriverLog("%s: Creating GazeMaster device: t%d", __FUNCTION__, number);

			auto tmp = std::make_unique<GazeMasterDriver>(type + std::to_string(number));

			bool resp = vr::VRServerDriverHost()->TrackedDeviceAdded(
				tmp->GetSerialNumber().c_str(),
				vr::TrackedDeviceClass_GenericTracker,
				tmp.get()
			);

			if (!resp) {
				DriverLog("%s: Failed to create device");
				return nullptr;
			}

			return tmp;
		}

		default:
			DriverLog("%s: bad decive type: %c", __FUNCTION__, type);
	}

	return nullptr;
}