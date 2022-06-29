// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "addons.h"
#include "hobovr_defines.h"

#include <filesystem>
#include "path_helpers.h"

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

    // yes this cursed call needs to be done this way

    #if defined(WIN)
    auto fptr = &GazeMasterDriver::Activate;
    std::string driver_path = hobovr::DLWrapper::get_symbol_path(
        reinterpret_cast<void*&>(fptr));

    #elif defined(LINUX)

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
    std::string driver_path = hobovr::DLWrapper::get_symbol_path(
        (void*)&GazeMasterDriver::Activate);
    #pragma GCC diagnostic pop

    #endif

    if (driver_path.size() == 0) {
        DriverLog("GazeMasterDriver::Activate() could not resolve driver path, no plugins will be loaded");
        return vr::VRInitError_None;
    }


    std::string driver_folder_path = hobovr::Path_StripFilename(driver_path);
    // driver_folder_path = hobovr::Path_Join(driver_folder_path, "..", "plugins");

    DriverLog("GazeMasterDriver::Activate() driver path: '%s'", driver_folder_path.c_str());

    std::vector<std::string> plugin_paths;

    std::string dl_exention = hobovr::Path_GetExtension(driver_path);

    for (const auto& i : std::filesystem::directory_iterator(driver_folder_path)) {
        //DriverLog("\t%s -> %s", i.path().u8string().c_str(), hobovr::Path_GetExtension(i.path().u8string()).c_str());
        if (hobovr::Path_GetExtension(i.path().u8string()) == dl_exention && i.path().u8string() != driver_path)
            plugin_paths.push_back(i.path().u8string());
    }

    if (plugin_paths.size() == 0) {
        DriverLog("GazeMasterDriver::Activate() 0 potential plugins found");
        return vr::VRInitError_None;
    }

    DriverLog("GazeMasterDriver::Activate() Processing %d potential plugins:",
        plugin_paths.size());

    for (auto& i : plugin_paths)
        m_plugin_handles.emplace_back(i);

    std::vector<std::unique_ptr<gaze::PluginBase>> plugin_adapters_candidates;
    for (auto& i : m_plugin_handles) {
        if (i.is_alive()) {

            void* symbol_res = i.get_symbol("gazePluginFactory");
            if (symbol_res != nullptr) {
                gaze::PluginBase* (*pluginFactory)();
                pluginFactory = (gaze::PluginBase* (*)())symbol_res;

                gaze::PluginBase* pluginInterface = pluginFactory();
                if (pluginInterface == nullptr) {
                    DriverLog("\tFailed to init plugin adapter");
                    continue;
                }

                DriverLog("\tAdded plugin adapter: '%s'",
                    pluginInterface->GetNameAndVersion().c_str());
                plugin_adapters_candidates.emplace_back(pluginInterface);

            } else {
                DriverLog("\tSkipping plugin, could not get factory symbol: '%s'",
                    i.get_last_error().c_str());
            }
        } else {
            DriverLog("\tFailed to init plugin: '%s'", i.get_last_error().c_str());
        }
    }

    DriverLog("GazeMasterDriver::Activate() Activating plugins:");

    for (auto& i : plugin_adapters_candidates) {
        std::string name = i->GetNameAndVersion();
        bool plugin_res = i->Activate(
            {
                gaze::g_plugin_interface_version_major,
                gaze::g_plugin_interface_version_minor,
                gaze::g_plugin_interface_version_hotfix
            }
        );
        if (plugin_res) {
            DriverLog("\t%s activated", name.c_str());
            m_plugin_adapters.push_back(std::move(i));
        }
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

    HoboVR_GazeState_t res = {
        packet->status,
        packet->age_seconds,
        pupil_position_r[0], pupil_position_r[1],
        pupil_position_l[0], pupil_position_l[1],
        pupil_dilation_r,
        pupil_dilation_l,
        eye_close_r,
        eye_close_l
    };

    for (auto& i : m_plugin_adapters)
        i->ProcessData(res);

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
