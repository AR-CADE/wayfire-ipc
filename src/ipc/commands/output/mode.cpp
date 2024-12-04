#include <cstdint>
#include <json/forwards.h>
#include <json/value.h>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>
#include "../command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>

static RETURN_STATUS set_mode(const wlr_output_mode& mode,
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
            entry.second.mode = mode;
            break;
        }
    }

    if (found)
    {
        wf::get_core().output_layout->apply_configuration(config);
    }

    return RETURN_SUCCESS;
}

Json::Value output_mode_handler(int argc, char **argv, command_handler_context *ctx)
{
    if (!ctx->output_config)
    {
        return ipc_json::command_result(RETURN_ABORTED, "Missing output config");
    }

    if (ctx->output_config->name == nullptr)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Output config name not set");
    }

    if (!argc)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Missing mode argument.");
    }

    int32_t use_custom_mode = -1;
    int32_t width  = -1;
    int32_t height = -1;
    float refresh_rate = -1;

    if (strcmp(argv[0], "--custom") == 0)
    {
        argv++;
        argc--;
        use_custom_mode = 1;
    } else
    {
        use_custom_mode = 0;
    }

    char *end;
    width = (int32_t)strtol(*argv, &end, 10);
    if (*end)
    {
        // Format is 1234x4321
        if (*end != 'x')
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                "Invalid mode width.");
        }

        ++end;
        height = (int32_t)strtol(end, &end, 10);
        if (*end)
        {
            if (*end != '@')
            {
                return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                    "Invalid mode height.");
            }

            ++end;
            refresh_rate = strtof(end, &end);
            if (strcasecmp("Hz", end) != 0)
            {
                return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                    "Invalid mode refresh rate.");
            }
        }
    } else
    {
        // Format is 1234 4321
        argc--;
        argv++;
        if (!argc)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                "Missing mode argument (height).");
        }

        height = (int32_t)strtol(*argv, &end, 10);
        if (*end)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                "Invalid mode height.");
        }
    }

    auto output = command::all_output_by_name_or_id(ctx->output_config->name);

    if (!output || !output->handle)
    {
        return ipc_json::command_result(RETURN_ABORTED, "Missing output");
    }

    struct wlr_output_mode const *selected_mode = nullptr;
    struct wlr_output_mode custom_mode;

    if (use_custom_mode == 1)
    {
        custom_mode.height    = height;
        custom_mode.width     = width;
        custom_mode.refresh   = refresh_rate;
        custom_mode.preferred = true;
        selected_mode = &custom_mode;
    } else
    {
        struct wlr_output_mode *mode = nullptr;
        wl_list_for_each(mode, &output->handle->modes, link)
        {
            if (mode != nullptr)
            {
                if (refresh_rate != -1)
                {
                    if (refresh_rate != mode->refresh)
                    {
                        continue;
                    }
                }

                if ((mode->height != height) || (mode->width != width))
                {
                    continue;
                }

                selected_mode = mode;
                break;
            }
        }

        if (selected_mode == nullptr)
        {
            return ipc_json::command_result(RETURN_ABORTED,
                "The specified mode is invalid or not found in the list of available modes of the specified output, use *--custom* to force this mode if you know what you're doing");
        }
    }

    RETURN_STATUS status = set_mode(*selected_mode, output);

    ctx->leftovers.argc = argc - 1;
    ctx->leftovers.argv = argv + 1;

    return ipc_json::command_result(status);
}
