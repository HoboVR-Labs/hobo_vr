
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

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateSectionSettings();

    void UpdateState(void* data) override;

    uint32_t get_packet_size();

private:
    float m_flSecondsFromVsyncToPhotons;
    float m_flDisplayFrequency;
    float m_flIPD;
    float m_fUserHead2EyeDepthMeters;
};

#endif // #ifndef __HOBO_VR_HMDS