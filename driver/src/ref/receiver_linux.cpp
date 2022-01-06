// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "receiver.h"

#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "driverlog.h"

#include "packets.h"


namespace recvv {

////////////////////////////////////////////////////////////////////////////////
// receiver class
////////////////////////////////////////////////////////////////////////////////

DriverReceiver::DriverReceiver(
	size_t struct_size,
	int port,
	std::string addr,
	std::string id_message,
	Callback* callback
): sIdMessage(id_message), pCallback(callback) {

	_buff_size = struct_size;
	_device_list.clear();

	// placeholders for filling in seckaddr_in
	struct sockaddr_in serv_addr;
	struct hostent *server;

	// create socket
	fdSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fdSocket < 0) {
		throw std::runtime_error(
			"DriverReceiver failed to create socket: errno=" +
			std::to_string(errno) +
			" addr=" +
			addr +
			":" +
			std::to_string(port) +
			" id=" +
			sIdMessage +
			"socket_obj=" +
			std::to_string(fdSocket)
		);
	}

	// filling out the connection info
	memset(&serv_addr, 0, sizeof(serv_addr));

	server = gethostbyname(addr.c_str());

	memmove(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	// try to connect
	int res = connect(
		fdSocket,
		(struct sockaddr *)&serv_addr,
		sizeof(struct sockaddr)
	);


	if (res < 0) {
		throw std::runtime_error(
			"DriverReceiver failed to connect: errno=" +
			std::to_string(errno) +
			" addr=" +
			addr +
			":" +
			std::to_string(port) +
			" id=" +
			sIdMessage
		);
	}
}

// overload for processing udu strings in the constructor

DriverReceiver::DriverReceiver(
	std::string udu_string,
	int port,
	std::string addr,
	std::string id_message,
	Callback* callback
): sIdMessage(id_message), pCallback(callback) {

	// // this is wasteful, too bad!
	// UpdateParams(udu_string);

	// DriverLog("uhhhhhhhhhh %s %d", udu_string.c_str(), _buff_size);

	// // call the main constructor
	// DriverReceiver(_buff_size, port, addr, id_message, callback);

	UpdateParams(udu_string); // refill device list vector
	bReset = false;

	// placeholders for filling in seckaddr_in
	struct sockaddr_in serv_addr;
	struct hostent *server;

	// create socket
	fdSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fdSocket < 0) {
		throw std::runtime_error(
			"DriverReceiver failed to create socket: errno=" +
			std::to_string(errno) +
			" addr=" +
			addr +
			":" +
			std::to_string(port) +
			" id=" +
			sIdMessage +
			"socket_obj=" +
			std::to_string(fdSocket)
		);
	}

	// filling out the connection info
	memset(&serv_addr, 0, sizeof(serv_addr));

	server = gethostbyname(addr.c_str());

	memmove(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);

	// try to connect
	int res = connect(
		fdSocket,
		(struct sockaddr *)&serv_addr,
		sizeof(struct sockaddr)
	);


	if (res < 0) {
		throw std::runtime_error(
			"DriverReceiver failed to connect: errno=" +
			std::to_string(errno) +
			" addr=" +
			addr +
			":" +
			std::to_string(port) +
			" id=" +
			sIdMessage
		);
	}
}

////////////////////////////////////////////////////////////////////////////////
// die safely
////////////////////////////////////////////////////////////////////////////////

DriverReceiver::~DriverReceiver() {
	Stop();
}

////////////////////////////////////////////////////////////////////////////////
// start the receiving thread and all
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::Start() {
	if (bAlive)
		return; // do nothing if thread is already running

	bAlive = true;
	Send(sIdMessage);

	pThread = std::make_shared<std::thread>(thread_enter, this);

	if (!pThread || !bAlive) {
		close_internal();

		throw std::runtime_error("DriverReceiver failed to start receiver thread");
	}
}

////////////////////////////////////////////////////////////////////////////////
// stop the receiving thread
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::Stop() {
	DriverLog("DriverReceiver(%s) stop called, exiting...", sIdMessage.c_str());

	bAlive = false;
	close_internal();

	if (pThread){
		pThread->join();
	}
}

////////////////////////////////////////////////////////////////////////////////
// fetch the device list
////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> DriverReceiver::GetDevices() {
	return _device_list;
}

////////////////////////////////////////////////////////////////////////////////
// fetch the expected receive buffer size,
// could differ from the actual size received
////////////////////////////////////////////////////////////////////////////////

size_t DriverReceiver::GetBufferSize() {
	return _buff_size;
}

////////////////////////////////////////////////////////////////////////////////
// send buffer
////////////////////////////////////////////////////////////////////////////////

int DriverReceiver::Send(const void* buff, size_t size, int flags) {
	return send(fdSocket, buff, size, flags);
}

////////////////////////////////////////////////////////////////////////////////
// send string as buffer
////////////////////////////////////////////////////////////////////////////////

int DriverReceiver::Send(const std::string message) {
	return Send(message.c_str(), message.size()); // wrap string as a buffer :P
}

////////////////////////////////////////////////////////////////////////////////
// set set a new callback
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::setCallback(Callback* pCb) {
	pCallback = pCb;
}

////////////////////////////////////////////////////////////////////////////////
// parses a new device list and update thread parameters
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::UpdateParams(std::string new_udu_string) {
	std::regex rgx("[htcg]");

	_device_list = get_rgx_vector(new_udu_string, rgx);
	size_t temp = 0;

	for (std::string i : _device_list) {
		if (i == "h" || i == "t") {
			temp += sizeof(HoboVR_HeadsetPose_t);
		} else if (i == "c") {
			temp += sizeof(HoboVR_ControllerState_t);
		} else if (i == "g") {
			temp += sizeof(HoboVR_GazeState_t);
		}
	}

	_buff_size = temp;

	bReset = true;
}

////////////////////////////////////////////////////////////////////////////////
// internal close socket call
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::close_internal() {
	if (fdSocket) {
		close(fdSocket);
	}
}

////////////////////////////////////////////////////////////////////////////////
// main receiver thread
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::thread() {
	int numbit = 0, msglen;
	// purposefully allocate a bigger buffer then needed
	int tempMsgSize = _buff_size * 10;
	void* recv_buffer = malloc(tempMsgSize);
	memset(recv_buffer, 0, tempMsgSize);

	while (bAlive) {
		// realloc early if we can
		if (bReset) {
			DriverLog("DriverReceiver(%s) resizing receive buffer...", sIdMessage.c_str());
			numbit = 0; // zero this off
			tempMsgSize = _buff_size * 10; // recalculate buffer size
			recv_buffer = realloc(recv_buffer, tempMsgSize); // realloc buffer
			memset(recv_buffer, 0, tempMsgSize);
			bReset = false;
		}

		msglen = receive_till_zero(
			fdSocket,
			recv_buffer,
			numbit,
			tempMsgSize
		);

		if (msglen == -1) {
			DriverLog("DriverReceiver(%s) bad message, exiting...", sIdMessage.c_str());
			break; // broken socket
		}

		if (!bReset && pCallback != nullptr)
			pCallback->OnPacket((char*)recv_buffer, msglen); // callback if we can


		std::this_thread::sleep_for(
			std::chrono::nanoseconds(1)
		);

	}
}


} // namespace recvv