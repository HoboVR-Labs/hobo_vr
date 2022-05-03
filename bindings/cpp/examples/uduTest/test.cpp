// #include <string>
#include <string.h>

#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <lazy_sockets.h>
// #include <unistd.h>

#include <thread>
#include <cmath>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

// #include <sys/select.h>

#include "packets.h"
#include "ref/hobovr_defines.h"
#include "timer.h"

using tcp_socket = lsc::LSocket<AF_INET, SOCK_STREAM, 0>;

//////////////////////////////////////////////////////////////
// user types
//////////////////////////////////////////////////////////////

struct hmd_and_controllers_t {
	HoboVR_HeadsetPose_t hmd;
	HoboVR_ControllerState_t ctrl_l;
	HoboVR_ControllerState_t ctrl_r;
	PacketEndTag tag = g_EndTag;
};

struct controllers_and_trackers_t {
	HoboVR_ControllerState_t ctrl_l;
	HoboVR_ControllerState_t ctrl_r;
	HoboVR_TrackerPose_t trkr1;
	HoboVR_TrackerPose_t trkr2;
	HoboVR_TrackerPose_t trkr3;
	HoboVR_TrackerPose_t trkr4;
	PacketEndTag tag = g_EndTag;
};

struct hmd_and_everything_else_t {
	HoboVR_HeadsetPose_t hmd;
	HoboVR_ControllerState_t ctrl_l;
	HoboVR_ControllerState_t ctrl_r;
	HoboVR_TrackerPose_t trkr1;
	HoboVR_TrackerPose_t trkr2;
	HoboVR_TrackerPose_t trkr3;
	HoboVR_TrackerPose_t trkr4;
	HoboVR_GazeState_t gaze_master;
	PacketEndTag tag = g_EndTag;
};

struct everything_else_t {
	HoboVR_ControllerState_t ctrl_l;
	HoboVR_ControllerState_t ctrl_r;
	HoboVR_TrackerPose_t trkr1;
	HoboVR_TrackerPose_t trkr2;
	HoboVR_TrackerPose_t trkr3;
	HoboVR_TrackerPose_t trkr4;
	HoboVR_GazeState_t gaze_master;
	PacketEndTag tag = g_EndTag;
};

struct hmd_and_gaze_master_t {
	HoboVR_HeadsetPose_t hmd;
	HoboVR_GazeState_t gaze_master;
	PacketEndTag tag = g_EndTag;
};

struct hmd_and_trackers_t {
	HoboVR_HeadsetPose_t hmd;
	HoboVR_TrackerPose_t trkr1;
	HoboVR_TrackerPose_t trkr2;
	HoboVR_TrackerPose_t trkr3;
	PacketEndTag tag = g_EndTag;
};

struct gaze_master_t {
	HoboVR_GazeState_t gaze_master;
	PacketEndTag tag = g_EndTag;
};

struct trackers_t {
	HoboVR_TrackerPose_t trkr1;
	HoboVR_TrackerPose_t trkr2;
	HoboVR_TrackerPose_t trkr3;
	PacketEndTag tag = g_EndTag;
};

enum ERuntype: uint8_t {
	EHMD_AND_CNTRLS = 0, // hmd_and_controllers_t
	EHMD_AND_TRKRS = 1, // hmd_and_trackers_t
	EHMD_AND_GAZE_MASTER = 2, // hmd_and_gaze_master_t
	EHMD_AND_EVERYTHING_ELSE = 3, // hmd_and_everything_else_t
	ECNTRLS_AND_TRKRS = 4, // controllers_and_trackers_t
	ETRACKERS = 5, // trackers_t
	EGAZE_MASTER = 6, // gaze_master_t
	EEVERYTHING_ELSE = 7, // everything_else_t

	ERuntype_max

};

std::string getString(ERuntype mode) {
	switch(mode) {
	case EHMD_AND_CNTRLS:
		return "EHMD_AND_CNTRLS";
	case ECNTRLS_AND_TRKRS:
		return "ECNTRLS_AND_TRKRS";
	case EHMD_AND_EVERYTHING_ELSE:
		return "EHMD_AND_EVERYTHING_ELSE";
	case EEVERYTHING_ELSE:
		return "EEVERYTHING_ELSE";
	case EHMD_AND_GAZE_MASTER:
		return "EHMD_AND_GAZE_MASTER";
	case EGAZE_MASTER:
		return "EGAZE_MASTER";
	case EHMD_AND_TRKRS:
		return "EHMD_AND_TRKRS";
	case ETRACKERS:
		return "ETRACKERS";
	default:
		if (mode >= ERuntype_max)
			return "INVALID_MODE";
	}

	return "FAILED SWITCH";
}

