
#ifndef __HOBO_VR_CONTRLRS
#define __HOBO_VR_CONTRLRS

#include <openvr_driver.h>
#include "driverlog.h"
#include "hobovr_device_base.h"
#include "packets.h"

#if defined(_WIN32)

#include "receiver_win.h"

#elif defined(__linux__)

#include "receiver_linux.h"
#define _stricmp strcasecmp

#endif

//-----------------------------------------------------------------------------
// Purpose: controller device implementation
//-----------------------------------------------------------------------------


class ControllerDriver : public hobovr::HobovrDevice<true, false> {
public:
    ControllerDriver(
        bool side,
        std::string myserial,
        const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
    );

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

    void UpdateState(void* data) override;

    uint32_t get_packet_size();

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