#include "command_impl.hpp"


Json::Value exit_handler(int argc, char **argv, command_handler_context *ctx)
{
    (void)argv;
    (void)ctx;

    if (Json::Value error;(error =
                               ipc_command::checkarg(argc, "exit", EXPECTED_EQUAL_TO,
                                   0)) != Json::nullValue)
    {
        return error;
    }

    wf::get_core().shutdown();

    return ipc_json::command_result(RETURN_SUCCESS);
}
