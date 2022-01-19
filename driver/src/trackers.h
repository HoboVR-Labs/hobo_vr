// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_TRACKERS
#define __HOBO_VR_TRACKERS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"


//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------

class TrackerDriver : public hobovr::HobovrDevice<true, false> {
public:
    TrackerDriver(
        std::string myserial,
        const std::shared_ptr<hobovr::tcp_socket> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

    void UpdateState(void* data) override;
    size_t GetPacketSize() override;

};


#endif // #ifndef __HOBO_VR_TRACKERS