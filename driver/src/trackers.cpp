
#include "trackers.h"

//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------

TrackerDriver::TrackerDriver(
    std::string myserial,
    const std::shared_ptr<SockReceiver::DriverReceiver> ReceiverObj
): HobovrDevice(myserial, "hobovr_tracker_m", ReceiverObj) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";
    m_sBindPath = "{hobovr}/input/hobovr_tracker_profile.json";
}

////////////////////////////////////////////////////////////////////////////////

vr::EVRInitError TrackerDriver::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_ControllerRoleHint_Int32,
        vr::TrackedControllerRole_RightHand
    );

    return vr::VRInitError_None;
}

////////////////////////////////////////////////////////////////////////////////

void TrackerDriver::UpdateState(void* data) {
    HoboVR_TrackerPose_t* packet = (HoboVR_TrackerPose_t*)data;

    // update all the things
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