#include <ipc/server.hpp>
#include <json/value.h>
#include <vector>
#include "commands/command_impl.hpp"

static struct wl_event_source *ipc_event_source;
static std::vector<ipc_server_cli*> ipc_client_list;
static int ipc_socket;
static struct sockaddr_un ipc_sockaddr;
static struct wl_listener ipc_display_destroy;

void ipc_server_t::send_tick(const std::string& payload)
{
    if (!has_event_listeners(IPC_I3_EVENT_TYPE_TICK))
    {
        return;
    }

    LOGD("Sending tick event");

    Json::Value tickmsg;
    tickmsg["first"]   = false;
    tickmsg["payload"] = payload;
    std::string tickmsg_string = ipc_json::json_to_string(tickmsg);
    send_event(tickmsg_string.c_str(), IPC_I3_EVENT_TYPE_TICK);
}

void ipc_server_t::send_event(const char *json_string, enum ipc_event_type event)
{
    assert(json_string != nullptr);

    for (int i = 0; i < (int)ipc_client_list.size(); i++)
    {
        ipc_server_cli *client = ipc_client_list[i];
        if (client == nullptr)
        {
            continue;
        }

        if ((client->subscribed_events == IPC_WF_EVENT_TYPE_NONE) ||
            ((client->subscribed_events & event_mask(event)) == 0))
        {
            continue;
        }

        if (!send_reply(client, event, json_string,
            (uint32_t)std::string(json_string).size()))
        {
            LOGI("Unable to send reply to IPC client");
            /* send_reply destroys client on error, which also removes it from the
             * list, so we need to process current index again */
            i--;
        }
    }
}

uint32_t ipc_server_t::client_count()
{
    return (uint32_t)ipc_client_list.size();
}

void ipc_server_t::serve()
{
    ipc_socket = -1;
    ipc_event_source = nullptr;

    if (ipc_client_list.size() > 0)
    {
        ipc_client_list.clear();
    }

    ipc_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ipc_socket == -1)
    {
        LOGE("Unable to create IPC socket");
        exit(-1);
        return;
    }

    LOGD("IPC socket CREATED");

    if (fcntl(ipc_socket, F_SETFD, FD_CLOEXEC) == -1)
    {
        LOGE("Unable to set CLOEXEC on IPC socket");
        exit(-1);
        return;
    }

    LOGD("IPC socket CLOEXEC set");

    if (fcntl(ipc_socket, F_SETFL, O_NONBLOCK) == -1)
    {
        LOGE("Unable to set NONBLOCK on IPC socket");
        exit(-1);
        return;
    }

    LOGD("IPC socket NONBLOCK set");

    setup_user_sockaddr();

    struct reset_sockaddr_functor
    {
        void operator ()(char *socket_var)
        {
            strncpy(ipc_sockaddr.sun_path, socket_var,
                sizeof(ipc_sockaddr.sun_path) - 1);
            ipc_sockaddr.sun_path[sizeof(ipc_sockaddr.sun_path) - 1] = 0;
        }
    } reset_sockaddr;

    // We want to use socket name set by user, not existing socket from another
    // wayfire instance.
    if ((getenv("WAYFIRESOCK") != nullptr) &&
        (access(getenv("WAYFIRESOCK"), F_OK) == -1))
    {
        reset_sockaddr(getenv("WAYFIRESOCK"));
    } else if ((getenv("SWAYSOCK") != nullptr) &&
               (access(getenv("SWAYSOCK"), F_OK) == -1))
    {
        reset_sockaddr(getenv("SWAYSOCK"));
    } else if ((getenv("I3SOCK") != nullptr) &&
               (access(getenv("I3SOCK"), F_OK) == -1))
    {
        reset_sockaddr(getenv("I3SOCK"));
    }

    unlink(ipc_sockaddr.sun_path);
    if (bind(ipc_socket, (struct sockaddr*)&ipc_sockaddr,
        sizeof(ipc_sockaddr)) == -1)
    {
        LOGE("Unable to bind IPC socket");
        exit(-1);
    }

    if (listen(ipc_socket, 3) == -1)
    {
        LOGE("Unable to listen on IPC socket");
        exit(-1);
    }

    LOGD("ipc_sockaddr->sun_path ", ipc_sockaddr.sun_path);

    // Set wayfire IPC socket path so that wf-msg works out of the box
    setenv("WAYFIRESOCK", ipc_sockaddr.sun_path, 1);
    setenv("I3SOCK", ipc_sockaddr.sun_path, 1);
    setenv("SWAYSOCK", ipc_sockaddr.sun_path, 1);

    ipc_display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(wf::get_core().display,
        &ipc_display_destroy);

    ipc_event_source =
        wl_event_loop_add_fd(wf::get_core().ev_loop, ipc_socket,
            WL_EVENT_READABLE, handle_connection, nullptr);
}

