# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) 2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>

import struct
import socket
import time
import numpy as np
import pyrr

TERMINATOR = b'\n'
SEND_TERMINATOR = b'\t\r\n'
MANAGER_UDU_MSG_t = struct.Struct("130I")
POSE_t = struct.Struct("13f")
CONTOLLER_t = struct.Struct("22f")


# here we assume that no server was running and only our driver will connect
# we assume the role of a server and a poser at the same time

serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.bind(('', 6969))
serversocket.listen(2)  # driver connects with 2 sockets


#######################################################################
# now lets accept both of them and resolve

print("waiting for driver to connect...")

client_a = serversocket.accept()
client_b = serversocket.accept()


print("waiting for driver resolution...")

resp_a = client_a[0].recv(50)
resp_b = client_b[0].recv(50)

if TERMINATOR in resp_a:
    id_msg_a, resp_a = resp_a.split(TERMINATOR, 1)

if TERMINATOR in resp_b:
    id_msg_b, resp_b = resp_b.split(TERMINATOR, 1)


if id_msg_a == b"hello" and id_msg_b == b"monky":
    tracking_socket = client_a[0]
    manager_socket = client_b[0]

elif id_msg_b == b"hello" and id_msg_a == b"monky":
    tracking_socket = client_b[0]
    manager_socket = client_a[0]

else:
    print("bad connection")
    client_a[0].close()
    client_b[0].close()

    serversocket.close()

    exit()

input("press anything to start the test...")

#######################################################################
# now let the fun begin :D

# lets start with something easy and simple, hmd only motion

# tell the manager about current device setup
device_list = MANAGER_UDU_MSG_t.pack(
    20,  # HobovrManagerMsgType::Emsg_uduString
    1,   # only 1 device
    0, 13,  # device description
    *np.zeros((128 - 2), dtype=int)
)

manager_socket.sendall(device_list + SEND_TERMINATOR)

print("hmd only: literal movement...")

# now lets do some tracking :D

# first some lateral movement
for i in range(60 * 8):
    pose = POSE_t.pack(
        0, 0, np.sin(i / 180 * np.pi) * 3,
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0
    )
    # print(pose)
    tracking_socket.sendall(pose + SEND_TERMINATOR)

    time.sleep(1 / 60)

# time.sleep(1)
print("next move...")

for i in range(60 * 8):
    pose = POSE_t.pack(
        0, np.sin(i / 180 * np.pi) * 3, np.sin(i / 180 * np.pi) * 3,
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0
    )
    # print(pose)
    tracking_socket.sendall(pose + SEND_TERMINATOR)

    time.sleep(1 / 60)


print("next move...")

for i in range(60 * 8):
    pose = POSE_t.pack(
        np.sin(i / 180 * np.pi) * 3, np.cos(i / 180 * np.pi) * 3, 0,
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0
    )
    # print(pose)
    tracking_socket.sendall(pose + SEND_TERMINATOR)

    time.sleep(1 / 60)

print("moving onto orientation testing...")

for i in range(60 * 4):
    q = pyrr.Quaternion.from_y_rotation(i / 60)
    pose = POSE_t.pack(
        0, 0, 0,
        q[3], *q[:3],
        int(i < 10), 0, 0,
        0, 0, 0
    )

    tracking_socket.sendall(pose + SEND_TERMINATOR)

    time.sleep(1 / 60)

#######################################################################
# now lets add some controllers

# tell the manager about current device setup
device_list = MANAGER_UDU_MSG_t.pack(
    20,  # HobovrManagerMsgType::Emsg_uduString
    3,   # 3 devices - 1 hmd, 2 controllers
    0, 13,  # device description
    1, 22,  # device description
    1, 22,  # device description
    *np.zeros((128 - 2 * 3), dtype=int)
)

manager_socket.sendall(device_list + SEND_TERMINATOR)

print("hmd with controllers: literal movement...")

# now since we need to see wtf is going on, hmd will be at (0,0,0)

