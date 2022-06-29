// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2022 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_GAZE_PLUGINS
#define __HOBO_VR_GAZE_PLUGINS

#include "packets.h"
#include <string>

namespace gaze {

struct PluginVersion_t {
	uint8_t major;
	uint8_t minor;
	uint8_t hotfix;
};	

class PluginBase {
public:
	virtual ~PluginBase() = default;

	virtual bool Activate(const PluginVersion_t version) = 0;

	virtual void ProcessData(const HoboVR_GazeState_t& data) = 0;

	virtual std::string GetNameAndVersion() = 0;
	virtual std::string GetLastError() = 0;
};

static constexpr uint8_t g_plugin_interface_version_major = 0;
static constexpr uint8_t g_plugin_interface_version_minor = 1;
static constexpr uint8_t g_plugin_interface_version_hotfix = 0;

}

#endif // #ifndef __HOBO_VR_GAZE_PLUGINS