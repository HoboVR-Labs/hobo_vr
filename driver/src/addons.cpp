// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "addons.h"
#include "hobovr_defines.h"

GazeMasterDriver::GazeMasterDriver(
    std::string myserial
): HobovrDevice(
    myserial,
    "hobovr_gaze_master",
    "{hobovr}/rendermodels/hobovr_gaze_master",
    "{hobovr}/input/hobovr_gaze_master_profile.json"
) {}

////////////////////////////////////////////////////////////////////////////////

vr::EVRInitError GazeMasterDriver::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    auto res = HobovrDevice::Activate(unObjectId);

    // if parent failed on Activate, stop here
    if (res != vr::VRInitError_None)
        return res;

    // opt out of hand selection, this device is meant for your face!
    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_ControllerRoleHint_Int32,
        vr::TrackedControllerRole_OptOut
    );

    // gaze directions
    // left eye
    auto err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/left/x",
        m_pupil_pos_l + 0,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 1: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/left/y",
        m_pupil_pos_l + 1,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 2: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    // right eye
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/right/x",
        m_pupil_pos_r + 0,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 3: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/right/y",
        m_pupil_pos_r + 1,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 4: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    // gaze occlusion
    // left eye
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/occlusion/left/value",
        m_eye_closed + 0,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 5: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    // right eye
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/occlusion/right/value",
        m_eye_closed + 1,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 6: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    // pupil dilation
    // left eye
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/dilation/left/value",
        m_pupil_dilation + 0,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 7: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    // right eye
    err = vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/gaze/dilation/right/value",
        m_pupil_dilation + 1,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided
    );
    if (err) {
        DriverLog(
            "gaze_master: failed to create scalar component 7: errno=%d",
            (int)err
        );
        return vr::VRInitError_Unknown;
    }

    return vr::VRInitError_None;
}


////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::UpdateState(void* data) {
    // do nothing if not activated
    if (m_unObjectId == vr::k_unTrackedDeviceIndexInvalid) {
        return;
    }


    HoboVR_GazeState_t* packet = (HoboVR_GazeState_t*)data;

    updateStatus(packet->status);

    // process inputs
    // pupil position tracking
    float default_pupil_pos[2] = {0.0f, 0.0f};
    auto pupil_position_l = (packet->status & EGazeStatus_leftPupilLost)
            ? default_pupil_pos : packet->pupil_position_l;

    auto pupil_position_r = (packet->status & EGazeStatus_rightPupilLost)
            ? default_pupil_pos : packet->pupil_position_r;

    // pupil dilation tracking
    float pupil_dilation_l =
            (packet->status & EGazeStatus_leftPupilDilationLost)
            ? 0.5f : packet->pupil_dilation_l;
    float pupil_dilation_r =
            (packet->status & EGazeStatus_rightPupilDilationLost)
            ? 0.5f : packet->pupil_dilation_r;

    // eye close tracking
    float eye_close_l = (packet->status & EGazeStatus_leftNoEyeClose)
            ? 0.1f : packet->eye_close_l;
    float eye_close_r = (packet->status & EGazeStatus_rightNoEyeClose)
            ? 0.1f : packet->eye_close_r;

    // process smoothing
    if (packet->status & EGazeStatus_lowPupilConfidence) {
        auto [l_x, l_y] = smooth2D(pupil_position_l);
        auto [r_x, r_y] = smooth2D(pupil_position_r);
        pupil_position_l[0] = l_x;
        pupil_position_l[1] = l_y;
        pupil_position_r[0] = r_x;
        pupil_position_r[1] = r_y;
    }

    if (packet->status & EGazeStatus_lowPupilDilationConfidence) {
        pupil_dilation_l = smooth1D(pupil_dilation_l);
        pupil_dilation_r = smooth1D(pupil_dilation_r);
    }

    if (packet->status & EGazeStatus_lowEyeCloseConfidence) {
        eye_close_l = smooth1D(eye_close_l);
        eye_close_r = smooth1D(eye_close_r);
    }


    auto ivrinput_cache = vr::VRDriverInput();

    // left eye dir
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_pos_l[0],
        pupil_position_l[0],
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_pos_l[1],
        pupil_position_l[1],
        packet->age_seconds
    );

    // right eye dir
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_pos_r[0],
        pupil_position_r[0],
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_pos_r[1],
        pupil_position_r[1],
        packet->age_seconds
    );

    // both eye occlusion
    ivrinput_cache->UpdateScalarComponent(
        m_eye_closed[0],
        eye_close_l,
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_eye_closed[1],
        eye_close_r,
        packet->age_seconds
    );

    // both eye dilation
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_dilation[0],
        pupil_dilation_l,
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_pupil_dilation[1],
        pupil_dilation_r,
        packet->age_seconds
    );

}

////////////////////////////////////////////////////////////////////////////////

float GazeMasterDriver::smooth1D(float val) {
    m_smooth_buff_1D.push_back(val);

    if (m_smooth_buff_1D.size() >= smooth_buff_size_max)
        m_smooth_buff_1D.erase(m_smooth_buff_1D.begin());

    float ret = 0.0f;

    for (auto i : m_smooth_buff_1D)
        ret += i;

    return ret / m_smooth_buff_1D.size();
}

////////////////////////////////////////////////////////////////////////////////

std::pair<float, float> GazeMasterDriver::smooth2D(float vec[2]) {
    m_smooth_buff_2D.push_back({vec[0], vec[1]});

    if (m_smooth_buff_2D.size() >= smooth_buff_size_max)
        m_smooth_buff_2D.erase(m_smooth_buff_2D.begin());

    float ret[2] = {0.0f, 0.0f};

    for (auto& i : m_smooth_buff_2D) {
        ret[0] += i.first;
        ret[1] += i.second;
    }

    return {ret[0] / m_smooth_buff_2D.size(), ret[1] / m_smooth_buff_2D.size()};
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::updateStatus(const uint16_t status) {
    vr::DriverPose_t pose;
    memset(&pose, 0, sizeof(pose));

    if (status & EGazeStatus_active) {
        pose.deviceIsConnected = true;
        pose.result = vr::TrackingResult_Running_OK;
    }

    if (status & EGazeStatus_calibrating)
        pose.result = vr::TrackingResult_Calibrating_InProgress;

    if (status < EGazeStatus_active) {
        pose.deviceIsConnected = false;
        pose.result = vr::TrackingResult_Uninitialized;
    }

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_unObjectId,
            pose,
            sizeof(pose)
        );
    }
}