for i in range(60 * 8):
    controller_z = np.sin(i / 180 * np.pi) * 3
    right_pose = pose = CONTOLLER_t.pack(
        0.2, 0, controller_z,
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    left_pose = CONTOLLER_t.pack(
        -0.2, 0, controller_z,
        0, 0, 0, -1,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    hmd_pose = POSE_t.pack(
        int(i < 10), 0, 0,
        int(controller_z <= 0), 0, -int(controller_z > 0), 0,
        int(i < 10), 0, 0,
        0, 0, 0
    )

    tracking_socket.sendall(
        hmd_pose + right_pose + left_pose + SEND_TERMINATOR
    )

    time.sleep(1 / 60)

print("next move...")

for i in range(60 * 8):
    q = pyrr.Quaternion.from_y_rotation(i / 60 * np.pi)

    mm = pyrr.matrix33.create_from_quaternion(q)

    loc = np.array([0, -0.1, -1])

    loc = mm.dot(loc)

    right_pose = pose = CONTOLLER_t.pack(
        *loc * 2,
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    left_pose = CONTOLLER_t.pack(
        *loc,
        0, 0, 0, -1,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    hmd_pose = POSE_t.pack(
        int(i < 10), 0, 0,
        q[3], *q[:3],
        int(i < 10), 0, 0,
        0, 0, 0
    )

    tracking_socket.sendall(
        hmd_pose + right_pose + left_pose + SEND_TERMINATOR
    )

    time.sleep(1 / 60)

print("next move...")

for i in range(60 * 8):
    q = pyrr.Quaternion.from_y_rotation(i / 60 * np.pi)

    mm = pyrr.matrix33.create_from_quaternion(q)

    loc = np.array([0, -0.1, -1])

    loc = mm.dot(loc)

    right_pose = pose = CONTOLLER_t.pack(
        *loc * [2, 3, 2],
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    left_pose = CONTOLLER_t.pack(
        *loc,
        q[3], *q[:3],
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )

    hmd_pose = POSE_t.pack(
        int(i < 10), 0, 0,
        q[3], *q[:3],
        int(i < 10), 0, 0,
        0, 0, 0
    )

    tracking_socket.sendall(
        hmd_pose + right_pose + left_pose + SEND_TERMINATOR
    )

    time.sleep(1 / 60)

#######################################################################
# now lets add some trackers

# tell the manager about current device setup
device_list = MANAGER_UDU_MSG_t.pack(
    20,  # HobovrManagerMsgType::Emsg_uduString
    6,   # 6 devices - 1 hmd, 2 controllers, 3 trackers
    0, 13,  # device description
    1, 22,  # device description
    1, 22,  # device description
    2, 13,  # device description
    2, 13,  # device description
    2, 13,  # device description
    *np.zeros((128 - 2 * 6), dtype=int)
)

manager_socket.sendall(device_list + SEND_TERMINATOR)

print("hmd with controllers and trackers: orbit...")


for i in range(60 * 8):
    q = pyrr.Quaternion.from_y_rotation(i / 60 * np.pi)

    mm = pyrr.matrix33.create_from_quaternion(q)

    loc = np.array([0, -0.5, -1])

    # loc = mm.dot(loc)

    packet = b''

    temp = CONTOLLER_t.pack(
        *mm.dot(loc),
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )
    packet += temp

    temp = CONTOLLER_t.pack(
        *mm.dot(loc * [1, 1, 2]),
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    )
    packet += temp

    temp = POSE_t.pack(
        *mm.dot(loc * [1, 1, 3]),
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
    )
    packet += temp

    temp = POSE_t.pack(
        *mm.dot(loc * [1, 1, 4]),
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
    )
    packet += temp

    temp = POSE_t.pack(
        *mm.dot(loc * [1, 1, 5]),
        1, 0, 0, 0,
        int(i < 10), 0, 0,
        0, 0, 0,
    )
    packet += temp

    hmd_pose = POSE_t.pack(
        int(i < 10), 0, 0,
        0, 0, -1, 0,
        int(i < 10), 0, 0,
        0, 0, 0
    )

    tracking_socket.sendall(
        hmd_pose + packet + SEND_TERMINATOR
    )

    time.sleep(1 / 60)


print("press ctrl+C to stop...")

try:
    i = 0

    while 1:
        q = pyrr.Quaternion.from_y_rotation(i / 180 * np.pi)

        mm = pyrr.matrix33.create_from_quaternion(q)

        loc = np.array([0, -0.5, -1])

        # loc = mm.dot(loc)

        packet = b''

        temp = CONTOLLER_t.pack(
            *mm.dot(loc),
            1, 0, 0, 0,
            int(i < 10), 0, 0,
            0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        packet += temp

        q = pyrr.Quaternion.from_y_rotation(i / (180 * 2) * np.pi)

        mm = pyrr.matrix33.create_from_quaternion(q)

        temp = CONTOLLER_t.pack(
            *mm.dot(loc * [1, 1, 2]),
            1, 0, 0, 0,
            int(i < 10), 0, 0,
            0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        )
        packet += temp

        q = pyrr.Quaternion.from_y_rotation(i / (180 * 3) * np.pi)

        mm = pyrr.matrix33.create_from_quaternion(q)

        temp = POSE_t.pack(
            *mm.dot(loc * [1, 1, 3]),
            1, 0, 0, 0,
            int(i < 10), 0, 0,
            0, 0, 0,
        )
        packet += temp

        q = pyrr.Quaternion.from_y_rotation(i / (180 * 3.5) * np.pi)

        mm = pyrr.matrix33.create_from_quaternion(q)

        temp = POSE_t.pack(
            *mm.dot(loc * [1, 1, 4]),
            1, 0, 0, 0,
            int(i < 10), 0, 0,
            0, 0, 0,
        )
        packet += temp

        q = pyrr.Quaternion.from_y_rotation(i / (180 * 3.8) * np.pi)

        mm = pyrr.matrix33.create_from_quaternion(q)

        temp = POSE_t.pack(
            *mm.dot(loc * [1, 1, 5]),
            1, 0, 0, 0,
            int(i < 10), 0, 0,
            0, 0, 0,
        )
        packet += temp

        hmd_pose = POSE_t.pack(
            int(i % 60 == 0) / 10, 0, 0,
            0, 0, -1, 0,
            # a trick to make steamvr think the hmd is active
            int(i % 60 == 0) / 10, 0, 0,
            0, 0, 0
        )

        tracking_socket.sendall(
            hmd_pose + packet + SEND_TERMINATOR
        )

        if i % 360 == 0:
            print(f"last q: {q}")

        time.sleep(1 / 60)
        i += 1

except KeyboardInterrupt:
    print("interrupted, exiting...")

#######################################################################
# the end, time to die ^-^

client_a[0].close()
client_b[0].close()

serversocket.close()
