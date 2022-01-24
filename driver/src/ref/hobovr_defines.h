#ifndef __HOBO_VR_DEFINES
#define __HOBO_VR_DEFINES

namespace hobovr {
	// le version
	static const uint32_t k_nHobovrVersionMajor = 0;
	static const uint32_t k_nHobovrVersionMinor = 7;
	static const uint32_t k_nHobovrVersionBuild = 7;
	static const std::string k_sHobovrVersionGG = "deep dark";

} // namespace hobovr

// keys for use with the settings API
// driver keys
static const char *const k_pch_Hobovr_Section = "driver_hobovr";

// hmd device keys
static const char *const k_pch_Hmd_Section = "hobovr_device_hmd";
static const char *const k_pch_Hmd_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char *const k_pch_Hmd_DisplayFrequency_Float = "displayFrequency";
static const char* const k_pch_Hmd_IPD_Float = "IPD";
static const char* const k_pch_Hmd_UserHead2EyeDepthMeters_Float = "UserHeadToEyeDepthMeters";



enum EHoboVR_VendorEvents
{
	// in vendor event range, event data is VREvent_Notification_t
	EHoboVR_EyeTrackingActive = 19996,
	// in vendor event range, event data is VREvent_Notification_t,
	// after this event no eye tracking updates will come until the next VREvent_Notification_t
	EHoboVR_EyeTrackingEnd = 19997,
	// in the vendor event range, event data is VREvent_Notification_t
	EHoboVR_UduChange = 19998,
};

#endif // #ifndef __HOBO_VR_DEFINES