bool ipc_server_t::send_reply(ipc_server_cli *client, int payload_type,
    const char *payload, uint32_t payload_length)
{
    assert(payload != nullptr);

    char data[IPC_HEADER_SIZE];

    memcpy(data, ipc_magic, sizeof(ipc_magic));
    memcpy(data + sizeof(ipc_magic), &payload_length, sizeof(payload_length));
    memcpy(data + sizeof(ipc_magic) + sizeof(payload_length), &payload_type,
        sizeof(payload_type));

    while (client->write_buffer_len + IPC_HEADER_SIZE + payload_length >=
           client->write_buffer_size)
    {
        client->write_buffer_size *= 2;
    }

    if (client->write_buffer_size > 4e6) // 4 MB
    {
        LOGE("Client write buffer too big (", client->write_buffer_size,
            "), disconnecting client");
        client_disconnect(client);
        return false;
    }

    char *new_buffer =
        static_cast<char*>(realloc(client->write_buffer, client->write_buffer_size));
    if (!new_buffer)
    {
        LOGE("Unable to reallocate ipc client write buffer");
        client_disconnect(client);
        return false;
    }

    client->write_buffer = new_buffer;

    memcpy(client->write_buffer + client->write_buffer_len, data, IPC_HEADER_SIZE);
    client->write_buffer_len += IPC_HEADER_SIZE;
    memcpy(client->write_buffer + client->write_buffer_len, payload, payload_length);
    client->write_buffer_len += payload_length;

    if (!client->writable_event_source)
    {
        client->writable_event_source = wl_event_loop_add_fd(
            wf::get_core().ev_loop, client->fd, WL_EVENT_WRITABLE,
            client_handle_writable, client);
    }

    LOGD("Added IPC reply of type ", payload_type, " to client ", client->fd,
        " queue: ",
        payload);
    return true;
}

void ipc_server_t::send_status(ipc_server_cli *client,
    enum ipc_payload_type payload_type,
    RETURN_STATUS status)
{
    std::string msg =
        status == RETURN_SUCCESS ? "{\"success\": true}" : "{\"success\": false}";
    send_reply(client, payload_type, msg.c_str(), msg.size());
}

void ipc_server_t::handle_display_destroy(struct wl_listener *listener, void *data)
{
    (void)listener;
    (void)data;

    if (ipc_event_source)
    {
        wl_event_source_remove(ipc_event_source);
    }

    close(ipc_socket);
    unlink(ipc_sockaddr.sun_path);

    while (ipc_client_list.size())
    {
        client_disconnect(ipc_client_list[ipc_client_list.size() - 1]);
    }

    ipc_client_list.clear();

    wl_list_remove(&ipc_display_destroy.link);
}

