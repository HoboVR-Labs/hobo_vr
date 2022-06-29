// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_CONTRLRS
#define __HOBO_VR_CONTRLRS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"

//-----------------------------------------------------------------------------
// Purpose: controller device implementation
//-----------------------------------------------------------------------------


class ControllerDriver : public hobovr::HobovrDevice<true, false> {
public:
    ControllerDriver(
        bool side,
        std::string myserial,
        const std::shared_ptr<hobovr::tcp_socket> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

    void UpdateState(void* data) override;
    size_t GetPacketSize() override;


private:
    vr::DriverPose_t pose;
    vr::VRInputComponentHandle_t m_compGrip;
    vr::VRInputComponentHandle_t m_compSystem;
    vr::VRInputComponentHandle_t m_compAppMenu;
    vr::VRInputComponentHandle_t m_compTrigger;
    vr::VRInputComponentHandle_t m_compTrackpadX;
    vr::VRInputComponentHandle_t m_compTrackpadY;
    vr::VRInputComponentHandle_t m_compTrackpadTouch;
    vr::VRInputComponentHandle_t m_compTriggerClick;
    vr::VRInputComponentHandle_t m_compTrackpadClick;

    vr::VRInputComponentHandle_t m_skeletonHandle;

    bool m_bHandSide;

};

#endif // #ifndef __HOBO_VR_CONTRLRS