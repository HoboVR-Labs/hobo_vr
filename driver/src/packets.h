// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBOVR_PACKETS
#define __HOBOVR_PACKETS

#pragma pack(push, 1)

////////////////////////////////////////////////////////////////////////////////
// Common structs
////////////////////////////////////////////////////////////////////////////////

static constexpr char KHoboVR_TrackingIdMessage[] = "hello\n";
static constexpr char KHoboVR_ManagerIdMessage[]  = "monky\n";

struct PacketEndTag {
    char a;
    char b;
    char c;
    // char d;
};

static constexpr PacketEndTag g_EndTag = {'\t', '\r', '\n'};

enum EHoboVR_PoserRespTypes {
    EPoserRespType_invalid = 0,

    EPoserRespType_badDeviceList = 10, // poser fucked up, data is HoboVR_RespBufSize_t

    EPoserRespType_driverShutdown = 20, // driver shutdown was called, poser needs to exit

    EPoserRespType_haptics = 30, // device responds with a haptics packet

    EPoserRespType_max
};

struct HoboVR_HapticResponse_t {
    char name[10]; // device name: "%c%d", device_type, id
    double duration_seconds;
    double frequency;
    double amplitude;
};

struct HoboVR_RespBufSize_t {
    uint32_t size; // number of bytes allocated
};

struct HoboVR_RespReserved_t {
    uint8_t parts[32]; // we get 32 reserved bytes
};

typedef union {
    HoboVR_RespReserved_t reserved;
    HoboVR_RespBufSize_t buf_size;
    HoboVR_HapticResponse_t haptics;
} HoboVR_RespData_t;

struct HoboVR_PoserResp_t {
    uint32_t type; // EHoboVR_PoserRespTypes enum
    HoboVR_RespData_t data;
    PacketEndTag terminator;
};

////////////////////////////////////////////////////////////////////////////////
// Structs related to headsets
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: hmd device implementation
//-----------------------------------------------------------------------------

struct HoboVR_HeadsetPose_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector
};


////////////////////////////////////////////////////////////////////////////////
// Structs related to controllers
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose:controller device implementation
//-----------------------------------------------------------------------------

struct HoboVR_ControllerState_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector

    float scalar_inputs[3];
    // scalar_inputs[0] - trigger value, one sided normalized  scalar axis
    // scalar_inputs[1] - trackpad x axis, normalized two sided scalar axis
    // scalar_inputs[2] - trackpad y axis, normalized two sided scalar axis

    uint8_t inputs_mask: 6; // bit field of size 6
    // (bool)(inputs_mask & 0b000001) - grip button
    // (bool)(inputs_mask & 0b000010) - SteamVR system button
    // (bool)(inputs_mask & 0b000100) - app menu button
    // (bool)(inputs_mask & 0b001000) - trackpad click button
    // (bool)(inputs_mask & 0b010000) - trackpad touch signal
    // (bool)(inputs_mask & 0b100000) - trigger click button
};

////////////////////////////////////////////////////////////////////////////////
// Structs related to trackers
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: tracker device implementation
//-----------------------------------------------------------------------------

struct HoboVR_TrackerPose_t {
    float position[3];  // 3D vector
    float orientation[4];  // quaternion
    float velocity[3];  // 3D vector
    float angular_velocity[3];  // 3D vector
};


////////////////////////////////////////////////////////////////////////////////
// Structs related to the manager
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: tracking reference device that manages settings
//-----------------------------------------------------------------------------

enum EHoboVR_ManagerMsgTypes {
    EManagerMsgType_invalid = 0,
    EManagerMsgType_ipd = 10,
    EManagerMsgType_uduString = 20,
    EManagerMsgType_poseTimeOffset = 30,
    EManagerMsgType_distortion = 40,
    EManagerMsgType_eyeGap = 50,
    EManagerMsgType_setSelfPose = 60,
    EManagerMsgType_getTrkBuffSize = 70,

    EManagerMsgType_max
};

enum EHoboVR_DeviceTypes {
    EDeviceType_invalid = -1,
    EDeviceType_headset = 0,
    EDeviceType_controller = 1,
    EDeviceType_tracker = 2,
    EDeviceType_gazeMaster = 3,

    EDeviceType_max
};

// changes the ipd for hmd devices
struct HoboVR_ManagerMsgIpd_t {
    float ipd_meters;
};

