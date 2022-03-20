// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "controllers.h"

//-----------------------------------------------------------------------------
// Purpose:controller device implementation
//-----------------------------------------------------------------------------

ControllerDriver::ControllerDriver(
    bool side,
    std::string myserial,
    const std::shared_ptr<hobovr::tcp_socket> ReceiverObj
): HobovrDevice(myserial, "hobovr_controller_m", ReceiverObj), m_bHandSide(side) {

    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_controller_mc0";
    m_sBindPath = "{hobovr}/input/hobovr_controller_profile.json";

    m_skeletonHandle = vr::k_ulInvalidInputComponentHandle;
}

////////////////////////////////////////////////////////////////////////////////

vr::EVRInitError ControllerDriver::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    HobovrDevice::Activate(unObjectId); // let the parent handle boilerplate stuff

    // the "make me look like a vive wand" bullshit
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_TrackingSystemName_String,
        "lighthouse"
    );
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, m_serialNumber.c_str());
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_WillDriftInYaw_Bool,
        false
    );
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_DeviceIsWireless_Bool,
        true
    );
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_DeviceIsCharging_Bool, false);
    // vr::VRProperties()->SetFloatProperty(m_ulPropertyContainer, vr::Prop_DeviceBatteryPercentage_Float, 1.f); // Always charged

    // why is this here? this is a controller!
    vr::HmdMatrix34_t l_matrix = {
        -1.f, 0.f, 0.f, 0.f,
        0.f, 0.f, -1.f, 0.f,
        0.f, -1.f, 0.f, 0.f
    };
    vr::VRProperties()->SetProperty(
        m_ulPropertyContainer,
        vr::Prop_StatusDisplayTransform_Matrix34,
        &l_matrix, sizeof(vr::HmdMatrix34_t),
        vr::k_unHmdMatrix34PropertyTag
    );

    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_UpdateAvailable_Bool, false);
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ManualUpdate_Bool, false);
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_Firmware_ManualUpdateURL_String, "https://developer.valvesoftware.com/wiki/SteamVR/HowTo_Update_Firmware");
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_DeviceProvidesBatteryStatus_Bool, true);
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_DeviceCanPowerOff_Bool,
        true
    );
    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_DeviceClass_Int32,
        vr::TrackedDeviceClass_Controller
    );
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_ForceUpdateRequired_Bool, false);
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Identifiable_Bool, true);
    // vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, vr::Prop_Firmware_RemindUpdate_Bool, false);
    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_Axis0Type_Int32,
        vr::k_eControllerAxis_TrackPad
    );
    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_Axis1Type_Int32,
        vr::k_eControllerAxis_Trigger
    );
    // vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, (m_bHandSide) ? vr::TrackedControllerRole_RightHand : vr::TrackedControllerRole_LeftHand);
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_HasDisplayComponent_Bool,
        false
    );
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_HasCameraComponent_Bool,
        false
    );
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_HasDriverDirectModeComponent_Bool,
        false
    );
    vr::VRProperties()->SetBoolProperty(
        m_ulPropertyContainer,
        vr::Prop_HasVirtualDisplayComponent_Bool,
        false
    );
    vr::VRProperties()->SetInt32Property(
        m_ulPropertyContainer,
        vr::Prop_ControllerHandSelectionPriority_Int32,
        0
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_ModelNumber_String,
        "Vive. Controller MV"
    );
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_SerialNumber_String, m_serialNumber.c_str()); // Changed
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_RenderModelName_String, "vr_controller_vive_1_5");
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_ManufacturerName_String,
        "HTC"
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_TrackingFirmwareVersion_String,
        "1533720215 htcvrsoftware@firmware-win32 2018-08-08 FPGA 262(1.6/0/0) BL 0 VRC 1533720214 Radio 1532585738"
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_HardwareRevision_String,
        "product 129 rev 1.5.0 lot 2000/0/0 0"
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_ConnectedWirelessDongle_String,
        "1E8092840E"
    ); // Changed
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_HardwareRevision_Uint64,
        2164327680U
    );
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_FirmwareVersion_Uint64,
        1533720215U
    );
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_FPGAVersion_Uint64,
        262U
    );
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_VRCVersion_Uint64,
        1533720214U
    );
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_RadioVersion_Uint64,
        1532585738U
    );
    vr::VRProperties()->SetUint64Property(
        m_ulPropertyContainer,
        vr::Prop_DongleVersion_Uint64,
        1461100729U
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_ResourceRoot_String,
        "htc"
    );
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_RegisteredDeviceType_String,
        (m_bHandSide) ? "htc/vive_controllerLHR-F94B3BD9" : "htc/vive_controllerLHR-F94B3BD8"
    ); // Changed
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_InputProfilePath_String,
        "{htc}/input/vive_controller_profile.json"
    );
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{htc}/icons/controller_status_off.png");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{htc}/icons/controller_status_searching.gif");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{htc}/icons/controller_status_searching_alert.gif");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{htc}/icons/controller_status_ready.png");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{htc}/icons/controller_status_ready_alert.png");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{htc}/icons/controller_status_error.png");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{htc}/icons/controller_status_off.png");
    // vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{htc}/icons/controller_status_ready_low.png");
    vr::VRProperties()->SetStringProperty(
        m_ulPropertyContainer,
        vr::Prop_ControllerType_String,
        "vive_controller"
    );

    if (m_bHandSide) {
        vr::VRProperties()->SetInt32Property(
            m_ulPropertyContainer,
            vr::Prop_ControllerRoleHint_Int32,
            vr::TrackedControllerRole_RightHand
        );
      // vr::VRDriverInput()->CreateSkeletonComponent(m_ulPropertyContainer, "/input/skeleton/right", 
      //                 "/skeleton/hand/right", 
      //                 "/pose/raw", vr::VRSkeletalTracking_Partial
      //                 , nullptr, 0U, &m_skeletonHandle);
    } else {
        vr::VRProperties()->SetInt32Property(
            m_ulPropertyContainer,
            vr::Prop_ControllerRoleHint_Int32,
            vr::TrackedControllerRole_LeftHand
        );
        // vr::VRDriverInput()->CreateSkeletonComponent(m_ulPropertyContainer, "/input/skeleton/left", 
        //                 "/skeleton/hand/left", 
        //                 "/pose/raw", vr::VRSkeletalTracking_Partial
        //                 , nullptr, 0U, &m_skeletonHandle);
    }

    // create all the bool input components
    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/grip/click",
        &m_compGrip
    );

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/system/click",
        &m_compSystem
    );

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/application_menu/click",
        &m_compAppMenu
    );

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/trackpad/click",
        &m_compTrackpadClick
    );

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/trackpad/touch",
        &m_compTrackpadTouch
    );

    vr::VRDriverInput()->CreateBooleanComponent(
        m_ulPropertyContainer,
        "/input/trigger/click",
        &m_compTriggerClick
    );


    // trigger
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/trigger/value",
        &m_compTrigger,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedOneSided
    );

    // trackpad
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/trackpad/x",
        &m_compTrackpadX,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );
    vr::VRDriverInput()->CreateScalarComponent(
        m_ulPropertyContainer,
        "/input/trackpad/y",
        &m_compTrackpadY,
        vr::VRScalarType_Absolute,
        vr::VRScalarUnits_NormalizedTwoSided
    );


    return vr::VRInitError_None;
}