int ipc_server_t::handle_connection(int fd, uint32_t mask, void *data)
{
    (void)fd;
    (void)data;
    LOGD("Event on IPC listening socket");
    assert(mask == WL_EVENT_READABLE);

    int client_fd = accept(ipc_socket, nullptr, nullptr);
    if (client_fd == -1)
    {
        LOGE("Unable to accept IPC client connection");
        return 0;
    }

    int flags;
    if (((flags = fcntl(client_fd, F_GETFD)) == -1) ||
        (fcntl(client_fd, F_SETFD, flags | FD_CLOEXEC) == -1))
    {
        LOGE("Unable to set CLOEXEC on IPC client socket");
        close(client_fd);
        return 0;
    }

    if (((flags = fcntl(client_fd, F_GETFL)) == -1) ||
        (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1))
    {
        LOGE("Unable to set NONBLOCK on IPC client socket");
        close(client_fd);
        return 0;
    }

    ipc_server_cli *client = new ipc_server_cli();

    client->pending_length = 0;
    client->fd = client_fd;
    client->subscribed_events = IPC_WF_EVENT_TYPE_NONE;
    client->event_source =
        wl_event_loop_add_fd(wf::get_core().ev_loop, client_fd,
            WL_EVENT_READABLE, client_handle_readable, client);
    client->writable_event_source = nullptr;

    client->write_buffer_size = 128;
    client->write_buffer_len  = 0;
    client->write_buffer = static_cast<char*>(malloc(client->write_buffer_size));

    LOGD("New client: fd ", client_fd);
    ipc_client_list.push_back(client);
    return 0;
}

void ipc_server_t::setup_user_sockaddr()
{
    struct sockaddr_un *ipc_sock =
        (sockaddr_un*)&ipc_sockaddr;

    ipc_sock->sun_family = AF_UNIX;
    int path_size = sizeof(ipc_sock->sun_path);

    // Env var typically set by logind, e.g. "/run/user/<user-id>"
    const char *dir = getenv("XDG_RUNTIME_DIR");
    if (!dir)
    {
        dir = "/tmp";
    }

    LOGD("Socket path ", dir);

    if (path_size <= snprintf(ipc_sock->sun_path, path_size,
        "%s/wf-ipc.%u.sock", dir, getuid()))
    {
        LOGE("Socket path is too long");
    }
}

int ipc_server_t::client_handle_writable(int client_fd, uint32_t mask, void *data)
{
    (void)client_fd;
    ipc_server_cli *client = static_cast<ipc_server_cli*>(data);

    if (mask & WL_EVENT_ERROR)
    {
        LOGE("IPC Client socket error, removing client");
        client_disconnect(client);
        return 0;
    }

    if (mask & WL_EVENT_HANGUP)
    {
        LOGD("Client ", client->fd, " hung up");
        client_disconnect(client);
        return 0;
    }

    if ((int)client->write_buffer_len <= 0)
    {
        return 0;
    }

    LOGD("Client ", client->fd, " writable");

    ssize_t written =
        write(client->fd, client->write_buffer, client->write_buffer_len);

    if ((written == -1) && (errno == EAGAIN))
    {
        return 0;
    } else if (written == -1)
    {
        LOGI("Unable to send data from queue to IPC client");
        client_disconnect(client);
        return 0;
    }

    memmove(client->write_buffer, client->write_buffer + written,
        client->write_buffer_len - written);
    client->write_buffer_len -= written;

    if ((client->write_buffer_len == 0) && client->writable_event_source)
    {
        wl_event_source_remove(client->writable_event_source);
        client->writable_event_source = nullptr;
    }

    return 0;
}

