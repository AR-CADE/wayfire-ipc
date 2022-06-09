#include "command/command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <strings.h>
#include <wayfire/core.hpp>


Json::Value exit_handler(int argc, char **argv, command_handler_context *ctx)
{
    (void)argv;
    (void)ctx;

    Json::Value error;
    if ((error =
             ipc_command::checkarg(argc, "exit", EXPECTED_EQUAL_TO,
                 0)) != Json::nullValue)
    {
        return error;
    }

    wf::get_core().shutdown();

    return ipc_json::build_status(RETURN_SUCCESS);
}