//////////////////////////////////////////////////////////////
// poser class 
//////////////////////////////////////////////////////////////

class Poser {

std::mutex m_mutex;
std::condition_variable m_block_main;
std::atomic<bool> m_keep_blocking{true};

// sockets
std::unique_ptr<tcp_socket> tracking_sock;
std::unique_ptr<tcp_socket> manager_sock;

std::unique_ptr<hobovr::Timer> timer;

uint8_t m_mode;

float time;

// device packets
hmd_and_controllers_t m_pkt1;
hmd_and_trackers_t m_pkt2;
hmd_and_gaze_master_t m_pkt3;
hmd_and_everything_else_t m_pkt4;
controllers_and_trackers_t m_pkt5;
trackers_t m_pkt6;
gaze_master_t m_pkt7;
everything_else_t m_pkt8;

public:
////////////////////////////////////////////////////////////////////
// poser's constructor
////////////////////////////////////////////////////////////////////

Poser(ERuntype mode): m_mode(mode) {
	timer = std::make_unique<hobovr::Timer>();
}

////////////////////////////////////////////////////////////////////
// starts the poser
////////////////////////////////////////////////////////////////////

int start() {
	int res = init_sockets();
	if (res) return res;

	// res = setup_device_list();
	// if (res) return res;

	timer->registerTimer(
		std::chrono::milliseconds(16),
		std::bind(&Poser::send_packet, this)
	);

	return 0;
}

////////////////////////////////////////////////////////////////////
// udu device list setup
////////////////////////////////////////////////////////////////////

int setup_device_list() {

	HoboVR_ManagerMsgUduString_t udu_descriptor;
	switch(m_mode) {
	case EHMD_AND_CNTRLS:
		udu_descriptor.len = 3;
		udu_descriptor.devices[0] = EDeviceType_headset;
		udu_descriptor.devices[1] = EDeviceType_controller;
		udu_descriptor.devices[2] = EDeviceType_controller;
		break;
	case ECNTRLS_AND_TRKRS:
		udu_descriptor.len = 6;
		udu_descriptor.devices[0] = EDeviceType_controller;
		udu_descriptor.devices[1] = EDeviceType_controller;
		udu_descriptor.devices[2] = EDeviceType_tracker;
		udu_descriptor.devices[3] = EDeviceType_tracker;
		udu_descriptor.devices[4] = EDeviceType_tracker;
		udu_descriptor.devices[5] = EDeviceType_tracker;
		break;
	case EHMD_AND_EVERYTHING_ELSE:
		udu_descriptor.len = 8;
		udu_descriptor.devices[0] = EDeviceType_headset;
		udu_descriptor.devices[1] = EDeviceType_controller;
		udu_descriptor.devices[2] = EDeviceType_controller;
		udu_descriptor.devices[3] = EDeviceType_tracker;
		udu_descriptor.devices[4] = EDeviceType_tracker;
		udu_descriptor.devices[5] = EDeviceType_tracker;
		udu_descriptor.devices[6] = EDeviceType_tracker;
		udu_descriptor.devices[7] = EDeviceType_gazeMaster;
		break;
	case EEVERYTHING_ELSE:
		udu_descriptor.len = 7;
		udu_descriptor.devices[0] = EDeviceType_controller;
		udu_descriptor.devices[1] = EDeviceType_controller;
		udu_descriptor.devices[2] = EDeviceType_tracker;
		udu_descriptor.devices[3] = EDeviceType_tracker;
		udu_descriptor.devices[4] = EDeviceType_tracker;
		udu_descriptor.devices[5] = EDeviceType_tracker;
		udu_descriptor.devices[6] = EDeviceType_gazeMaster;
		break;
	case EHMD_AND_GAZE_MASTER:
		udu_descriptor.len = 2;
		udu_descriptor.devices[0] = EDeviceType_headset;
		udu_descriptor.devices[1] = EDeviceType_gazeMaster;
		break;
	case EGAZE_MASTER:
		udu_descriptor.len = 1;
		udu_descriptor.devices[0] = EDeviceType_gazeMaster;
		break;
	case EHMD_AND_TRKRS:
		udu_descriptor.len = 4;
		udu_descriptor.devices[0] = EDeviceType_headset;
		udu_descriptor.devices[1] = EDeviceType_tracker;
		udu_descriptor.devices[2] = EDeviceType_tracker;
		udu_descriptor.devices[3] = EDeviceType_tracker;
		break;
	case ETRACKERS:
		udu_descriptor.len = 3;
		udu_descriptor.devices[0] = EDeviceType_tracker;
		udu_descriptor.devices[1] = EDeviceType_tracker;
		udu_descriptor.devices[2] = EDeviceType_tracker;
		break;
	default:
		printf("invalid mode: %d\n", (int)m_mode);
		return -EINVAL;
	}

	printf("setting up devices for %s...\n", getString((ERuntype)m_mode).c_str());

	HoboVR_ManagerMsg_t msg{
		EManagerMsgType_uduString,
		(HoboVR_ManagerData_t&)udu_descriptor,
		g_EndTag
	};

	int res = manager_sock->Send(&msg, sizeof(msg));
	if (res != sizeof(msg)) {
		printf("failed to send udu descriptor: %d\n", lerrno);
		return -ECOMM;
	}

	char buff[256];

	res = manager_sock->Recv(buff, sizeof(HoboVR_ManagerResp_t));
	if (res != sizeof(HoboVR_ManagerResp_t)) {
		printf("failed to receive manager response: %d,%d\n", res, lerrno);
		if (res <= 0)
			return -ECONNRESET;
		else
			return -EBADMSG;
	}

	uint32_t* status_code = (uint32_t*)buff;

	if (*status_code != EManagerResp_ok) {
		printf("failed to update udu state: %d", (int)(*status_code));
		return -EBADMSG;
	}

	printf("finished setting up devices for %s\n", getString((ERuntype)m_mode).c_str());

	return 0;
}

////////////////////////////////////////////////////////////////////
// driver communication initialization
////////////////////////////////////////////////////////////////////

int init_sockets() {
	// main poser logic
	tcp_socket binder;

	int res = binder.Bind("0.0.0.0", 6969);
	if (res) return -lerrno;

	printf("waiting for driver to connect...\n");

	res = binder.Listen(2);
	if (res) return -lerrno;


	// accept sockets
	lsc::lsocket_t sockA = binder.Accept();
	if (sockA == lsc::EAccept_error) return -lerrno;

	lsc::lsocket_t sockB = binder.Accept();
	if (sockB == lsc::EAccept_error) return -lerrno;

	// id sockets
	char buffA[32];
	char buffB[32];

	printf("resolving socket IDs\n");
	printf("sockets: A:%d B:%d\n", (int)sockA, (int)sockB);

	res = recv(sockA, buffA, sizeof(KHoboVR_TrackingIdMessage), 0);
	if (res < 0) return -lerrno;
	res = recv(sockB, buffB, sizeof(KHoboVR_ManagerIdMessage), 0);
	if (res < 0) return -lerrno;

	lsc::lsocket_t tracking_fd, manager_fd;

	if (
		!memcmp(buffA, KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage)) &&
		!memcmp(buffB, KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage))
	) {
		tracking_fd = sockA;
		manager_fd = sockB;
	} else if (
		!memcmp(buffB, KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage)) &&
		!memcmp(buffA, KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage))
	) {
		tracking_fd = sockB;
		manager_fd = sockA;
	} else {
		printf("buffA: '%s' == '%s' => %d,%d\n",
			buffA,
			KHoboVR_TrackingIdMessage,
			memcmp(buffA, KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage)),
			memcmp(buffA, KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage))
		);
		printf("buffB: '%s' == '%s' => %d,%d\n",
			buffB,
			KHoboVR_ManagerIdMessage,
			memcmp(buffB, KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage)),
			memcmp(buffB, KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage))
		);
		printf("failed to ID sockets\n");
		return -EBADMSG;
	}

	// init lazy_socket objects
	tracking_sock = std::make_unique<tcp_socket>(tracking_fd, lsc::EStat_connected);
	manager_sock = std::make_unique<tcp_socket>(manager_fd, lsc::EStat_connected);
	printf("sockets: tracking:%d manager:%d\n", (int)tracking_sock->GetHandle(), (int)manager_sock->GetHandle());

	return 0;
}

