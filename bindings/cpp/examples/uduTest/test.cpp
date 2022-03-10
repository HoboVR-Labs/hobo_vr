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

// #include <sys/select.h>

#include "packets.h"

// my tracking packet
struct my_tracking {
	// HoboVR_HeadsetPose_t h1;
	HoboVR_ControllerState_t c1;
	HoboVR_ControllerState_t c2;
	HoboVR_TrackerPose_t t1;
	HoboVR_TrackerPose_t t2;
	HoboVR_TrackerPose_t t3;
	HoboVR_GazeState_t g1;

	PacketEndTag term;
};

struct my_tracking2 {
	HoboVR_ControllerState_t c1;
	HoboVR_TrackerPose_t t1;
	HoboVR_GazeState_t g1;

	PacketEndTag term;
};

using tcp_socket = lsc::LSocket<2, 1, 0>;

static constexpr PacketEndTag my_tag = {'\t', '\r', '\n'};

static HoboVR_ManagerMsgUduString_t data1 = {
	6,
	// EDeviceType_headset,
	EDeviceType_controller,
	EDeviceType_controller,
	EDeviceType_tracker,
	EDeviceType_tracker,
	EDeviceType_tracker,
	EDeviceType_gazeMaster
};

static HoboVR_ManagerMsg_t device_list1 = {EManagerMsgType_uduString, (HoboVR_ManagerData_t&)data1, my_tag};

int main()
{
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
	tcp_socket tracking_sock(tracking_fd, lsc::EStat_connected);
	tcp_socket manager_sock(manager_fd, lsc::EStat_connected);

	printf("starting tracking loop...\n");
	printf("device list size: %lu\n", (unsigned	long)sizeof(my_tracking));

	char recv_buffer[256];

	int h = 0;

	while (1) {
		my_tracking my_packet;
		my_packet.term = {'\t', '\r', '\n'};

		// my_packet.h1 = {
		// 	{0, (float)sin((float)h/10) * 2.0f, 0},
		// 	{1, 0, 0, 0},
		// 	{(float)(h % 50 / 10.0f), 0, 0},
		// 	{0, 0, 0}
		// };

		// provide "tracking" data
		my_packet.c1 = {
			{(float)sin((float)h/10) * 2.0f, 0, 0},
			{1, 0, 0, 0},
			{(float)(h % 50 / 10.0f), 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			0
		};

		my_packet.c2 = {
			{0, (float)sin((float)h/10) * 2.0f, 0},
			{1, 0, 0, 0},
			{(float)(h % 50 / 10.0f), 0, 0},
			{0, 0, 0},
			{0, 0, 0},
			0
		};

		my_packet.t1 = {
			{(float)sin((float)h/10) * 2.0f, 0, 0.5},
			{1, 0, 0, 0},
			{(float)(h % 50 / 10.0f), 0, 0},
			{0, 0, 0}
		};

		my_packet.t2 = {
			{0, (float)sin((float)h/10) * 2.0f, 0.5},
			{1, 0, 0, 0},
			{(float)(h % 50 / 10.0f), 0, 0},
			{0, 0, 0}
		};

		my_packet.t3 = {
			{0, 0, (float)sin((float)h/10) * 2.0f},
			{1, 0, 0, 0},
			{(float)(h % 50 / 10.0f), 0, 0},
			{0, 0, 0}
		};

		my_packet.g1 = {
			EGazeStatus_rightEyeLost,
			0.01f,
			{0, 0},
			{0, 0},
			{1, 0, 0, 0},
			{1, 0, 0, 0},
			0,
			0
		};

		// printf("packet: %d %f %f %f\n", h,
		// 	my_packet.c1.position[0],
		// 	my_packet.c1.position[1],
		// 	my_packet.c1.position[2]
		// );

		res = tracking_sock.Send(&my_packet, sizeof(my_packet));
		if (res <= 0) {
			printf("failed to send tracking packet...\n");
			break; // we dead in this case
		}

		res = tracking_sock.Recv(recv_buffer, sizeof(HoboVR_PoserResp_t), lsc::ERecv_nowait);
		if (res < 0 && lerrno != EAGAIN && lerrno != EWOULDBLOCK && lerrno) {
			printf("failled recv tracking response\n");
			break; // we dead in this case
		}

		if (res > 0) {
			HoboVR_PoserResp_t* data = (HoboVR_PoserResp_t*)recv_buffer;
			printf("poser response: %d %d\n", (int)data->type, res);
			if ((int)data->type == EPoserRespType_badDeviceList) {
				printf(
					"driver reported device packet size: %d, actual: %lu\n",
					data->data.buf_size.size,
					(unsigned long)sizeof(my_tracking)
				);
				
				// send device list

				for (;;) {

					res = manager_sock.Send(&device_list1, sizeof(device_list1));
					if (res != sizeof(device_list1))
						printf("failed to send device list...\n");

					res = manager_sock.Recv(recv_buffer, sizeof(HoboVR_ManagerResp_t));
					if (res <= 0) {
						printf("failed to recv manager response...\n");
						return -ENOMSG;
					}

					HoboVR_ManagerResp_t* manager_resp = (HoboVR_ManagerResp_t*)recv_buffer;
					printf("manager responded: %d\n", (int)manager_resp->status);

					if ((int)manager_resp->status == EManagerResp_ok)
						break; // everything ok, we can exit the loop
				}

				printf("everything ok, continuing tracking...\n");

			} else if ((int)data->type == EPoserRespType_driverShutdown) {
				printf("driver shutting down, we need to exit too\n");
				break;

			}

		}

		h++;

		std::this_thread::sleep_for(
			std::chrono::milliseconds(16)
		);
	}

	printf("resp=%d lerrno=%d\n", res, lerrno);


	return 0;
}