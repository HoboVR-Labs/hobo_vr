
#ifndef __HOBO_VR_ADDONS
#define __HOBO_VR_ADDONS

#include <openvr_driver.h>
#include "driverlog.h"
#include "ref/hobovr_device_base.h"

//-----------------------------------------------------------------------------
// Purpose: eye tracking addon device, code name: Gaze Master
//-----------------------------------------------------------------------------

enum EGazeStatus {
    EGazeStatus_invalid = 0,
    EGazeStatus_active = 1,

    EGazeStatus_max
};

struct Gaze_t {
    uint32_t status; // EGazeStatus enum

    float gaze_direction_r[2]; // vec 2
    float gaze_direction_l[2]; // vec 2
    float gaze_orientation_r[4]; // quat
    float gaze_orientation_l[4]; // quat
    // maybe something else?
};

class GazeMasterDriver: public hobovr::HobovrDevice<false, false> {
public:
    GazeMasterDriver(std::string myserial);

    void ProcessEvent(const vr::VREvent_t &vrEvent);

    void UpdateState(void* data) override;

public:

    Gaze_t m_Gaze; // public member for sharing eye tracking state
};



//-----------------------------------------------------------------------------
// Purpose: (WIP) face tracking addon device, code name: Smile Driver
//-----------------------------------------------------------------------------

class SmileDriver: public hobovr::HobovrDevice<false, false> {
public:
    SmileDriver(std::string myserial);

    void UpdateState(void* data) override;
};

#endif // #ifndef __HOBO_VR_ADDONS