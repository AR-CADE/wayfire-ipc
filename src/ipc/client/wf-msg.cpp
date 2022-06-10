#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <ipc/json.hpp>
#include <ipc/tools.hpp>
#include <ipc.h>
#include "ipc-client.hpp"
#include <wayfire/plugin.hpp>

static bool success_object(Json::Value result)
{
    Json::Value ret = result.get("success", Json::nullValue);
    if (ret.isNull() || !ret.isBool())
    {
        return true;
    }

    return ret.asBool();
}

// Iterate results array and return false if any of them failed
static bool success(Json::Value r, bool fallback)
{
    if (!r.isArray())
    {
        if (r.isObject())
        {
            return success_object(r);
        }

        return fallback;
    }

    if (r.empty())
    {
        return fallback;
    }

    if (std::any_of(r.begin(), r.end(),
        [] (Json::Value& result) {return !success_object(result);}))
    {
        return false;
    }

    return true;
}

static void pretty_print_cmd(Json::Value r)
{
    if (!success_object(r))
    {
        Json::Value error = r.get("error", Json::nullValue);

        if (error.isNull())
        {
            printf("An unknkown error occurred");
        } else
        {
            std::string str = ipc_json::json_to_string(error);
            printf("Error: %s\n", str.c_str());
        }
    }
}

static void pretty_print_workspace(Json::Value w)
{
    Json::Value name = w.get("name", Json::nullValue);
    // Json::Value rect = w.get("rect", Json::nullValue);
    Json::Value visible = w.get("visible", Json::nullValue);
    Json::Value output  = w.get("output", Json::nullValue);
    Json::Value urgent  = w.get("urgent", Json::nullValue);
    Json::Value layout  = w.get("layout", Json::nullValue);
    Json::Value representation = w.get("representation", Json::nullValue);
    Json::Value focused = w.get("focused", Json::nullValue);

    printf(
        "Workspace %s%s%s%s\n"
        "  Output: %s\n"
        "  Layout: %s\n"
        "  Representation: %s\n\n",
        name.asString().c_str(),
        focused.asBool() ? " (focused)" : "",
        !visible.asBool() ? " (off-screen)" : "",
        urgent.asBool() ? " (urgent)" : "",
        output.asString().c_str(),
        layout.asString().c_str(),
        representation.asString().c_str());
}

static const char *pretty_type_name(const char *name)
{
    // TODO these constants probably belong in the common lib
    struct
    {
        const char *a;
        const char *b;
    } type_names[] = {
        {"keyboard", "Keyboard"},
        {"pointer", "Mouse"},
        {"touchpad", "Touchpad"},
        {"tablet_pad", "Tablet pad"},
        {"tablet_tool", "Tablet tool"},
        {"touch", "Touch"},
        {"switch", "Switch"},
    };

    for (size_t i = 0; i < sizeof(type_names) / sizeof(type_names[0]); ++i)
    {
        if (strcmp(type_names[i].a, name) == 0)
        {
            return type_names[i].b;
        }
    }

    return name;
}

static void pretty_print_input(Json::Value i)
{
    Json::Value id   = i.get("identifier", Json::nullValue);
    Json::Value name = i.get("name", Json::nullValue);
    Json::Value type = i.get("type", Json::nullValue);
    Json::Value product = i.get("product", Json::nullValue);
    Json::Value vendor  = i.get("vendor", Json::nullValue);

    const char *fmt =
        "Input device: %s\n"
        "  Type: %s\n"
        "  Identifier: %s\n"
        "  Product ID: %d\n"
        "  Vendor ID: %d\n";

    printf(fmt, name.asString().c_str(),
        pretty_type_name(type.asString().c_str()),
        id.asString().c_str(),
        product.asInt(),
        vendor.asInt());

    Json::Value kbdlayout = i.get("xkb_active_layout_name", Json::nullValue);
    if (!kbdlayout.isNull())
    {
        std::string str    = ipc_json::json_to_string(kbdlayout);
        const char *layout = str.c_str();
        printf("  Active Keyboard Layout: %s\n",
            layout != nullptr ? layout : "(unnamed)");
    }

    Json::Value libinput = i.get("libinput", Json::nullValue);
    if (!libinput.isNull())
    {
        Json::Value events = libinput.get("send_events", Json::nullValue);
        if (!events.isNull())
        {
            printf("  Libinput Send Events: %s\n",
                events.asString().c_str());
        }
    }

    printf("\n");
}

