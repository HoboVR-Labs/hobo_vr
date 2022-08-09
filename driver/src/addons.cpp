// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "addons.h"
#include "hobovr_defines.h"
#include "path_helpers.h"

std::string get_self_path() {
    return boost::dll::symbol_location_ptr(get_self_path).string();
}

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

    std::string driver_path = get_self_path();

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
        //DriverLog("\t%s -> %s", i.path().string().c_str(), hobovr::Path_GetExtension(i.path().string()).c_str());
        if (hobovr::Path_GetExtension(i.path().string()) == dl_exention && i.path().string() != driver_path)
            plugin_paths.push_back(i.path().string());
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
    std::string factory_symbol_name = "gazePluginFactory";

    int h = 0;

    for (auto& i : m_plugin_handles) {
        if (!i.is_loaded()) {
            DriverLog("\t%d failed to load plugin, skipping...", h);
            continue;
        }

        if (!i.has(factory_symbol_name)) {
            DriverLog("\t%d does not contain factory function, skipping...", h);
            continue;
        }

        std::function<gaze::PluginBase*()> factory_func;

        factory_func = i.get<gaze::PluginBase*()>(factory_symbol_name);

        if (factory_func == nullptr) {
            DriverLog("\t%d failed to obtain factory function ptr", h);
            continue;
        }

        std::unique_ptr<gaze::PluginBase> plugin(factory_func());

        if (!plugin) {
            DriverLog("\t%d failed to construct plugin", h);
            continue;
        }

        DriverLog("\tcreated plugin %d at %p",
            h, plugin.get());

        plugin_adapters_candidates.push_back(std::move(plugin));

        h++;
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
            DriverLog("\t'%s' activated", name.c_str());
            m_plugin_adapters.push_back(std::move(i));
        } else {
            DriverLog("\t'%s' failed to activate, reason: '%s'",
                name.c_str(), i->GetLastError().c_str());
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
