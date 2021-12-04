
#include "addons.h"
#include "ref/hobovr_defines.h"

GazeMasterDriver::GazeMasterDriver(
    std::string myserial
): HobovrDevice(myserial, "") {
    m_sRenderModelPath = "{hobovr}/rendermodels/hobovr_tracker_mt0";

}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::UpdateState(void* data) {
    Gaze_t* packet = (Gaze_t*)data;

    m_Gaze = *packet;

    // vr::VREvent_SpatialAnchor_t event_data;
    // event_data.unHandle = (uint32_t)&m_Gaze;
    // vr::VRServerDriverHost()->VendorSpecificEvent(
    //     m_unObjectId,
    //     EHoboVR_EyeTrackingHandle,
    //     (vr::VREvent_Data_t&)event_data
    // );
}

////////////////////////////////////////////////////////////////////////////////

void GazeMasterDriver::ProcessEvent(const vr::VREvent_t &vrEvent) {
    HobovrDevice::ProcessEvent(vrEvent); // let parent handle boiler plate stuff

    // if we get this event we need to respond with a handle
    // this event is a request for a handle
    if (vrEvent.eventType == EHoboVR_EyeTrackingRequestHandle) {
        vr::VREvent_SpatialAnchor_t event_data;
        event_data.unHandle = (uint32_t)&m_Gaze;
        vr::VRServerDriverHost()->VendorSpecificEvent(
            m_unObjectId,
            (vr::EVREventType)EHoboVR_VendorEvents::EHoboVR_EyeTrackingHandle,
            (vr::VREvent_Data_t&)event_data,
            0
        );
    }
}