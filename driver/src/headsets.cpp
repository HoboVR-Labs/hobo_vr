
#include "headsets.h"

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

HeadsetDriver::HeadsetDriver(
    std::string myserial
):HobovrDevice(myserial, "hobovr_hmd_m") {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_hmd_mh0";
    m_sBindPath = "{hobovr}/input/hobovr_hmd_profile.json";

    m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section,
        k_pch_Hmd_SecondsFromVsyncToPhotons_Float
    );

    m_flDisplayFrequency = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section,
        k_pch_Hmd_DisplayFrequency_Float
    );

    m_flIPD = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section,
        k_pch_Hmd_IPD_Float
    );

    m_fUserHead2EyeDepthMeters = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section,
        k_pch_Hmd_UserHead2EyeDepthMeters_Float
    );

    // log non boilerplate device specific settings 
    DriverLog("device hmd settings: vsync time %fs, display freq %f, ipd %fm, head2eye depth %fm",
        m_flSecondsFromVsyncToPhotons,
        m_flDisplayFrequency,
        m_flIPD,
        m_fUserHead2EyeDepthMeters
    );

    hobovr::HobovrComponent_t extDisplayComp;
    extDisplayComp.type = hobovr::EHobovrCompType::EHobovrComp_ExtendedDisplay;
    extDisplayComp.tag = vr::IVRDisplayComponent_Version;
    extDisplayComp.ptr_handle = new hobovr::HobovrExtendedDisplayComponent();
    m_vComponents.push_back(extDisplayComp);
}

////////////////////////////////////////////////////////////////////////////////

vr::EVRInitError HeadsetDriver::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

    vr::VRProperties()->SetFloatProperty(
        m_ulPropertyContainer,
        vr::Prop_UserIpdMeters_Float,
        m_flIPD
    );

    vr::VRProperties()->SetFloatProperty(
        m_ulPropertyContainer,
        vr::Prop_UserHeadToEyeDepthMeters_Float,
        0.f
    );

    vr::VRProperties()->SetFloatProperty(
        m_ulPropertyContainer,
        vr::Prop_DisplayFrequency_Float,
        m_flDisplayFrequency
    );

    vr::VRProperties()->SetFloatProperty(
        m_ulPropertyContainer,
        vr::Prop_SecondsFromVsyncToPhotons_Float,
        m_flSecondsFromVsyncToPhotons
    );

    // avoid "not fullscreen" warnings from vrmonitor
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_IsOnDesktop_Bool,
        false
    );

    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_DisplayDebugMode_Bool,
        false
    );

    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_UserHeadToEyeDepthMeters_Float,
        m_fUserHead2EyeDepthMeters
    );

    return vr::VRInitError_None;
}

////////////////////////////////////////////////////////////////////////////////

void HeadsetDriver::UpdateSectionSettings() {
    // get new ipd
    m_flIPD = vr::VRSettings()->GetFloat(
        k_pch_Hmd_Section,
        k_pch_Hmd_IPD_Float
    );
    // set new ipd
    vr::VRProperties()->SetFloatProperty(
        m_ulPropertyContainer,
        vr::Prop_UserIpdMeters_Float,
        m_flIPD
    );

    DriverLog("device hmd: ipd set to %f", m_flIPD);
}

////////////////////////////////////////////////////////////////////////////////

void HeadsetDriver::UpdateState(void* data) {
    HoboVR_HeadsetPose_t* packet = (HoboVR_HeadsetPose_t*)data;

    vr::DriverPose_t pose;
    pose.poseTimeOffset = m_fPoseTimeOffset;
    pose.result = vr::TrackingResult_Running_OK;
    pose.poseIsValid = true;
    pose.deviceIsConnected = true;
    pose.willDriftInYaw = false;
    pose.shouldApplyHeadModel = true;
    pose.vecPosition[0] = packet->position[0];
    pose.vecPosition[1] = packet->position[1];
    pose.vecPosition[2] = packet->position[2];

    pose.qRotation = {
        (double)(packet->orientation[0]),
        (double)(packet->orientation[1]),
        (double)(packet->orientation[2]),
        (double)(packet->orientation[3])
    };

    pose.vecVelocity[0] = packet->velocity[0];
    pose.vecVelocity[1] = packet->velocity[1];
    pose.vecVelocity[2] = packet->velocity[2];

    pose.vecAngularVelocity[0] = packet->angular_velocity[0];
    pose.vecAngularVelocity[1] = packet->angular_velocity[1];
    pose.vecAngularVelocity[2] = packet->angular_velocity[2];

    pose.qWorldFromDriverRotation = { 1, 0, 0, 0 };
    pose.qDriverFromHeadRotation = { 1, 0, 0, 0 };
    pose.vecWorldFromDriverTranslation[0] = 0;
    pose.vecWorldFromDriverTranslation[1] = 0;
    pose.vecWorldFromDriverTranslation[2] = 0;

    pose.vecDriverFromHeadTranslation[0] = 0;
    pose.vecDriverFromHeadTranslation[1] = 0;
    pose.vecDriverFromHeadTranslation[2] = 0;

    if (m_unObjectId != vr::k_unTrackedDeviceIndexInvalid) {
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_unObjectId,
            pose,
            sizeof(pose)
        );
    }
}