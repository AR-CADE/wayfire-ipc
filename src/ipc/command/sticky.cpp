#include "command/command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>

Json::Value sticky_handler(int argc, char **argv, command_handler_context *ctx)
{
    Json::Value error;
    if ((error =
             ipc_command::checkarg(argc, "sticky", EXPECTED_EQUAL_TO,
                 1)) != Json::nullValue)
    {
        return error;
    }

    if (!wf::get_core().output_layout->get_num_outputs())
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "Can't run this command while there's no outputs connected.");
    }

    Json::Value con = ctx->container;

    if (con.isNull())
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "No current container");
    }

    wayfire_view view = ipc_command::container_get_view(con);

    bool sticky = ipc_command::parse_boolean(argv[0], view->sticky);

    view->set_sticky(sticky);

    return ipc_json::build_status(RETURN_SUCCESS);
}
