#include <ostream>
#include <string>
#include <wayfire/util/log.hpp>
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "ipc-client.hpp"
#include <iostream>

char *get_socketpath(void)
{
    const char *wfsock = getenv("WAYFIRESOCK");
    if (wfsock)
    {
        return strdup(wfsock);
    }

    const char *swaysock = getenv("SWAYSOCK");
    if (swaysock)
    {
        return strdup(swaysock);
    }

    const char *i3sock = getenv("I3SOCK");
    if (i3sock)
    {
        return strdup(i3sock);
    }

    const char *dir = getenv("XDG_RUNTIME_DIR");
    if (!dir)
    {
        dir = "/tmp";
    }

    int path_size  = 108;
    char *sun_path = static_cast<char*>(malloc(path_size + 1));
    bzero(sun_path, path_size + 1);

    if (path_size <=
        snprintf(sun_path, path_size, "%s/wf-ipc.%u.sock", dir, getuid()))
    {
        LOGE("Socket path is too long");
        return nullptr;
    }

    return sun_path;
}

int ipc_open_socket(const char *socket_path)
{
    struct sockaddr_un addr;
    int socketfd;
    if ((socketfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        client_abort("Unable to open Unix socket");
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;
    int l = sizeof(struct sockaddr_un);
    if (connect(socketfd, (struct sockaddr*)&addr, l) == -1)
    {
        client_abort("Unable to connect to %s", socket_path);
    }

    return socketfd;
}

bool ipc_set_recv_timeout(int socketfd, struct timeval tv)
{
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
    {
        LOGE("Failed to set ipc recv timeout");
        return false;
    }

    return true;
}

struct ipc_response *ipc_recv_response(int socketfd)
{
    char data[IPC_HEADER_SIZE];

    size_t total = 0;
    while (total < IPC_HEADER_SIZE)
    {
        ssize_t received = recv(socketfd, data + total, IPC_HEADER_SIZE - total, 0);
        if (received <= 0)
        {
            client_abort("Unable to receive IPC response");
        }

        total += received;
    }

    struct ipc_response *response =
        static_cast<struct ipc_response*>(malloc(sizeof(struct ipc_response)));
    if (!response)
    {
        LOGE("Unable to allocate memory for IPC response");
        return nullptr;
    }

    memcpy(&response->size, data + sizeof(ipc_magic), sizeof(uint32_t));
    memcpy(&response->type, data + sizeof(ipc_magic) + sizeof(uint32_t),
        sizeof(uint32_t));

    char *payload = static_cast<char*>(malloc(response->size + 1));
    if (!payload)
    {
        free(response);
        LOGE("Unable to allocate memory for IPC response");
        return nullptr;
    }

    total = 0;
    while (total < response->size)
    {
        ssize_t received =
            recv(socketfd, payload + total, response->size - total, 0);
        if (received < 0)
        {
            client_abort("Unable to receive IPC response");
        }

        total += received;
    }

    payload[response->size] = '\0';
    response->payload = payload;

    return response;
}

void free_ipc_response(struct ipc_response *response)
{
    free(response->payload);
    free(response);
}

char *ipc_single_command(int socketfd, uint32_t type, const char *payload,
    uint32_t *len)
{
    char data[IPC_HEADER_SIZE];
    memcpy(data, ipc_magic, sizeof(ipc_magic));
    memcpy(data + sizeof(ipc_magic), len, sizeof(*len));
    memcpy(data + sizeof(ipc_magic) + sizeof(*len), &type, sizeof(type));

    if (write(socketfd, data, IPC_HEADER_SIZE) == -1)
    {
        client_abort("Unable to send IPC header");
    }

    if (write(socketfd, payload, *len) == -1)
    {
        client_abort("Unable to send IPC payload");
    }

    struct ipc_response *resp = ipc_recv_response(socketfd);
    char *response = resp->payload;
    *len = resp->size;
    free(resp);

    return response;
}