static void pretty_print_seat(Json::Value i)
{
    Json::Value name = i.get("name", Json::nullValue);
    Json::Value capabilities = i.get("capabilities", Json::nullValue);
    Json::Value devices = i.get("devices", Json::nullValue);

    const char *fmt =
        "Seat: %s\n"
        "  Capabilities: %d\n";

    printf(fmt, name.asString().c_str(),
        capabilities.asInt());

    if (devices.isArray() && (devices.empty() == false))
    {
        printf("  Devices:\n");
        for (auto& device : devices)
        {
            Json::Value device_name = device.get("name", Json::nullValue);
            printf("    %s\n", device_name.asString().c_str());
        }
    }

    printf("\n");
}

static void pretty_print_output(Json::Value o)
{
    Json::Value name    = o.get("name", Json::nullValue);
    Json::Value rect    = o.get("rect", Json::nullValue);
    Json::Value focused = o.get("focused", Json::nullValue);
    Json::Value active  = o.get("active", Json::nullValue);
    Json::Value ws = o.get("current_workspace", Json::nullValue);

    Json::Value make   = o.get("make", Json::nullValue);
    Json::Value model  = o.get("model", Json::nullValue);
    Json::Value serial = o.get("serial", Json::nullValue);
    Json::Value scale  = o.get("scale", Json::nullValue);
    Json::Value scale_filter = o.get("scale_filter", Json::nullValue);
    Json::Value subpixel     = o.get("subpixel_hinting", Json::nullValue);
    Json::Value transform    = o.get("transform", Json::nullValue);
    Json::Value max_render_time = o.get("max_render_time", Json::nullValue);
    Json::Value adaptive_sync_status =
        o.get("adaptive_sync_status", Json::nullValue);

    Json::Value x = rect.get("x", Json::nullValue);
    Json::Value y = rect.get("y", Json::nullValue);

    Json::Value modes = o.get("modes", Json::nullValue);
    Json::Value current_mode = o.get("current_mode", Json::nullValue);
    Json::Value width   = current_mode.get("width", Json::nullValue);
    Json::Value height  = current_mode.get("height", Json::nullValue);
    Json::Value refresh = current_mode.get("refresh", Json::nullValue);

    if (active.asBool() == true)
    {
        printf(
            "Output %s '%s %s %s'%s\n"
            "  Current mode: %dx%d @ %.3f Hz\n"
            "  Position: %d,%d\n"
            "  Scale factor: %f\n"
            "  Scale filter: %s\n"
            "  Subpixel hinting: %s\n"
            "  Transform: %s\n"
            "  Workspace: %s\n",
            name.asString().c_str(),
            make.asString().c_str(),
            model.asString().c_str(),
            serial.asString().c_str(),
            focused.asBool() ? " (focused)" : "",
            width.asInt(),
            height.asInt(),
            (double)refresh.asInt() / 1000,
            x.asInt(), y.asInt(),
            scale.asDouble(),
            scale_filter.asString().c_str(),
            subpixel.asString().c_str(),
            transform.asString().c_str(),
            ws.asString().c_str());

        int max_render_time_int = max_render_time.asInt();
        printf("  Max render time: ");
        printf(max_render_time_int == 0 ? "off\n" : "%d ms\n", max_render_time_int);

        printf("  Adaptive sync: %s\n",
            adaptive_sync_status.asString().c_str());
    } else
    {
        printf(
            "Output %s '%s %s %s' (inactive)\n",
            name.asString().c_str(),
            make.asString().c_str(),
            model.asString().c_str(),
            serial.asString().c_str());
    }

    if (modes.isArray() && (modes.empty() == false))
    {
        printf("  Available modes:\n");
        for (auto& mode : modes)
        {
            Json::Value mode_width   = mode.get("width", Json::nullValue);
            Json::Value mode_height  = mode.get("height", Json::nullValue);
            Json::Value mode_refresh = mode.get("refresh", Json::nullValue);

            printf("    %dx%d @ %.3f Hz\n", mode_width.asInt(),
                mode_height.asInt(),
                (double)mode_refresh.asInt() / 1000);
        }
    }

    printf("\n");
}

