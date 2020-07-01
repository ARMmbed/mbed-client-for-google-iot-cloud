/* Mbed OS Google Cloud BSP
 * Copyright (c) 2019-2020, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "mbed_trace.h"

#define TRACE_GROUP "GoogleCloudPort"

#include <iotc_bsp_io_net.h>
#include <stdio.h>
#include <string.h>
#include "iotc_macros.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
iotc_bsp_io_net_state_t iotc_bsp_io_net_socket_connect(
    iotc_bsp_socket_t *iotc_socket, const char *host, uint16_t port,
    iotc_bsp_socket_type_t socket_type)
{
    nsapi_error_t result = NSAPI_ERROR_OK;
    auto net = NetworkInterface::get_default_instance();
    auto conn = new TCPSocket;
    if (!net) {
        tr_error("Error! No network inteface found.\n");
        goto DISCONNECT;
    }

    result = net->connect();
    if (NSAPI_ERROR_OK != result) {
        tr_error("failed to connect with : %d", result);
        goto DISCONNECT;
    }

    result = conn->open(net);
    if (result != NSAPI_ERROR_OK) {
        tr_error("Error! socket.open() returned: %d\n", result);
        goto DISCONNECT;
    }
    {
        SocketAddress addr;
        result = net->gethostbyname(host, &addr);
        if (result != NSAPI_ERROR_OK) {
            tr_error("Error! DNS resolution for %s failed with %d", host, result);
            goto DISCONNECT;
        }
        addr.set_port(port);

        result = conn->connect(addr);

        if (result != NSAPI_ERROR_OK) {
            tr_error("Error! socket.connect() returned: %d\n", result);
            goto DISCONNECT;
        }
    }
    iotc_socket = (iotc_bsp_socket_t *)conn;
    return IOTC_BSP_IO_NET_STATE_OK;
DISCONNECT:
    delete conn;
    // Bring down the network interface
    net->disconnect();

    return IOTC_BSP_IO_NET_STATE_ERROR;
}

iotc_bsp_io_net_state_t iotc_bsp_io_net_connection_check(
    iotc_bsp_socket_t iotc_socket, const char *host, uint16_t port)
{
    IOTC_UNUSED(host);
    IOTC_UNUSED(port);

    int valopt = 0;
    unsigned lon = sizeof(int);

    if (((TCPSocket *)iotc_socket)->getsockopt(NSAPI_SOCKET, 0, (void *)(&valopt), &lon) <
            0) {
        return IOTC_BSP_IO_NET_STATE_ERROR;
    }

    if (valopt) {
        return IOTC_BSP_IO_NET_STATE_ERROR;
    }

    return IOTC_BSP_IO_NET_STATE_OK;
}

iotc_bsp_io_net_state_t iotc_bsp_io_net_write(iotc_bsp_socket_t iotc_socket,
                                              int *out_written_count,
                                              const uint8_t *buf,
                                              size_t count)
{
    if (NULL == out_written_count || NULL == buf) {
        return IOTC_BSP_IO_NET_STATE_ERROR;
    }

    auto res = ((TCPSocket *)iotc_socket)->send(buf, count);

    if (res < 0) {
        *out_written_count = 0;
        tr_error("failed to send data with %d", res);

        if (NSAPI_ERROR_NO_SOCKET == res) {
            return IOTC_BSP_IO_NET_STATE_CONNECTION_RESET;
        }

        if (NSAPI_ERROR_WOULD_BLOCK == res) {
            return IOTC_BSP_IO_NET_STATE_BUSY;
        }

        return IOTC_BSP_IO_NET_STATE_ERROR;
    }
    *out_written_count = res;

    return IOTC_BSP_IO_NET_STATE_OK;
}

iotc_bsp_io_net_state_t iotc_bsp_io_net_read(iotc_bsp_socket_t iotc_socket,
                                             int *out_read_count, uint8_t *buf,
                                             size_t count)
{

    ((TCPSocket *)iotc_socket)->set_blocking(false);
    auto res = ((TCPSocket *)iotc_socket)->recv(buf, count);

    if (res < 0) {
        *out_read_count = 0;
        tr_error("failed to recv data with %d", res);

        if (NSAPI_ERROR_NO_SOCKET == res) {
            return IOTC_BSP_IO_NET_STATE_CONNECTION_RESET;
        }

        if (NSAPI_ERROR_WOULD_BLOCK == res) {
            return IOTC_BSP_IO_NET_STATE_BUSY;
        }

        return IOTC_BSP_IO_NET_STATE_ERROR;

    } else if (res != count) {
        tr_warning("Unexpected recv length (got %u, expected %u)", res, count);
    }
    *out_read_count = res;

    return IOTC_BSP_IO_NET_STATE_OK;

}

iotc_bsp_io_net_state_t iotc_bsp_io_net_close_socket(
    iotc_bsp_socket_t *iotc_socket)
{
    if (NULL == iotc_socket) {
        return IOTC_BSP_IO_NET_STATE_ERROR;
    }

    ((TCPSocket *)iotc_socket)->close();
    delete ((TCPSocket *)iotc_socket);
    *iotc_socket = 0;
    return IOTC_BSP_IO_NET_STATE_OK;
}

#define CFD_SET(socket, event, pollfd) \
  pollfd.fd = socket;                 \
  pollfd.events |= event;
#define CFD_ISSET(event, pollfd) (pollfd.revents | event)

iotc_bsp_io_net_state_t iotc_bsp_io_net_select(
    iotc_bsp_socket_events_t *socket_events_array,
    size_t socket_events_array_size, long timeout_sec)
{
    struct pollfd fds[1];  // note: single socket support

    /* translate the library socket events settings to the event sets used by
     * Mbed OS poll mechanism
     * Note: currently only one socket is supported
     */
    size_t socket_id = 0;
    for (socket_id = 0; socket_id < socket_events_array_size; ++socket_id) {
        iotc_bsp_socket_events_t *socket_events = &socket_events_array[socket_id];

        if (NULL == socket_events) {
            return IOTC_BSP_IO_NET_STATE_ERROR;
        }

        if (1 == socket_events->in_socket_want_read) {
            CFD_SET(socket_events->iotc_socket, POLLIN, fds[0]);
        }

        if ((1 == socket_events->in_socket_want_write) ||
                (1 == socket_events->in_socket_want_connect)) {
            CFD_SET(socket_events->iotc_socket, POLLOUT, fds[0]);
        }

        if (1 == socket_events->in_socket_want_error) {
            CFD_SET(socket_events->iotc_socket, POLLERR, fds[0]);
        }
    }

    const int result = poll(fds, 1, timeout_sec);

    if (0 < result) {
        /* translate the result back to the socket events structure */
        for (socket_id = 0; socket_id < socket_events_array_size; ++socket_id) {
            iotc_bsp_socket_events_t *socket_events = &socket_events_array[socket_id];

            if (CFD_ISSET(POLLIN, fds[0])) {
                socket_events->out_socket_can_read = 1;
            }

            if (CFD_ISSET(POLLOUT, fds[0])) {
                if (1 == socket_events->in_socket_want_connect) {
                    socket_events->out_socket_connect_finished = 1;
                }

                if (1 == socket_events->in_socket_want_write) {
                    socket_events->out_socket_can_write = 1;
                }
            }

            if (CFD_ISSET(POLLERR, fds[0])) {
                socket_events->out_socket_error = 1;
            }
        }

        return IOTC_BSP_IO_NET_STATE_OK;
    } else if (0 == result) {
        return IOTC_BSP_IO_NET_STATE_TIMEOUT;
    }
    return IOTC_BSP_IO_NET_STATE_ERROR;
}

#ifdef __cplusplus
}
#endif