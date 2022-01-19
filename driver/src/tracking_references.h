// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_TRACKING_REF
#define __HOBO_VR_TRACKING_REF

#include <vector>
#include <string>
#include <openvr_driver.h>
#include "driverlog.h"

#include "hobovr_device_base.h"

//-----------------------------------------------------------------------------
// Purpose: settings manager using a tracking reference, this is meant to be
// a settings and other utility communication and management tool
// for that purpose it will have a it's own server connection and poser protocol
// for now tho it will just stay as a tracking reference
// also a device that will always be present with hobo_vr devices from now on
// it will also allow live device management in the future
// 
// think of it as a settings manager made to look like a tracking reference
//-----------------------------------------------------------------------------

enum HobovrTrackingRef_Msg_type
{
    Emsg_invalid = 0,
    Emsg_ipd = 10,
    Emsg_uduString = 20,
    Emsg_poseTimeOffset = 30,
    Emsg_distortion = 40,
    Emsg_eyeGap = 50,
    Emsg_setSelfPose = 60,
};

// static std::vector<std::pair<std::string, int>> g_vpUduChangeBuffer;

class HobovrTrackingRef_SettManager: public vr::ITrackedDeviceServerDriver {
private:
    PacketEndTag m_tag = {'\t', '\r', '\n'};
    std::unique_ptr<hobovr::tcp_socket> m_pSocketComm;
    std::unique_ptr<hobovr::tcp_receiver_loop> m_pReceiver;


public:
    HobovrTrackingRef_SettManager(std::string myserial);
    inline virtual ~HobovrTrackingRef_SettManager() {
        m_pReceiver->Stop();
    }

    void OnPacket(void* buff, size_t len);

    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

    void Deactivate() override;

    inline void EnterStandby() override {}

    void *GetComponent(const char *pchComponentNameAndVersion) override;

    void DebugRequest(
        const char *pchRequest,
        char *pchResponseBuffer,
        uint32_t unResponseBufferSize
    ) override;

    vr::DriverPose_t GetPose() override;

    std::string GetSerialNumber() const;

    void UpdatePose();
    bool GetUduEvent();
    std::string GetUduBuffer();

private:
    // interproc sync
    bool _UduEvent;
    std::string _vpUduChangeBuffer; // for passing udu data

    // openvr api stuff
    vr::TrackedDeviceIndex_t m_unObjectId; // DO NOT TOUCH THIS, parent will handle this, use it as read only!
    vr::PropertyContainerHandle_t m_ulPropertyContainer; // THIS EITHER, use it as read only!

    std::string m_sRenderModelPath; // path to the device's render model, should be populated in the constructor of the derived class

    vr::DriverPose_t m_Pose; // device's pose, use this at runtime

    std::string m_sSerialNumber; // steamvr uses this to identify devices, no need for you to touch this after init
    std::string m_sModelNumber; // steamvr uses this to identify devices, no need for you to touch this after init

};

#endif // #ifndef __HOBO_VR_TRACKING_REF