////////////////////////////////////////////////////////////////////
// method to block the main thread while the poser is running
////////////////////////////////////////////////////////////////////

void block() {
	std::unique_lock lk(m_mutex);
	m_block_main.wait(lk, [this](){return !is_alive();});
}

////////////////////////////////////////////////////////////////////
// or a check for poser status
////////////////////////////////////////////////////////////////////

bool is_alive() const {
	return m_keep_blocking.load();
}

////////////////////////////////////////////////////////////////////
// and of course you can stop the poser if you need to
////////////////////////////////////////////////////////////////////

void stop() {
	m_keep_blocking.store(false);
	m_block_main.notify_one();
}

////////////////////////////////////////////////////////////////////
// send packet callback
////////////////////////////////////////////////////////////////////

void send_packet() {
	char buff[256];

	switch(m_mode) {
	////////////////////////////////////////////////////
	case EHMD_AND_CNTRLS: {
		hmd_and_controllers_t pose;
		pose.hmd.position[0] = sin(time);
		pose.hmd.orientation[0] = 1;

		int res = tracking_sock->Send(&pose, sizeof(pose));

		res = tracking_sock->Recv(buff, sizeof(HoboVR_PoserResp_t), lsc::ERecv_nowait);
		if (res < 0) {
			if (lerrno != EAGAIN && lerrno != EWOULDBLOCK) {
				printf("failed to receive driver response: %d\n", lerrno);
				std::this_thread::sleep_for(
					std::chrono::milliseconds(5)
				);
				stop();
			}
		}

		if (res == sizeof(HoboVR_PoserResp_t)) {
			process_tracking_response((HoboVR_PoserResp_t*)buff);
		}

		printf("sent %ld\n", sizeof(pose));


		break;
	} // case EHMD_AND_CNTRLS

	////////////////////////////////////////////////////
	case EHMD_AND_TRKRS: {
		hmd_and_trackers_t pose;
		pose.hmd.position[0] = sin(time);
		pose.hmd.orientation[0] = 1;

		int res = tracking_sock->Send(&pose, sizeof(pose));

		res = tracking_sock->Recv(buff, sizeof(HoboVR_PoserResp_t), lsc::ERecv_nowait);
		if (res < 0) {
			if (lerrno != EAGAIN && lerrno != EWOULDBLOCK) {
				printf("failed to receive driver response: %d\n", lerrno);
				std::this_thread::sleep_for(
					std::chrono::milliseconds(5)
				);
				stop();
			}
		}

		if (res == sizeof(HoboVR_PoserResp_t)) {
			process_tracking_response((HoboVR_PoserResp_t*)buff);
		}

		printf("sent %ld\n", sizeof(pose));

		break;

	} // case EHMD_AND_TRKRS

	////////////////////////////////////////////////////

	default:
		printf("mode not supported... yet...\n");
		stop();
	}

	// printf("packet processing end\n");

	time += 0.016;
}

