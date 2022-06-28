// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2022 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "plugin_helper.h"

#include <dlfcn.h>

namespace hobovr {


DLWrapper::DLWrapper(const std::string& fname, int flags): m_flags(flags) {
	m_handle = dlopen(fname.c_str(), RTLD_LAZY);
}

DLWrapper::~DLWrapper() {
	if (is_alive())
		dlclose(m_handle);
}


std::string DLWrapper::get_last_error() {
	return std::string(dlerror());
}

void* DLWrapper::get_symbol(const std::string& symbol) {
	if (is_alive())
		return dlsym(m_handle, symbol.c_str());

	return nullptr;
}

std::string DLWrapper::get_symbol_path(void* addr) {
	Dl_info info = {};
	int res = dladdr(addr, &info);
	if (res)
		return std::string(info.dli_fname);

	return "";
}


}