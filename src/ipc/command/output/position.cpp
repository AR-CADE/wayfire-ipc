#include <json/forwards.h>
#include <json/value.h>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>
#include "../command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>

static RETURN_STATUS set_position(wf::output_config::position_t position,
    wf::output_t *output)
{
    bool found = true;

    auto config = wf::get_core().output_layout->get_current_configuration();

    for (auto& entry : config)
    {
        auto o = wf::get_core().output_layout->find_output(entry.first);

        if (o->get_id() == output->get_id())
        {
            found = true;
            entry.second.position = position;
            break;
        }
    }

    if (found)
    {
        wf::get_core().output_layout->apply_configuration(config);
    }

    return RETURN_SUCCESS;
}

Json::Value output_position_handler(int argc, char **argv,
    command_handler_context *ctx)
{
    if (!ctx->output_config)
    {
        return ipc_json::command_result(RETURN_ABORTED, "Missing output config");
    }

    if (ctx->output_config->name == nullptr)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Output config name not set");
    }

    if (!argc)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Missing position argument.");
    }

    int32_t x = 0, y = 0;

    char *end;
    x = strtol(*argv, &end, 10);
    if (*end)
    {
        // Format is 1234,4321
        if (*end != ',')
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Invalid position x.");
        }

        ++end;
        y = strtol(end, &end, 10);
        if (*end)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Invalid position y.");
        }
    } else
    {
        // Format is 1234 4321 (legacy)
        argc--;
        argv++;
        if (!argc)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Missing position argument (y).");
        }

        y = strtol(*argv, &end, 10);
        if (*end)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Invalid position y.");
        }
    }

    auto output = command::all_output_by_name_or_id(ctx->output_config->name);

    if (!output)
    {
        return ipc_json::command_result(RETURN_ABORTED, "Missing output");
    }

    wf::output_config::position_t position(x, y);

    RETURN_STATUS status = set_position(position, output);

    ctx->leftovers.argc = argc - 1;
    ctx->leftovers.argv = argv + 1;

    return ipc_json::command_result(status);
}
