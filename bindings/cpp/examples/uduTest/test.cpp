// #include <string>
#include <string.h>

#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#include <lazy_sockets.h>
#include <unistd.h>

#include <errno.h>
#include <thread>
#include <cmath>
#include <memory>
#include <mutex>

#include <sys/select.h>

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
	// 7,
	6,
	// EDeviceType_headset,
	EDeviceType_controller,
	EDeviceType_controller,
	EDeviceType_tracker,
	EDeviceType_tracker,
	EDeviceType_tracker,
	EDeviceType_gazeMaster,
};

static HoboVR_ManagerMsg_t device_list1 = {
	EManagerMsgType_uduString,
	(HoboVR_ManagerData_t&)data1,
	my_tag
};

static std::mutex io_mutex;

void manager_thread(tcp_socket* soc, int* other, int* other2, bool* alive, bool* udu) {

	char buff[1024];
	int res;

	res = soc->Accept();
	if (res < 0) {
		printf("manager failed to accept: %d\n", errno);
		return;
	}

	int soc_fd = res;

	res = recv(res, buff, sizeof(KHoboVR_ManagerIdMessage), 0);

	int manager_fd;
	
	{
		std::lock_guard<std::mutex> lk(io_mutex); // lock others

		if (
			memcmp(buff, KHoboVR_ManagerIdMessage, sizeof(KHoboVR_ManagerIdMessage)) == 0
		) {
			manager_fd = soc_fd;
		} else {
			manager_fd = -1;
			*other2 = soc_fd;
		}
	}


	std::this_thread::sleep_for(
		std::chrono::milliseconds(1)
	);

	if (manager_fd == -1) {
		std::lock_guard<std::mutex> lk(io_mutex); // lock others
		manager_fd = *other;
	}

	printf("manager: socket fd: %d\n", manager_fd);

	// create a lazy socket for manager
	tcp_socket manager_sock(manager_fd, lsc::EStat_connected);

	printf("manager: trying to sync device list...\n");

	while (1) {
		res = manager_sock.Send(&device_list1, sizeof(device_list1));
		if (res != sizeof(device_list1))
			printf("manager: failed to send device list...\n");

		if (res < 0) {
			printf("manager: resp=%d errno=%d\n", res, errno);
			return; // we dead in this case
		}

		printf("manager: device list sent, waiting for response...\n");

		res = manager_sock.Recv(
			buff,
			sizeof(HoboVR_ManagerResp_t)
		);
		if (res == sizeof(HoboVR_ManagerResp_t))
			break;


		HoboVR_ManagerResp_t* data = (HoboVR_ManagerResp_t*)buff;
		printf("manager: response %d %d\n", (int)(data->status), res);

		if ((int)data->status == EManagerResp_ok) {
			printf("manager: finished syncing device lists\n");
			break;
		}
	}

	*udu = false;

	printf("manager: starting thread...\n");

	while (*alive) {
		res = manager_sock.Recv(buff, sizeof(buff), lsc::ERecv_nowait);

		if (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno) {
			break; // we dead in this case
		}

		if (*udu) {
			std::lock_guard<std::mutex> lk(io_mutex); // lock others
			*udu = false;

			printf("manager: udu shit\n");

			res = manager_sock.Send(&device_list1, sizeof(device_list1));
			if (res < 0) {
				break; // we dead in this case
			}

			res = manager_sock.Recv(buff, sizeof(HoboVR_ManagerResp_t));
			if (res < 0) break;

			HoboVR_ManagerResp_t* man_resp = (HoboVR_ManagerResp_t*)buff;
			printf("manager responded: %d\n", (int)man_resp->status);
		}

		std::this_thread::sleep_for(
			std::chrono::milliseconds(16)
		);
	}
	printf("manager: exit resp=%d errno=%d\n", res, errno);
}

int main()
{
	tcp_socket binder;

	int res = binder.Bind("0.0.0.0", 6969);
	if (res) return -errno;

	res = binder.Listen(2);
	if (res) return -errno;

	bool send_packet_sync = true;
	bool alive = true;

	int other;
	int other2;

	res = binder.Accept();
	if (res < 0) return -errno;

	int soc_fd = res;

	char buffA[32];
	res = recv(res, buffA, sizeof(KHoboVR_TrackingIdMessage), 0);

	int tracking_fd;


	{
		std::lock_guard<std::mutex> lk(io_mutex); // lock others

		if (
			memcmp(buffA, KHoboVR_TrackingIdMessage, sizeof(KHoboVR_TrackingIdMessage)) == 0
		) {
			tracking_fd = soc_fd;
		} else {
			tracking_fd = -1;
			other = soc_fd;
		}
	}

	// pass the binder to the manager thread
	std::thread man(manager_thread, &binder, &other, &other2, &alive, &send_packet_sync);

	if (tracking_fd == -1) {
		std::lock_guard<std::mutex> lk(io_mutex); // lock others
		tracking_fd = other2;
	}

	printf("tracking fd: %d\n", tracking_fd);


	tcp_socket tracking_sock(tracking_fd, lsc::EStat_connected);

	//////////////////////////////////////////////////////////////////////
	// now its time for runtime
	//////////////////////////////////////////////////////////////////////

	while (send_packet_sync)
		std::this_thread::sleep_for(
			std::chrono::seconds(1)
		);

	char recv_buffer[1024];

	printf("starting tracking loop...\n");
	printf("device list size: %lu\n", sizeof(my_tracking));

	int h = 0;

	while (1) {
		my_tracking my_packet;
		// memcpy(&my_packet.term[0], "\t\r\n", 3);
		my_packet.term = {'\t', '\r', '\n'};

		// my_packet.h1 = {
		// 	{0, 0, 0},
		// 	{1, 0, 0, 0},
		// 	{(float)(h % 10 / 10.0f), 0, 0},
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
			0.01,
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
		if (res < 0) {
			printf("resp=%d errno=%d\n", res, errno);
			break; // we dead in this case
		}

		res = tracking_sock.Recv(recv_buffer, sizeof(HoboVR_PoserResp_t), lsc::ERecv_nowait);
		if (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno) {
			printf("2 resp=%d errno=%d\n", res, errno);
			break; // we dead in this case
		}

		if (res > 0) {
			HoboVR_PoserResp_t* data = (HoboVR_PoserResp_t*)recv_buffer;
			// printf("poser response: %d %d\n", (int)data->type, res);
			if ((int)data->type == EPoserRespType_badDeviceList) {
				printf("poser reported device list size: %d, actual: %lu\n", data->data.buf_size.size, sizeof(my_tracking));
				std::lock_guard<std::mutex> lk(io_mutex); // lock others
				send_packet_sync = true;
			}
		}

		h++;

		std::this_thread::sleep_for(
			std::chrono::milliseconds(16)
		);
	}

	printf("3 resp=%d errno=%d\n", res, errno);

	alive = false;
	man.join();

	return 0;
}