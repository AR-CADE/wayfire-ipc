#include "../command_impl.hpp"
#include "ipc.h"
#include <cstdint>
#include <cstdio>
#include <json/json.h>
#include <json/value.h>
#include <map>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>

static RETURN_STATUS signal_power_cmd(Json::Value argv)
{
    auto output = wf::get_core().get_active_output();
    command_signal data;
    data.argv = argv;
    output->emit_signal("power-command", &data);

    return RETURN_SUCCESS;
}

Json::Value output_power_handler(int argc, char **argv, command_handler_context *ctx)
{
    if (!ctx->output_config)
    {
        return ipc_json::build_status(RETURN_ABORTED, Json::nullValue,
            "Missing output config");
    }

    if (ctx->output_config->name == nullptr)
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "Output config name not set");
    }

    if (!(argc > 0))
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "Missing power argument.");
    }

    Json::Value args = Json::arrayValue;
    args.append("output");
    args.append(ctx->output_config->name);
    args.append("power");
    for (int i = 0; i < argc; ++i)
    {
        args.append(std::string(argv[i]));
    }

    ctx->leftovers.argc = argc - 1;
    ctx->leftovers.argv = argv + 1;

    return ipc_json::build_status(signal_power_cmd(args));
}