int ipc_server_t::client_handle_readable(int client_fd, uint32_t mask, void *data)
{
    ipc_server_cli *client = static_cast<ipc_server_cli*>(data);

    if (mask & WL_EVENT_ERROR)
    {
        LOGE("IPC Client socket error, removing client");
        client_disconnect(client);
        return 0;
    }

    if (mask & WL_EVENT_HANGUP)
    {
        LOGD("Client ", client->fd, " hung up");
        client_disconnect(client);
        return 0;
    }

    LOGD("Client ", client->fd, " readable");

    int read_available;
    if (ioctl(client_fd, FIONREAD, &read_available) == -1)
    {
        LOGI("Unable to read IPC socket buffer size");
        client_disconnect(client);
        return 0;
    }

    // Wait for the rest of the command payload in case the header has already
    // been read
    if (client->pending_length > 0)
    {
        if ((uint32_t)read_available >= client->pending_length)
        {
            // Reset pending values.
            uint32_t pending_length = client->pending_length;
            enum ipc_payload_type pending_type = client->pending_type;
            client->pending_length = 0;
            ipc_client_handle_command(client, pending_length, pending_type);
        }

        return 0;
    }

    if (read_available < (int)IPC_HEADER_SIZE)
    {
        return 0;
    }

    uint8_t buf[IPC_HEADER_SIZE];
    // Should be fully available, because read_available >= IPC_HEADER_SIZE
    ssize_t received = recv(client_fd, buf, IPC_HEADER_SIZE, 0);
    if (received == -1)
    {
        LOGI("Unable to receive header from IPC client");
        client_disconnect(client);
        return 0;
    }

    if (memcmp(buf, ipc_magic, sizeof(ipc_magic)) != 0)
    {
        LOGD("IPC header check failed");
        client_disconnect(client);
        return 0;
    }

    memcpy(&client->pending_length, buf + sizeof(ipc_magic),
        sizeof(uint32_t));
    memcpy(&client->pending_type,
        buf + sizeof(ipc_magic) + sizeof(uint32_t),
        sizeof(uint32_t));

    if (read_available - received >= (long)client->pending_length)
    {
        // Reset pending values.
        uint32_t pending_length = client->pending_length;
        enum ipc_payload_type pending_type = client->pending_type;
        client->pending_length = 0;
        ipc_client_handle_command(client, pending_length, pending_type);
    }

    return 0;
}

void ipc_server_t::client_disconnect(ipc_server_cli *client)
{
    if (client == nullptr)
    {
        LOGE("Client must not be null");
        exit(-1);
    }

    shutdown(client->fd, SHUT_RDWR);

    LOGI("IPC Client ", client->fd, " disconnected");
    wl_event_source_remove(client->event_source);
    if (client->writable_event_source)
    {
        wl_event_source_remove(client->writable_event_source);
    }

    for (auto it = ipc_client_list.begin(); it != ipc_client_list.end();)
    {
        if (*it == client)
        {
            it = ipc_client_list.erase(it);
        } else
        {
            ++it;
        }
    }

    free(client->write_buffer);
    client->write_buffer = nullptr;
    close(client->fd);
    delete client;
}

bool ipc_server_t::has_event_listeners(enum ipc_event_type event)
{
    for (int i = 0; i < (int)ipc_client_list.size(); i++)
    {
        ipc_server_cli *client = ipc_client_list[i];
        if ((client->subscribed_events & event_mask(event)) != 0)
        {
            return true;
        }
    }

    return false;
}

#define ASYNC_INTERNAL_COMMAND 1
#define SYNC_INTERNAL_COMMAND 0

void ipc_server_t::ipc_client_handle_command(ipc_server_cli *client,
    uint32_t payload_length,
    enum ipc_payload_type payload_type)
{
    if (client == nullptr)
    {
        LOGE("Client must not be null");
        exit(-1);
    }

    std::string buf(payload_length, '\0');

    if (payload_length > 0)
    {
        // Payload should be fully available
        ssize_t received = recv(client->fd, buf.data(), payload_length, 0);
        if (received == -1)
        {
            LOGI("Unable to receive payload from IPC client");
            client_disconnect(client);
            return;
        }
    }

    switch (payload_type)
    {
      case IPC_COMMAND:
    {
        std::vector<std::string> lines = ipc_tools::split_string(buf, "\n");

        std::string exec;
        for (std::string line : lines)
        {
            exec.append(line);
            exec.append(";");
        }

        Json::Value ret_list = ipc_command::execute_command(exec);

        std::string json_string = ipc_json::json_to_string(ret_list);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());
        break;
    }

      case IPC_SEND_TICK:
    {
        send_tick(buf);
        send_status(client, payload_type, RETURN_SUCCESS);
        break;
    }

