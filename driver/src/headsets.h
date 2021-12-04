
#ifndef __HOBO_VR_HMDS
#define __HOBO_VR_HMDS

#include <openvr_driver.h>
#include "driverlog.h"
#include "ref/hobovr_device_base.h"

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

struct HoboVR_HeadsetPose_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector
};

class HeadsetDriver : public hobovr::HobovrDevice<false, false> {
public:
    HeadsetDriver(std::string myserial);

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateSectionSettings();

    void UpdateState(void* data) override;

private:
    float m_flSecondsFromVsyncToPhotons;
    float m_flDisplayFrequency;
    float m_flIPD;
    float m_fUserHead2EyeDepthMeters;
};

#endif // #ifndef __HOBO_VR_HMDS