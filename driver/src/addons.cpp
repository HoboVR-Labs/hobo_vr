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
        m_comp_gaze_l + 0,
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
        m_comp_gaze_l + 1,
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
        m_comp_gaze_r + 0,
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
        m_comp_gaze_r + 1,
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
        m_comp_occlusion + 0,
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
        m_comp_occlusion + 1,
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

    return vr::VRInitError_None;
}


////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::UpdateState(void* data) {
    // do nothing if not activated
    if (m_unObjectId == vr::k_unTrackedDeviceIndexInvalid) {
        return;
    }

    HoboVR_GazeState_t* packet = (HoboVR_GazeState_t*)data;

    auto ivrinput_cache = vr::VRDriverInput();

    // left eye dir
    ivrinput_cache->UpdateScalarComponent(
        m_comp_gaze_l[0],
        packet->gaze_direction_l[0],
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_comp_gaze_l[1],
        packet->gaze_direction_l[1],
        packet->age_seconds
    );

    // right eye dir
    ivrinput_cache->UpdateScalarComponent(
        m_comp_gaze_r[0],
        packet->gaze_direction_r[0],
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_comp_gaze_r[1],
        packet->gaze_direction_r[1],
        packet->age_seconds
    );

    // both eye occlusion
    ivrinput_cache->UpdateScalarComponent(
        m_comp_occlusion[0],
        packet->left_eye_occlusion,
        packet->age_seconds
    );
    ivrinput_cache->UpdateScalarComponent(
        m_comp_occlusion[1],
        packet->right_eye_occlusion,
        packet->age_seconds
    );

}

