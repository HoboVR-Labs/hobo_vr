
#ifndef __HOBO_VR_CONTRLRS
#define __HOBO_VR_CONTRLRS

#include <openvr_driver.h>
#include "driverlog.h"
#include "ref/hobovr_device_base.h"

#if defined(_WIN32)

#include "ref/receiver_win.h"

#elif defined(__linux__)

#include "ref/receiver_linux.h"
#define _stricmp strcasecmp

#endif

//-----------------------------------------------------------------------------
// Purpose:controller device implementation
//-----------------------------------------------------------------------------

struct HoboVR_ControllerState_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector

    float inputs[9];
    // vive wand style inputs

    // inputs[0] - grip button, recast as bool
    // inputs[1] - SteamVR system button, recast as bool
    // inputs[2] - app menu button, recast as bool
    // inputs[3] - trackpad click button, recast as bool
    // inputs[4] - trigger value, one sided normalized  scalar axis
    // inputs[5] - trackpad x axis, normalized two sided scalar axis
    // inputs[6] - trackpad y axis, normalized two sided scalar axis
    // inputs[7] - trackpad touch signal, recast as bool
    // inputs[8] - trigger click button, recast as bool
};

class ControllerDriver : public hobovr::HobovrDevice<true, false> {
public:
    ControllerDriver(
        bool side,
        std::string myserial,
        const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateState(void* data) override;

private:
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