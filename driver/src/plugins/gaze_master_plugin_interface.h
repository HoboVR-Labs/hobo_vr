// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2022 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_GAZE_PLUGINS
#define __HOBO_VR_GAZE_PLUGINS

#include "packets.h"
#include <string>

namespace gaze {

class PluginBase {
public:
	virtual ~PluginBase() = default;

	virtual bool Activate() = 0;

	virtual void ProcessData(const HoboVR_GazeState_t& data) = 0;

	virtual std::string GetNameAndVersion() = 0;
	virtual std::string GetLastError() = 0;
};

}

#endif // #ifndef __HOBO_VR_GAZE_PLUGINS