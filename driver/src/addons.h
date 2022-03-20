// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_ADDONS
#define __HOBO_VR_ADDONS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"

// #include <shoom.h>

// #include <thread>

//-----------------------------------------------------------------------------
// Purpose: eye tracking addon device, code name: Gaze Master
//-----------------------------------------------------------------------------

class GazeMasterDriver: public hobovr::HobovrDevice<false, false> {
public:
    GazeMasterDriver(std::string myserial);

    virtual vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) final;

    virtual void UpdateState(void* data) final;
    inline virtual size_t GetPacketSize() final {return sizeof(HoboVR_GazeState_t);}


private:

    vr::VRInputComponentHandle_t m_comp_gaze_l[2];
    vr::VRInputComponentHandle_t m_comp_gaze_r[2];
    vr::VRInputComponentHandle_t m_comp_occlusion[2];

};



//-----------------------------------------------------------------------------
// Purpose: (WIP) face tracking addon device, code name: Smile Driver
//-----------------------------------------------------------------------------

class SmileDriver: public hobovr::HobovrDevice<false, false> {
public:
    SmileDriver(std::string myserial);

    void UpdateState(void* data) override;
    size_t GetPacketSize() override;
};

#endif // #ifndef __HOBO_VR_ADDONS