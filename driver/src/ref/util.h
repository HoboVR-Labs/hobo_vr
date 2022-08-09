// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#pragma once

#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <iterator>
#include <regex>
#include <string>
#include <sstream>

#include "packets.h"

namespace util {

	// get regex vector
	inline std::vector<std::string> get_rgx_vector(std::string ss, std::regex rgx) {
		std::sregex_token_iterator iter(ss.begin(), ss.end(), rgx, 0);
		std::sregex_token_iterator end;

		std::vector<std::string> out;
		for (; iter != end; ++iter)
			out.push_back((std::string)*iter);
		return out;
	}

	inline size_t udu2sizet(std::string udu_string) {
		size_t out = 0;
		for (auto i : udu_string) {
			switch(i) {
				case 'h': {out += sizeof(HoboVR_HeadsetPose_t); 		break;}
				case 'c': {out += sizeof(HoboVR_ControllerState_t); 	break;}
				case 't': {out += sizeof(HoboVR_TrackerPose_t); 		break;}
				case 'g': {out += sizeof(HoboVR_GazeState_t); 			break;}
			}
		}

		return out;
	}
}

#endif // UTIL_H