      case IPC_GET_OUTPUTS:
    {
        Json::Value object = Json::arrayValue;
        auto outputs = wf::get_core().output_layout->get_outputs();
        for (wf::output_t *output : outputs)
        {
            if (output->handle->enabled)
            {
                Json::Value output_json = ipc_json::describe_output(output);

                output_json["focused"] = wf::get_core().get_active_output() ==
                    output;

                const char *subpixel =
                    ipc_json::output_subpixel_description(output->handle->subpixel);
                output_json["subpixel_hinting"] = subpixel;

                object.append(output_json);
            }
        }

        for (wf::output_t *output : outputs)
        {
            if (!output->handle->enabled)
            {
                Json::Value output_json = ipc_json::describe_disabled_output(output);
                object.append(output_json);
            }
        }

        std::string json_string = ipc_json::json_to_string(object);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());

        break;
    }

      case IPC_GET_WORKSPACES:
    {
        Json::Value workspaces = Json::arrayValue;
        std::vector<Json::Value> vector;

        auto outputs = wf::get_core().output_layout->get_outputs();
        for (wf::output_t *output : outputs)
        {
            auto wsize = output->workspace->get_workspace_grid_size();
            for (int x = 0; x < wsize.width; x++)
            {
                for (int y = 0; y < wsize.height; y++)
                {
                    Json::Value workspace = ipc_json::describe_workspace(
                        wf::point_t{x, y}, output);
                    vector.push_back(workspace);
                }
            }
        }

        std::sort(vector.begin(), vector.end(),
            [] (Json::Value workspace_a, Json::Value workspace_b)
            {
                Json::Value workspace_a_id = workspace_a.get("id", Json::nullValue);
                Json::Value workspace_b_id = workspace_b.get("id", Json::nullValue);

                if (workspace_a_id.isNull() || !workspace_a_id.isInt())
                {
                    return false;
                }

                if (workspace_b_id.isNull() || !workspace_b_id.isInt())
                {
                    return false;
                }

                int a_id = workspace_a_id.asInt();
                int b_id = workspace_b_id.asInt();

                return a_id < b_id;
            });

        for (auto& workspace : vector)
        {
            workspaces.append(workspace);
        }

        std::string json_string = ipc_json::json_to_string(workspaces);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());

        break;
    }

      case IPC_GET_INPUTS:
    {
        Json::Value object;

        auto devices = wf::get_core().get_input_devices();
        for (auto & dev : devices)
        {
            object.append(ipc_json::describe_input(dev));
        }

        std::string json_string = ipc_json::json_to_string(object);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());
        break;
    }

      case IPC_GET_VERSION:
    {
        Json::Value version     = ipc_json::get_version();
        std::string json_string = ipc_json::json_to_string(version);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());
        break;
    }

      case IPC_SUBSCRIBE:
    {
        // TODO: Check if they're permitted to use these events
        Json::Value request = ipc_json::string_to_json(buf);
        if ((request.size() == 0) || !request.isArray())
        {
            send_status(client, payload_type, RETURN_INVALID_PARAMETER);
            LOGI("Failed to parse subscribe request");
            break;
        }

        bool has_tick = false;
        // parse requested event types
        for (Json::Value::ArrayIndex i = 0; i < request.size(); i++)
        {
            if (request[i].isString())
            {
                std::string event = std::string(request[i].asString());

                if (event == IPC_I3_EVENT_WORKSPACE)
                {
                    client->subscribed_events |= event_mask(
                        IPC_I3_EVENT_TYPE_WORKSPACE);
                } else if (event == IPC_I3_EVENT_SHUTDOWN)
                {
                    client->subscribed_events |= event_mask(
                        IPC_I3_EVENT_TYPE_SHUTDOWN);
                } else if (event == IPC_I3_EVENT_WINDOW)
                {
                    client->subscribed_events |=
                        event_mask(IPC_I3_EVENT_TYPE_WINDOW);
                } else if (event == IPC_I3_EVENT_BINDING)
                {
                    client->subscribed_events |=
                        event_mask(IPC_I3_EVENT_TYPE_BINDING);
                } else if (event == IPC_I3_EVENT_TICK)
                {
                    client->subscribed_events |= event_mask(IPC_I3_EVENT_TYPE_TICK);
                    has_tick = true;
                } else if (event == IPC_SWAY_EVENT_INPUT)
                {
                    client->subscribed_events |=
                        event_mask(IPC_SWAY_EVENT_TYPE_INPUT);
                } else
                {
                    client->subscribed_events = IPC_WF_EVENT_TYPE_NONE;
                    send_status(client, payload_type, RETURN_UNSUPPORTED);
                    LOGI("Unsupported event type in subscribe request");
                    return;
                }
            }
        }

        send_status(client, payload_type, RETURN_SUCCESS);

        if (has_tick)
        {
            Json::Value msg;
            msg["first"]   = true;
            msg["payload"] = "";
            std::string msg_string = ipc_json::json_to_string(msg);
            send_reply(client, (int)IPC_I3_EVENT_TYPE_TICK,
                msg_string.c_str(), msg_string.size());
        }

        break;
    }

      case IPC_UNSUBSCRIBE:
    {
        // TODO: Check if they're permitted to use these events
        Json::Value request = ipc_json::string_to_json(buf);
        if ((request.size() == 0) || !request.isArray())
        {
            send_status(client, payload_type, RETURN_INVALID_PARAMETER);
            LOGI("Failed to parse unsubscribe request");
            break;
        }

        // parse requested event types
        for (Json::Value::ArrayIndex i = 0; i < request.size(); i++)
        {
            if (request[i].isString())
            {
                std::string event = std::string(request[i].asString());

                auto event_type = ipc_tools::event_to_type(event);

                if (event_type == IPC_WF_EVENT_TYPE_UNSUPPORTED)
                {
                    send_status(client, payload_type, RETURN_UNSUPPORTED);
                    LOGI("Unsupported event type in subscribe request");
                    return;
                }

                if ((client->subscribed_events & event_mask(event_type)) != 0)
                {
                    client->subscribed_events = client->subscribed_events &
                        ~event_mask(event_type);
                }
            }
        }

        send_status(client, payload_type, RETURN_SUCCESS);
        break;
    }

      case IPC_GET_SEATS:
    {
        Json::Value seats = Json::arrayValue;
        Json::Value seat  =
            ipc_json::describe_seat(wf::get_core().get_current_seat());
        seats.append(seat);
        std::string json_string = ipc_json::json_to_string(seats);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());

        break;
    }

      case IPC_GET_CONFIG:
    {
        Json::Value object;

        auto config = &wf::get_core().config;
        if (config)
        {
            Json::Value sections = Json::arrayValue;
            std::vector<std::shared_ptr<wf::config::section_t>> all_sections =
                config->get_all_sections();

            for (std::shared_ptr<wf::config::section_t> s : all_sections)
            {
                Json::Value section;
                section["name"] = s->get_name();
                section["type"] = "section";

                Json::Value options = Json::arrayValue;
                std::vector<std::shared_ptr<wf::config::option_base_t>>
                registred_options = s->get_registered_options();

                for (std::shared_ptr<wf::config::option_base_t> registred_option
                     :
                     registred_options)
                {
                    Json::Value option;
                    option["name"]  = registred_option->get_name();
                    option["type"]  = "option";
                    option["value"] = registred_option->get_value_str();
                    options.append(option);
                }

                section["options"] = options;
                sections.append(section);
            }

            object["config"] = sections;
        }

        std::string json_string = ipc_json::json_to_string(object);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());
        break;
    }

      case IPC_GET_TREE:
    {
        Json::Value tree = ipc_json::get_tree();
        std::string json_string = ipc_json::json_to_string(tree);
        send_reply(client, payload_type, json_string.c_str(),
            (uint32_t)json_string.size());

        break;
    }

      case IPC_SYNC:
    {
        // Not supported, just return success:false
        send_status(client, payload_type, RETURN_UNSUPPORTED);
        break;
    }

      default:
    {
        send_status(client, payload_type, RETURN_UNSUPPORTED);
        LOGI("Unknown IPC command type ", payload_type);
        break;
    }
    }

    return;
}
