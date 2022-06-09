#include <json/forwards.h>
#include <json/value.h>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>
#include "../command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <wayfire/core.hpp>

static RETURN_STATUS set_tranform(enum wl_output_transform transform,
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
            entry.second.transform = transform;
            break;
        }
    }

    if (found)
    {
        wf::get_core().output_layout->apply_configuration(config);
    }

    return RETURN_SUCCESS;
}

static enum wl_output_transform invert_rotation_direction(
    enum wl_output_transform t)
{
    switch (t)
    {
      case WL_OUTPUT_TRANSFORM_90:
        return WL_OUTPUT_TRANSFORM_270;

      case WL_OUTPUT_TRANSFORM_270:
        return WL_OUTPUT_TRANSFORM_90;

      case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        return WL_OUTPUT_TRANSFORM_FLIPPED_270;

      case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        return WL_OUTPUT_TRANSFORM_FLIPPED_90;

      default:
        return t;
    }
}

Json::Value transform_handler(int argc, char **argv, command_handler_context *ctx)
{
    if (!ctx->output_config)
    {
        return ipc_json::build_status(RETURN_ABORTED, Json::nullValue,
            "Missing output config");
    }

    if (!argc)
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "Missing transform argument.");
    }

    enum wl_output_transform transform;
    if ((strcmp(*argv, "normal") == 0) ||
        (strcmp(*argv, "0") == 0))
    {
        transform = WL_OUTPUT_TRANSFORM_NORMAL;
    } else if (strcmp(*argv, "90") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_90;
    } else if (strcmp(*argv, "180") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_180;
    } else if (strcmp(*argv, "270") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_270;
    } else if (strcmp(*argv, "flipped") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_FLIPPED;
    } else if (strcmp(*argv, "flipped-90") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_FLIPPED_90;
    } else if (strcmp(*argv, "flipped-180") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_FLIPPED_180;
    } else if (strcmp(*argv, "flipped-270") == 0)
    {
        transform = WL_OUTPUT_TRANSFORM_FLIPPED_270;
    } else
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            "Invalid output transform.");
    }

    // Sway uses clockwise transforms, while WL_OUTPUT_TRANSFORM_* describe
    // anti-clockwise transforms
    transform = invert_rotation_direction(transform);

    auto output_config   = ctx->output_config;
    wf::output_t *output = command::all_output_by_name_or_id(output_config->name);
    if (output == nullptr)
    {
        return ipc_json::build_status(RETURN_ABORTED, Json::nullValue,
            "Cannot apply relative transform to unknown output");
    }

    ctx->leftovers.argc = argc - 1;
    ctx->leftovers.argv = argv + 1;
    if ((argc > 1) &&
        ((strcmp(argv[1],
            "clockwise") == 0) || (strcmp(argv[1], "anticlockwise") == 0)))
    {
        if (output_config->name != nullptr)
        {
            return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
                "Output config name not set");
        }

        if (strcmp(output_config->name, "*") == 0)
        {
            return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
                "Cannot apply relative transform to all outputs.");
        }

        if (strcmp(argv[1], "anticlockwise") == 0)
        {
            transform = invert_rotation_direction(transform);
        }

        struct wlr_output *w_output = output->handle;
        transform = wlr_output_transform_compose(w_output->transform, transform);
        ctx->leftovers.argv += 1;
        ctx->leftovers.argc -= 1;
    }

    if (!output_config)
    {
        return ipc_json::build_status(RETURN_ABORTED, Json::nullValue,
            "Missing output");
    }

    RETURN_STATUS status = set_tranform(transform, output);

    return ipc_json::build_status(status);
}