// updates device list live
struct HoboVR_ManagerMsgUduString_t {
    uint16_t len; // number of devices, with the maximum being 512
    uint8_t devices[512]; // elements are EHoboVR_DeviceTypes enum
};

// updates pose time offsets
struct HoboVR_ManagerMsgPoseTimeOff_t {
    double time_offset_seconds;
};

// updates distortion parameters, will require a restart to take effect
struct HoboVR_ManagerMsgDistortion_t {
    double k1;
    double k2;
    double zoom_width;
    double zoom_height;
};

// updates the hobovr_comp_extendedDisplay eye gap setting,
// will require a restart to take effect
struct HoboVR_ManagerMsgEyeGap_t {
    uint32_t width; // in pixels
};

// updates the location of the virtual base station device (which manager runs as)
struct HoboVR_ManagerMsgSelfPose_t {
    float position[3];
};

struct HoboVR_ManagerMsgReserved_t {
    uint8_t meh[520]; // 520 total reserved bytes
};

union HoboVR_ManagerData_t {
    HoboVR_ManagerMsgReserved_t reserved;
    HoboVR_ManagerMsgIpd_t ipd;
    HoboVR_ManagerMsgUduString_t udu;
    HoboVR_ManagerMsgPoseTimeOff_t time_offset;
    HoboVR_ManagerMsgDistortion_t distortion;
    HoboVR_ManagerMsgEyeGap_t eye_offset;
    HoboVR_ManagerMsgSelfPose_t self_pose;
};

struct HoboVR_ManagerMsg_t {
    uint32_t type; // EHoboVR_ManagerMsgTypes enum
    HoboVR_ManagerData_t data;
    PacketEndTag terminator;
};

////////////////////////////////////////////////////////////////////////////////

enum EHoboVR_ManagerResp {
    EManagerResp_invalid = 0,

    EManagerResp_notification = 100,

    EManagerResp_ok = 200,
    EManagerResp_okRestartRequired = 201,

    EManagerResp_failed = 400,
    EManagerResp_failedToProcess = 401,

    EManagerResp_max
};

// manager response message, only sent in response to ManagerMsg structs
struct HoboVR_ManagerResp_t {
    uint32_t status; // EHoboVR_ManagerResp enum
};

////////////////////////////////////////////////////////////////////////////////
// Structs related to addons
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: eye tracking addon device, code name: Gaze Master
//-----------------------------------------------------------------------------

enum EHoboVR_GazeStatus {
    EGazeStatus_invalid =               0,
    EGazeStatus_active =                        0b00000000001, // everything is ok, both eyes are tracking
    EGazeStatus_calibrating =                   0b00000000010, // device is performing calibration,
                                                    // application might receive invalid data while this flag is present

    EGazeStatus_leftPupilLost =                 0b00000000100, // left eye lost tracking, (0,0) position will be used instead
    EGazeStatus_rightPupilLost =                0b00000001000, // right eye lost tracking, (0,0) position will be used instead
    EGazeStatus_lowPupilConfidence =            0b00000010000, // low confidence in pupil tracking, it will be smoothed

    EGazeStatus_leftPupilDilationLost =         0b00000100000, // left pupil dilation tracking lost, default to relaxed: 0.5
    EGazeStatus_rightPupilDilationLost =        0b00001000000, // right pupil dilation tracking lost, default to relaxed: 0.5
    EGazeStatus_lowPupilDilationConfidence =    0b00010000000, // low confidence in pupil dilation tracking, it will be smoothed

    EGazeStatus_leftNoEyeClose =                0b00100000000, // can't track how closed the left eyelid is, default to relaxed open: 0.1
    EGazeStatus_rightNoEyeClose =               0b01000000000, // can't track how closed the right eyelid is, default to relaxed open: 0.1
    EGazeStatus_lowEyeCloseConfidence =         0b10000000000, // low confidence in eyelid tracking, it will be smoothed

    EGazeStatus_max
};

struct HoboVR_GazeState_t {
    uint16_t status; // EHoboVR_GazeStatus enum

    double age_seconds;

    float pupil_position_r[2]; // vec 2D
    float pupil_position_l[2]; // vec 2D

    float pupil_dilation_r; // 0 to 1
    float pupil_dilation_l; // 0 to 1

    float eye_close_r; // 0 to 1
    float eye_close_l; // 0 to 1
};

#pragma pack(pop)

#endif // #ifndef __HOBOVR_PACKETS