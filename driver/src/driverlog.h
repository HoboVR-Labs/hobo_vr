// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020 Josh Miklos <josh.miklos@hobovrlabs.org>

#ifndef DRIVERLOG_H
#define DRIVERLOG_H

#pragma once

#include <string>
#include "openvr_driver.h"

extern void DriverLog( const char *pchFormat, ... );


// --------------------------------------------------------------------------
// Purpose: Write to the log file only in debug builds
// --------------------------------------------------------------------------
extern void DebugDriverLog( const char *pchFormat, ... );


extern bool InitDriverLog( vr::IVRDriverLog *pDriverLog );
extern void CleanupDriverLog();



#endif // DRIVERLOG_H