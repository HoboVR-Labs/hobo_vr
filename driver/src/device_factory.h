// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_DEVICE_FACTORY
#define __HOBO_VR_DEVICE_FACTORY

#include <memory>

#include <lazy_sockets.h>

#include "hobovr_device_base.h"

namespace hobovr {
	std::unique_ptr<IHobovrDevice> createDeviceFactory(
		char type,
		int number,
		std::shared_ptr<tcp_socket> soc
	);
} // namespace hobovr

#endif // #ifndef __HOBO_VR_DEVICE_FACTORY