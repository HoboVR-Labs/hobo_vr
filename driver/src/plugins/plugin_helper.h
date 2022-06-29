// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2022 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#ifndef __HOBO_VR_PLUGIN_HELPER
#define __HOBO_VR_PLUGIN_HELPER

#include <string>

namespace hobovr {

class DLWrapper {
public:
	DLWrapper(const std::string& fname, int flags=0);
	~DLWrapper();

	std::string get_last_error();
	inline bool is_alive() const {
		return m_handle != nullptr;
	}

	void* get_symbol(const std::string& symbol);
	static std::string get_symbol_path(void* addr);

private:
	int m_flags;
	void* m_handle = nullptr;
};

}

#endif // #ifndef __HOBO_VR_PLUGIN_HELPER