static void pretty_print_version(Json::Value v)
{
    if (v.isNull())
    {
        return;
    }

    Json::Value ver = v.get("human_readable", Json::nullValue);
    std::string str = ipc_json::json_to_string(ver);
    printf("wayfire version %s\n", str.c_str());
}

static void pretty_print_config(Json::Value c)
{
    if (c.isNull())
    {
        return;
    }

    Json::Value config = c.get("config", Json::nullValue);
    std::string str    = ipc_json::json_to_string(config);
    printf("%s\n", str.c_str());
}

static void pretty_print_tree(Json::Value obj, int indent)
{
    if (obj.isNull())
    {
        return;
    }

    for (int i = 0; i < indent; i++)
    {
        printf("  ");
    }

    int id = obj.get("id", Json::nullValue).asInt();
    std::string name  = obj.get("name", Json::nullValue).asString();
    std::string type  = obj.get("type", Json::nullValue).asString();
    Json::Value shell = obj.get("shell", Json::nullValue);

    printf("#%d: %s \"%s\"", id, type.c_str(), name.c_str());

    if (!shell.isNull())
    {
        int pid = obj.get("pid", Json::nullValue).asInt();
        Json::Value app_id = obj.get("app_id", Json::nullValue);
        Json::Value window_props_obj = obj.get("window_properties", Json::nullValue);

        Json::Value instance = (window_props_obj != Json::nullValue) ?
            window_props_obj.get("instance", Json::nullValue) :
            Json::nullValue;

        Json::Value clazz = (window_props_obj != Json::nullValue) ?
            window_props_obj.get("class", Json::nullValue) :
            Json::nullValue;

        Json::Value x11_id = obj.get("window", Json::nullValue);

        printf(" (%s, pid: %d", shell.asString().c_str(), pid);
        if (!app_id.isNull())
        {
            printf(", app_id: \"%s\"", app_id.asString().c_str());
        }

        if (!instance.isNull())
        {
            printf(", instance: \"%s\"", instance.asString().c_str());
        }

        if (!clazz.isNull())
        {
            printf(", class: \"%s\"", clazz.asString().c_str());
        }

        if (!x11_id.isNull())
        {
            if (x11_id.asInt() != 0)
            {
                printf(", X11 window: 0x%X", x11_id.asInt());
            }
        }

        printf(")");
    }

    printf("\n");

    Json::Value nodes_obj = obj.get("nodes", Json::nullValue);
    if ((nodes_obj.isNull() == false) && nodes_obj.isArray())
    {
        for (auto& node : nodes_obj)
        {
            pretty_print_tree(node, indent + 1);
        }
    }
}

static void pretty_print(int type, Json::Value resp)
{
    switch (type)
    {
      case IPC_SEND_TICK:
        return;

      case IPC_GET_VERSION:
        pretty_print_version(resp);
        return;

      case IPC_GET_CONFIG:
        pretty_print_config(resp);
        return;

      case IPC_GET_TREE:
        pretty_print_tree(resp, 0);
        return;

      case IPC_COMMAND:
      case IPC_GET_WORKSPACES:
      case IPC_GET_INPUTS:
      case IPC_GET_OUTPUTS:
      case IPC_GET_SEATS:
        break;

      default:
        std::cout << ipc_json::json_to_string(resp) << std::endl;
        // std::cout << resp << std::endl;
        return;
    }

    if (resp.isArray())
    {
        for (auto& obj : resp)
        {
            switch (type)
            {
              case IPC_COMMAND:
                pretty_print_cmd(obj);
                break;

              case IPC_GET_WORKSPACES:
                pretty_print_workspace(obj);
                break;

              case IPC_GET_INPUTS:
                pretty_print_input(obj);
                break;

              case IPC_GET_OUTPUTS:
                pretty_print_output(obj);
                break;

              case IPC_GET_SEATS:
                pretty_print_seat(obj);
                break;
            }
        }
    }
}

