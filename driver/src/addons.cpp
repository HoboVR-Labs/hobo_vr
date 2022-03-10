// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "addons.h"
#include "hobovr_defines.h"

GazeMasterDriver::GazeMasterDriver(
    std::string myserial
): HobovrDevice(myserial, "hobovr_gaze_master") {
    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_gaze_master";
    m_bSlowUpdateThreadIsAlive = false;
    m_bPause = false;

    m_pSharedMemory = std::make_shared<shoom::Shm>(
        "hobovr_gaze_master_state",
        sizeof(HoboVR_GazeState_t)
    );
}

////////////////////////////////////////////////////////////////////////////////

vr::EVRInitError GazeMasterDriver::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId);


    DriverLog(
        "GazeMaster: creating shmem for %d bytes on '%s' path...",
        m_pSharedMemory->Size(),
        m_pSharedMemory->Path().c_str()
    );

    if (m_pSharedMemory->Create() != shoom::kOK) {
        DriverLog(
            "GazeMaster: failed to open shmem on %s",
            m_pSharedMemory->Path().c_str()
        );
        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid; // to stop state updates...
        return vr::VRInitError_IPC_SharedStateInitFailed;
    }
    DriverLog("GazeMaster: success!");

    // start with invalid state
    ((HoboVR_GazeState_t*)m_pSharedMemory->Data())->status = EGazeStatus_invalid;

    m_bSlowUpdateThreadIsAlive = true;
    m_ptSlowUpdateThread = new std::thread(SlowUpdateThreadEnter, this);

    PowerOn();

    return vr::VRInitError_None;
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::SlowUpdateThreadEnter(GazeMasterDriver* self) {
    self->SlowUpdateThread();
}


////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::SlowUpdateThread() {
    while (m_bSlowUpdateThreadIsAlive) {

        if (!m_bPause && m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
            vr::VREvent_Notification_t event_data = {69, 0};
            vr::VRServerDriverHost()->VendorSpecificEvent(
                m_unObjectId,
                (vr::EVREventType)EHoboVR_VendorEvents::EHoboVR_EyeTrackingActive,
                (vr::VREvent_Data_t&)event_data,
                0
            );
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::PowerOff() {
    HobovrDevice::PowerOff();

    m_bPause = true;

    vr::VREvent_Notification_t event_data = {41, 0};
    vr::VRServerDriverHost()->VendorSpecificEvent(
        m_unObjectId,
        (vr::EVREventType)EHoboVR_VendorEvents::EHoboVR_EyeTrackingEnd,
        (vr::VREvent_Data_t&)event_data,
        0
    );
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::PowerOn() {
    HobovrDevice::PowerOn();

    m_bPause = false;

    vr::VREvent_Notification_t event_data = {41, 0};
    vr::VRServerDriverHost()->VendorSpecificEvent(
        m_unObjectId,
        (vr::EVREventType)EHoboVR_VendorEvents::EHoboVR_EyeTrackingActive,
        (vr::VREvent_Data_t&)event_data,
        0
    );
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::UpdateState(void* data) {
    HoboVR_GazeState_t* packet = (HoboVR_GazeState_t*)data;
    // (void)packet;
    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
        HoboVR_GazeState_t* shared_state = (HoboVR_GazeState_t*)m_pSharedMemory->Data();
        // (void)shared_state;
        *shared_state = *packet;
    }

}

////////////////////////////////////////////////////////////////////////////////

size_t GazeMasterDriver::GetPacketSize() {
    return sizeof(HoboVR_GazeState_t);
}