////////////////////////////////////////////////////////////////////////////////

void ControllerDriver::UpdateState(void* data) {
    HoboVR_ControllerState_t* packet = (HoboVR_ControllerState_t*)data;
    // update all the things
    // vr::DriverPose_t pose;
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

    auto ivrinput_cache = vr::VRDriverInput();

    ivrinput_cache->UpdateBooleanComponent(
        m_compGrip,
        (bool)(packet->inputs_mask & 0b000001),
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateBooleanComponent(
        m_compSystem,
        (bool)(packet->inputs_mask & 0b000010),
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateBooleanComponent(
        m_compAppMenu,
        (bool)(packet->inputs_mask & 0b000100),
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateBooleanComponent(
        m_compTrackpadClick,
        (bool)(packet->inputs_mask & 0b001000),
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateScalarComponent(
        m_compTrigger,
        packet->scalar_inputs[0],
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateScalarComponent(
        m_compTrackpadX,
        packet->scalar_inputs[1],
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateScalarComponent(
        m_compTrackpadY,
        packet->scalar_inputs[2],
        (double)m_fPoseTimeOffset
    );


    ivrinput_cache->UpdateBooleanComponent(
        m_compTrackpadTouch,
        (bool)(packet->inputs_mask & 0b010000),
        (double)m_fPoseTimeOffset
    );

    ivrinput_cache->UpdateBooleanComponent(
        m_compTriggerClick,
        (bool)(packet->inputs_mask & 0b100000),
        (double)m_fPoseTimeOffset
    );

}

////////////////////////////////////////////////////////////////////////////////

size_t ControllerDriver::GetPacketSize() {
    return sizeof(HoboVR_ControllerState_t);
}
