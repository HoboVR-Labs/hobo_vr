// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_HMDS
#define __HOBO_VR_HMDS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

class HeadsetDriver : public hobovr::HobovrDevice<false, false> {
public:
    HeadsetDriver(std::string myserial);

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

    void UpdateSectionSettings() override;

    void UpdateState(void* data) override;
    size_t GetPacketSize() override;


private:
    vr::DriverPose_t pose;
    float m_flSecondsFromVsyncToPhotons;
    float m_flDisplayFrequency;
    float m_flIPD;
    float m_fUserHead2EyeDepthMeters;
};

#endif // #ifndef __HOBO_VR_HMDS