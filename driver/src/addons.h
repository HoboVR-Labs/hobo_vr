// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_ADDONS
#define __HOBO_VR_ADDONS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"

#include <shoom.h>

#include <thread>

//-----------------------------------------------------------------------------
// Purpose: eye tracking addon device, code name: Gaze Master
//-----------------------------------------------------------------------------

class GazeMasterDriver: public hobovr::HobovrDevice<false, false> {
public:
    GazeMasterDriver(std::string myserial);

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

    void UpdateState(void* data) override;
    size_t GetPacketSize() override;

    void PowerOff() override;
    void PowerOn() override;



private:

    void SlowUpdateThread();
    static void SlowUpdateThreadEnter(GazeMasterDriver* self);

private:

    std::shared_ptr<shoom::Shm> m_pSharedMemory;

    bool m_bSlowUpdateThreadIsAlive;
    bool m_bPause;
    std::thread* m_ptSlowUpdateThread;

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