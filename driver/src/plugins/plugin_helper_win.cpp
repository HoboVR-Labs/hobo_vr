// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2022 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "plugin_helper.h"

#include <windows.h>
#include <libloaderapi.h>

namespace hobovr {


DLWrapper::DLWrapper(const std::string& fname, int flags): m_flags(flags) {
	m_handle = LoadLibraryExA(fname.c_str(), NULL, m_flags);
}

DLWrapper::~DLWrapper() {
	if (is_alive())
		FreeLibrary(reinterpret_cast<HMODULE>(m_handle));
}


std::string DLWrapper::get_last_error() {
	wchar_t *s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
				NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPWSTR)&s, 0, NULL);
	std::wstring ws(s);
	
	//disabling loss of data warning. Conversion from unicode to ascii will always lose data.
	#pragma warning( push )
	#pragma warning( disable : 4244)
	std::string str(ws.begin(), ws.end());
	#pragma warning( pop ) 

	return str;
}

void* DLWrapper::get_symbol(const std::string& symbol) {
	if (is_alive())
		return GetProcAddress(reinterpret_cast<HMODULE>(m_handle), symbol.c_str());

	return nullptr;
}

std::string DLWrapper::get_symbol_path(void* addr) {
	HMODULE hmodule = NULL;

	GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(addr), &hmodule );

	char *pwchPath = new char[512];
	int res = GetModuleFileNameA( hmodule, pwchPath, 512 );

	std::string str(pwchPath);

	delete[] pwchPath;

	if (res)
		return str;

	return "";
}


}