int main(int argc, char **argv)
{
    static bool quiet   = false;
    static bool raw     = false;
    static bool monitor = false;
    char *socket_path   = nullptr;
    char *cmdtype = nullptr;

    static const struct option long_options[] =
    {
        {
            "help", no_argument, nullptr, 'h'
        },
        {"monitor", no_argument, nullptr, 'm'},
        {"pretty", no_argument, nullptr, 'p'},
        {"quiet", no_argument, nullptr, 'q'},
        {"raw", no_argument, nullptr, 'r'},
        {"socket", required_argument, nullptr, 's'},
        {"type", required_argument, nullptr, 't'},
        {"version", no_argument, nullptr, 'v'},
        {0, 0, 0, 0}
    };

    const char *usage =
        "Usage: swaymsg [options] [message]\n"
        "\n"
        "  -h, --help             Show help message and quit.\n"
        "  -m, --monitor          Monitor until killed (-t SUBSCRIBE only)\n"
        "  -p, --pretty           Use pretty output even when not using a tty\n"
        "  -q, --quiet            Be quiet.\n"
        "  -r, --raw              Use raw output even if using a tty\n"
        "  -s, --socket <socket>  Use the specified socket.\n"
        "  -t, --type <type>      Specify the message type.\n"
        "  -v, --version          Show the version number and quit.\n";

    raw = !isatty(STDOUT_FILENO);

    while (1)
    {
        int option_index = 0;
        int c = getopt_long(argc, argv, "hmpqrs:t:v", long_options, &option_index);
        if (c == -1)
        {
            break;
        }

        switch (c)
        {
          case 'm': // Monitor
            monitor = true;
            break;

          case 'p': // Pretty
            raw = false;
            break;

          case 'q': // Quiet
            quiet = true;
            break;

          case 'r': // Raw
            raw = true;
            break;

          case 's': // Socket
            socket_path = strdup(optarg);
            break;

          case 't': // Type
            cmdtype = strdup(optarg);
            break;

          case 'v':
            std::cout << "wf-msg version " << WF_MSG_VERSION << std::endl;
            std::cout << "wayfire version " << WAYFIRE_VERSION << std::endl;
            std::cout << "wayfire abi version " << WAYFIRE_API_ABI_VERSION <<
                std::endl;
            exit(EXIT_SUCCESS);
            break;

          default:
            fprintf(stderr, "%s", usage);
            exit(EXIT_FAILURE);
        }
    }

    if (!cmdtype)
    {
        cmdtype = strdup("command");
    }

    if (!socket_path)
    {
        socket_path = get_socketpath();
        if (!socket_path)
        {
            if (quiet)
            {
                exit(EXIT_FAILURE);
            }

            client_abort("Unable to retrieve socket path");
        }
    }

    uint32_t type = IPC_COMMAND;

    if (strcasecmp(cmdtype, "command") == 0)
    {
        type = IPC_COMMAND;
    } else if (strcasecmp(cmdtype, "get_workspaces") == 0)
    {
        type = IPC_GET_WORKSPACES;
    } else if (strcasecmp(cmdtype, "get_seats") == 0)
    {
        type = IPC_GET_SEATS;
    } else if (strcasecmp(cmdtype, "get_inputs") == 0)
    {
        type = IPC_GET_INPUTS;
    } else if (strcasecmp(cmdtype, "get_outputs") == 0)
    {
        type = IPC_GET_OUTPUTS;
    } else if (strcasecmp(cmdtype, "get_tree") == 0)
    {
        type = IPC_GET_TREE;
    } else if (strcasecmp(cmdtype, "get_marks") == 0)
    {
        type = IPC_GET_MARKS;
    } else if (strcasecmp(cmdtype, "get_bar_config") == 0)
    {
        type = IPC_GET_BAR_CONFIG;
    } else if (strcasecmp(cmdtype, "get_version") == 0)
    {
        type = IPC_GET_VERSION;
    } else if (strcasecmp(cmdtype, "get_binding_modes") == 0)
    {
        type = IPC_GET_BINDING_MODES;
    } else if (strcasecmp(cmdtype, "get_binding_state") == 0)
    {
        type = IPC_GET_BINDING_STATE;
    } else if (strcasecmp(cmdtype, "get_config") == 0)
    {
        type = IPC_GET_CONFIG;
    } else if (strcasecmp(cmdtype, "send_tick") == 0)
    {
        type = IPC_SEND_TICK;
    } else if (strcasecmp(cmdtype, "subscribe") == 0)
    {
        type = IPC_SUBSCRIBE;
    } else
    {
        if (quiet)
        {
            exit(EXIT_FAILURE);
        }

        client_abort("Unknown message type %s", cmdtype);
    }

    free(cmdtype);

    if (monitor && (type != IPC_SUBSCRIBE))
    {
        if (!quiet)
        {
            LOGE("Monitor can only be used with -t SUBSCRIBE");
        }

        free(socket_path);
        return 1;
    }

#if STRICT_C
    char *command = nullptr;

    if (optind < argc)
    {
        command = join_args(argv + optind, argc - optind);
    } else
    {
        command = strdup("");
    }

#else
    std::string command;

    if (optind < argc)
    {
        command = ipc_tools::join_args(argv + optind, argc - optind);
    } else
    {
        command = "";
    }

#endif

    int ret = 0;
    int socketfd = ipc_open_socket(socket_path);
    struct timeval timeout = {
        .tv_sec = 3, .tv_usec = 0
    };
    ipc_set_recv_timeout(socketfd, timeout);
#if STRICT_C
    uint32_t len = (uint32_t)std::string(command).size();
    char *resp   = ipc_single_command(socketfd, type, command, &len);
#else
    uint32_t len = (uint32_t)command.size();
    char *resp   = ipc_single_command(socketfd, type, command.c_str(), &len);
#endif
    // pretty print the json
    Json::Value result = ipc_json::string_to_json(resp);

    if (result.isNull() || result.empty())
    {
        if (!quiet)
        {
            LOGE("failed to parse payload as json: \n%s", resp);
        }

        ret = 1;
    } else
    {
        if (!success(result, true))
        {
            ret = 2;
        }

        if (!quiet && ((type != IPC_SUBSCRIBE) || (ret != 0)))
        {
            if (raw)
            {
                std::cout << ipc_json::json_to_string(result) << std::endl;
            } else
            {
                pretty_print(type, result);
            }
        }
    }

#if STRICT_C
    free(command);
#endif
    free(resp);

    if ((type == IPC_SUBSCRIBE) && (ret == 0))
    {
        std::cout << "Monitoring. " << std::endl;
        // Remove the timeout for subscribed events
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;
        ipc_set_recv_timeout(socketfd, timeout);

        do {
            struct ipc_response *reply = ipc_recv_response(socketfd);
            if (!reply)
            {
                break;
            }

            Json::Value obj = ipc_json::string_to_json(reply->payload);
            if (obj.isNull() || obj.empty())
            {
                if (!quiet)
                {
                    LOGE("failed to parse payload as json: \n%s", resp);
                }

                ret = 1;

                break;
            } else if (quiet)
            {
                //
            } else
            {
                if (raw)
                {
                    std::cout << obj << std::endl;
                } else
                {
                    std::cout << ipc_json::json_to_string(obj) << std::endl;
                }

                fflush(stdout);
            }

            free_ipc_response(reply);
        } while (monitor);
    }

    close(socketfd);
    free(socket_path);
    return ret;
}
