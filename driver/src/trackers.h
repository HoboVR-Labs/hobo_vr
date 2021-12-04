#ifndef __HOBO_VR_TRACKERS
#define __HOBO_VR_TRACKERS

#include <openvr_driver.h>
#include "driverlog.h"
#include "ref/hobovr_device_base.h"


//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------

struct HoboVR_TrackerPose_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector
};


class TrackerDriver : public hobovr::HobovrDevice<true, false> {
public:
    TrackerDriver(
        std::string myserial,
        const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateState(void* data) override;
};


#endif // #ifndef __HOBO_VR_TRACKERS