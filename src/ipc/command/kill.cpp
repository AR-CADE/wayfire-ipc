#include "command/command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>



Json::Value kill_handler(int argc, char **argv, command_handler_context *ctx)
{
    (void)argc;
    (void)argv;

    if (!wf::get_core().output_layout->get_num_outputs())
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Can't run this command while there's no outputs connected.");
    }

    Json::Value con = ctx->container;
    Json::Value ws  = ctx->workspace;

    if (con.isNull() == false)
    {
        ipc_command::close_view_container_child(con);
        ipc_command::close_view_container(con);
    } else
    {
        return ipc_json::command_result(RETURN_UNSUPPORTED,
            "Killing a workspace is unsupported for now.");
        // workspace_for_each_container(ws, close_container_iterator, NULL);
    }

    return ipc_json::command_result(RETURN_SUCCESS);
}
