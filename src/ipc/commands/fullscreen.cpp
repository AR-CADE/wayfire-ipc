#include "command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <strings.h>
#include <wayfire/core.hpp>
#include <wayfire/window-manager.hpp>

// fullscreen [enable|disable|toggle] [global]
Json::Value fullscreen_handler(int argc, char **argv, command_handler_context *ctx)
{
    Json::Value error;
    if ((error =
             ipc_command::checkarg(argc, "fullscreen", EXPECTED_AT_MOST,
                 2)) != Json::nullValue)
    {
        return error;
    }

    if (!wf::get_core().output_layout->get_num_outputs())
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Can't run this command while there's no outputs connected.");
    }

    Json::Value con = ctx->container;

    if (con.isNull())
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER);
    }

    wayfire_view view = ipc_command::container_get_view(con);

    if (view == nullptr)
    {
        return ipc_json::command_result(RETURN_NOT_FOUND);
    }

    auto toplevel_view = wf::toplevel_cast(view);
    if (toplevel_view == nullptr)
    {
        return ipc_json::command_result(RETURN_NOT_FOUND);
    }

    bool is_fullscreen = toplevel_view->toplevel()->current().fullscreen;
    bool enable = !is_fullscreen;

    if (argc >= 1)
    {
        if (strcasecmp(argv[0], "global") != 0)
        {
            enable = ipc_command::parse_boolean(argv[0], is_fullscreen);
        }
    }

    wf::get_core().default_wm->fullscreen_request(toplevel_view, view->get_output(), enable);

    return ipc_json::command_result(RETURN_SUCCESS);
}
