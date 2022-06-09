#ifndef _WAYFIRE_IPC_SERVER_H
#define _WAYFIRE_IPC_SERVER_H


#include <ipc.h>
#include <errno.h>
#include <fcntl.h>
#include <future>
#include <json/json.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <wayfire/config-backend.hpp>
#include <wayfire/core.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/view.hpp>
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/section.hpp>
#include <wayfire/geometry.hpp>
#include <wayfire/input-device.hpp>
#include <wayfire/object.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/workspace-manager.hpp>
#include <wayland-server-core.h>
#include <wayland-client.h>
#include <ipc/json.hpp>
#include <ipc/tools.hpp>

class ipc_server_cli
{
  public:
    //
    // The following uuid and group_uuid are to extend the i3 protocol
    // by adding a sendtick_to and sendtick_togroup functions, to be able
    // to send an event to a specific client or a group of clients.
    //
    std::string uuid;
    std::string group_uuid;

    wl_event_loop *ev_loop = nullptr;
    struct wl_event_source *event_source = nullptr;
    struct wl_event_source *writable_event_source = nullptr;
    int fd = 0;
    wf::compositor_core_t *core = nullptr;
    long subscribed_events   = 0;
    size_t write_buffer_len  = 0;
    size_t write_buffer_size = 0;
    char *write_buffer = nullptr;
    //
    // The following are for storing data between event_loop calls
    //
    uint32_t pending_length = 0;
    enum ipc_payload_type pending_type;
};

class ipc_server_t
{
  public:
    static void send_tick(const std::string& payload);
    static void send_event(const char *json_string, enum ipc_event_type event);
    static uint32_t client_count();

    // protected:

  public:
    void serve();

  protected:
    static bool send_reply(ipc_server_cli *client, int payload_type,
        const char *payload, uint32_t payload_length);
    static void send_status(ipc_server_cli *client,
        enum ipc_payload_type payload_type,
        RETURN_STATUS status);

  public:
    static void handle_display_destroy(wl_listener *listener, void *data);
    static bool has_event_listeners(enum ipc_event_type event);

  private:
    static int handle_connection(int fd, uint32_t mask, void *data);
    void setup_user_sockaddr();
    static int client_handle_writable(int client_fd, uint32_t mask, void *data);
    static int client_handle_readable(int client_fd, uint32_t mask, void *data);
    static void client_disconnect(ipc_server_cli *client);
    static void ipc_client_handle_command(ipc_server_cli *client,
        uint32_t payload_length,
        enum ipc_payload_type payload_type);
};

#endif
