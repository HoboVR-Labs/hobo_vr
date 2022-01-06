// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#pragma once

#ifndef RECEIVER_H
#define RECEIVER_H

#include <vector>
#include <thread>
#include <util.h>

namespace recvv {

	class DriverReceiver {
	public:
		DriverReceiver(
			std::string udu_string,
			int port=6969,
			std::string addr="127.0.01",
			std::string id_message="hello\n",
			Callback* callback=nullptr
		);

		DriverReceiver(
			size_t struct_size,
			int port=6969,
			std::string addr="127.0.01",
			std::string id_message="hello\n",
			Callback* callback=nullptr
		);

		~DriverReceiver();

		void Start();
		void Stop();

		std::vector<std::string> GetDevices();
		size_t GetBufferSize();

		int Send(const void* buff, size_t size, int flags = 0);
		int Send(const std::string message);

		void setCallback(Callback* pCb);

		// parses a new device list basted on the 
		void UpdateParams(std::string new_udu_string);

	private:
		size_t _buff_size = 0;
		std::vector<std::string> _device_list;
		std::string sIdMessage;

		bool bAlive = false;
		bool bReset = false;
		std::shared_ptr<std::thread> pThread = nullptr;

		Callback* pCallback = nullptr;

		#ifdef SOCKET
		SOCKET fdSocket;

		#else
		int fdSocket;

		#endif // #ifdef SOCKET

		void close_internal();

		inline static void thread_enter(DriverReceiver *ptr) {
			ptr->thread();
		}

		void thread();

	}; // class DriverReceiver

} // namespace recvv
#undef SOCKET


#endif // #ifndef RECEIVER_H
