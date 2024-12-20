#include "command_impl.hpp"


struct output_config *new_output_config(const char *name)
{
    auto oc =
        (struct output_config*)calloc(1, sizeof(struct output_config));
    if (oc == nullptr)
    {
        return nullptr;
    }

    oc->name = strdup(name);
    if (oc->name == nullptr)
    {
        free(oc);
        return nullptr;
    }

/*  oc->enabled = -1;
 *   oc->width = oc->height = -1;
 *   oc->refresh_rate = -1;
 *   oc->custom_mode = -1;
 *   oc->drm_mode.type = -1;
 *   oc->x = oc->y = -1;
 *   oc->scale = -1;
 *   oc->scale_filter = SCALE_FILTER_DEFAULT;
 *   oc->transform = -1;
 *   oc->subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
 *   oc->max_render_time = -1;
 *   oc->adaptive_sync = -1;
 *   oc->render_bit_depth = RENDER_BIT_DEPTH_DEFAULT; */
    return oc;
}

void free_output_config(struct output_config *oc)
{
    if (!oc)
    {
        return;
    }

    if (oc->name)
    {
        free(oc->name);
    }

    oc->name = nullptr;
    // free(oc->background);
    // free(oc->background_option);
    free(oc);
}

Json::Value output_power_handler(int argc, char **argv,
    command_handler_context *ctx);
Json::Value output_scale_handler(int argc, char **argv,
    command_handler_context *ctx);
Json::Value output_transform_handler(int argc, char **argv,
    command_handler_context *ctx);
Json::Value output_position_handler(int argc, char **argv,
    command_handler_context *ctx);
Json::Value output_mode_handler(int argc, char **argv, command_handler_context *ctx);

std::map<std::string, std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)>, std::less<>> const output_handler_map{{"dpms", output_power_handler},
    {"power", output_power_handler},
    {"scale", output_scale_handler},
    {"transform", output_transform_handler},
    {"position", output_position_handler},
    {"mode", output_mode_handler}, };


std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> output_handler_map_try_at(const std::string& k)
{
    try {
        return output_handler_map.at(k);
    } catch (const std::out_of_range& ex)
    {
        (void)ex;
        return nullptr;
    }
}

void cleanup(command_handler_context*& ctx)
{
    if (ctx->output_config != nullptr)
    {
        free_output_config(ctx->output_config);
    }

    ctx->output_config  = nullptr;
    ctx->leftovers.argc = 0;
    ctx->leftovers.argv = nullptr;
}

Json::Value output_handler(int argc, char **argv,
    command_handler_context *ctx)
{
    Json::Value error;
    if ((error = ipc_command::checkarg(argc, "output", EXPECTED_AT_LEAST, 1)) !=
        Json::nullValue)
    {
        return error;
    }

    auto output_config = new_output_config(argv[0]);
    if (output_config == nullptr)
    {
        return ipc_json::command_result(RETURN_OUT_OF_RESOURCES);
    }

    ctx->output_config = output_config;
    error = ipc_json::command_result(RETURN_INVALID_PARAMETER);

    argc--;
    argv++;

    while (argc > 0)
    {
        ctx->leftovers.argc = 0;
        ctx->leftovers.argv = nullptr;

        auto cmd_handler = output_handler_map_try_at(*argv);

        if (cmd_handler == nullptr)
        {
            error = ipc_json::command_result(RETURN_NOT_FOUND);
            goto fail;
        }

        error.clear();
        error = cmd_handler(argc - 1, argv + 1, ctx);

        if (error["status"] != RETURN_SUCCESS)
        {
            goto fail;
        }

        argc = ctx->leftovers.argc;
        argv = ctx->leftovers.argv;
    }

    cleanup(ctx);

    return ipc_json::command_result(RETURN_SUCCESS);

fail:
    cleanup(ctx);

    return error;
}
