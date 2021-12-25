#ifndef __HOBOVR_PACKETS
#define __HOBOVR_PACKETS

#pragma pack(push, 1)

////////////////////////////////////////////////////////////////////////////////
// Common structs
////////////////////////////////////////////////////////////////////////////////


struct HoboVR_HapticResponse_t {
    char name[10]; // device name: "%c%d", device_type, id
    double duration_seconds;
    double frequency;
    double amplitude;
};

enum EHoboVR_PoserRespType {
    EPoserRespType_invalid = 0,

    EPoserRespType_badDeviceList = 10, // poser fucked up, data is HoboVR_RespBufSize_t

    EPoserRespType_max
};

struct HoboVR_RespBufSize_t {
    uint32_t size; // number of bytes allocated
};

struct HoboVR_RespReserved_t {
    uint8_t parts[64]; // we get 32 reserved bytes
};

typedef union {
    HoboVR_RespReserved_t reserved;
    HoboVR_RespBufSize_t buf_size;
} HoboVR_RespData_t;

struct HoboVR_PoserResp_t {
    uint32_t type; // EHoboVR_PoserRespType enum
    HoboVR_RespData_t data;
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
// Structs related to addons
////////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// Purpose: eye tracking addon device, code name: Gaze Master
//-----------------------------------------------------------------------------

enum EHoboVR_GazeStatus {
    EGazeStatus_invalid = 0,
    EGazeStatus_active = 1, // everything is ok, both eyes are tracking
    EGazeStatus_leftEyeLost = 2, // left eye lost tracking
    EGazeStatus_rightEyeLost = 3, // right eye lost tracking
    EGazeStatus_bothEyesLost = 4, // both eyes lost tracking
    EGazeStatus_lowConfidence = 5, // low confidence in tracking

    EGazeStatus_max
};

struct HoboVR_GazeState_t {
    uint32_t status; // EHoboVR_GazeStatus enum

    float age_seconds;

    float gaze_direction_r[2]; // vec 2
    float gaze_direction_l[2]; // vec 2
    float gaze_orientation_r[4]; // quat
    float gaze_orientation_l[4]; // quat
    // maybe something else?
};

#pragma pack(pop)

#endif // #ifndef __HOBOVR_PACKETS