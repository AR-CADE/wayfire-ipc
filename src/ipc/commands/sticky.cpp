#include "command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>

Json::Value sticky_handler(int argc, char **argv, command_handler_context *ctx)
{
    if (Json::Value error;(error =
                               ipc_command::checkarg(argc, "sticky", EXPECTED_EQUAL_TO,
                                   1)) != Json::nullValue)
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
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "No current container");
    }

    wayfire_view view = ipc_command::container_get_view(con);

    auto toplevel_view = wf::toplevel_cast(view);
    if (toplevel_view == nullptr)
    {
        return ipc_json::command_result(RETURN_ABORTED,
            "No toplevel for this container");
    }

    bool sticky = ipc_command::parse_boolean(argv[0], toplevel_view->sticky);

    toplevel_view->set_sticky(sticky);

    return ipc_json::command_result(RETURN_SUCCESS);
}
