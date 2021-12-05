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
        const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateState(void* data) override;

    uint32_t get_packet_size();
};


#endif // #ifndef __HOBO_VR_TRACKERS