////////////////////////////////////////////////////////////////////
// poser response processing
////////////////////////////////////////////////////////////////////

void process_tracking_response(HoboVR_PoserResp_t* data) {
	printf("response processing start\n");

	switch (data->type) {
	case EPoserRespType_badDeviceList: {
		int res = setup_device_list();
		if (res) {
			printf("failed to setup devices: %d\n", res);
			stop();
		}

		break;
	}

	case EPoserRespType_haptics:
		printf("haptics:\n");

		printf("\tname: %s\n",
			data->data.haptics.name
		);
		printf("\tduration: %f\n",
			data->data.haptics.duration_seconds
		);
		printf("\tfrequency: %f\n",
			data->data.haptics.frequency
		);
		printf("\tfrequency: %f\n",
			data->data.haptics.amplitude
		);

		break;

	case EPoserRespType_driverShutdown:
		printf("driver is shutting down...\n");
		stop();
		break;

	default:
		printf("bad response type\n");
	}
	printf("response processing end\n");
}

};

////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////

int main() {

	printf(
		"hobovr version: %d.%d.%d %s\n",
		hobovr::k_nHobovrVersionMajor,
		hobovr::k_nHobovrVersionMinor,
		hobovr::k_nHobovrVersionBuild,
		hobovr::k_sHobovrVersionGG.c_str()
	);

	uint8_t mode;
	printf("choose mode:\n");
	for (uint8_t i=0; i != ERuntype_max; ++i)
		printf("\t%s = %d\n", getString((ERuntype)i).c_str(), (int)i);

	std::cin >> (int&)mode;

	if (mode >= ERuntype_max) {
		printf("bad mode: %d\n", mode);
		return -EINVAL;
	}

	Poser poser((ERuntype)mode);

	int res = poser.start();
	if (res) return res;

	printf("started event loop\n");

	poser.block();

	printf("thank you for using hobovr airlines, goodbye\n");

	return 0;
}
