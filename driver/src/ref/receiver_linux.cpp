// SPDX-License-Identifier: GPL-2.0-only

// Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

#include "receiver.h"

#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

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
    memset(&serv_addr, 0, sizeof(serv_addr));

    server = gethostbyname(addr.c_str());

    memcpy(&serv_addr, server, server->h_length);
    serv_addr.sin_port = htons(port);

    // create socket
    fdSocket = socket(AF_INET, SOCK_STREAM, 0);

    // try to connect
    int res = connect(fdSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));


	if (res < 0) {
		#ifdef DRIVERLOG_H
		DriverLog(
			"DriverReceiver(%s) failed to connect: %d",
			sIdMessage.c_str(),
			errno
		);
		#endif
		throw std::runtime_error("failed to connect: " + std::to_string(errno));
	}

	#ifdef DRIVERLOG_H
	DriverLog(
		"DriverReceiver(%s) receiver failed to connect to host",
		sIdMessage.c_str()
	);
	#endif
}

// overload for processing udu strings in the constructor

DriverReceiver::DriverReceiver(
	std::string udu_string,
	int port,
	std::string addr,
	std::string id_message,
	Callback* callback
) {
	// this is wasteful, too bad!
	UpdateParams(udu_string);

	// call the main constructor
	DriverReceiver(_buff_size, port, addr, id_message, callback);

	UpdateParams(udu_string); // refill device list vector

	bReset = false;
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
		#ifdef DRIVERLOG_H
		DriverLog(
			"DriverReceiver(%s) failed to start thread",
			sIdMessage.c_str()
		);
		#endif
		close_internal();

        throw std::runtime_error("failed to start receiver thread");
	}
}

////////////////////////////////////////////////////////////////////////////////
// stop the receiving thread
////////////////////////////////////////////////////////////////////////////////

void DriverReceiver::Stop() {
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

int DriverReceiver::Send(const void* buff, size_t size) {
	return send(fdSocket, buff, size, 0);
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

	#ifdef DRIVERLOG_H
	DriverLog("DriverReceiver(%s) thread started", sIdMessage.c_str());
	#endif

	while (bAlive) {

		msglen = receive_till_zero(
			fdSocket,
			(char*)recv_buffer,
			numbit,
			tempMsgSize
		);

		if (msglen == -1)
			break; // broken socket

		if (!bReset && pCallback != nullptr)
			pCallback->OnPacket((char*)recv_buffer, msglen); // callback if we can

		if (bReset) {
			numbit = 0; // zero this off
			tempMsgSize = _buff_size * 10; // recalculate buffer size
			recv_buffer = realloc(recv_buffer, tempMsgSize); // realloc buffer
			bReset = false;
		}

	}

	#ifdef DRIVERLOG_H
	DriverLog("DriverReceiver(%s) thread stopped", sIdMessage.c_str());
	#endif
}


} // namespace recvv