#include "output.hpp"
#include "ipc.h"
#include <json/value.h>
#include <string>

struct output_config *new_output_config(const char *name) {
	struct output_config *oc = (struct output_config *)calloc(1, sizeof(struct output_config));
	if (oc == nullptr) {
		return nullptr;
	}
	oc->name = strdup(name);
	if (oc->name == nullptr) {
		free(oc);
		return nullptr;
	}
/* 	oc->enabled = -1;
	oc->width = oc->height = -1;
	oc->refresh_rate = -1;
	oc->custom_mode = -1;
	oc->drm_mode.type = -1;
	oc->x = oc->y = -1;
	oc->scale = -1;
	oc->scale_filter = SCALE_FILTER_DEFAULT;
	oc->transform = -1;
	oc->subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
	oc->max_render_time = -1;
	oc->adaptive_sync = -1;
	oc->render_bit_depth = RENDER_BIT_DEPTH_DEFAULT; */
	return oc;
}

void free_output_config(struct output_config *oc) {
	if (!oc) {
		return;
	}
	if (oc->name)
	{
		free(oc->name);
	}
	oc->name = nullptr;
	//free(oc->background);
	//free(oc->background_option);
	free(oc);
}

Json::Value dpms_handler(int argc, char **argv, command_handler_context *ctx);

std::map<std::string, std::function<Json::Value(int argc, char **argv, command_handler_context *ctx)>> output_handler_map
{ 
	{"dpms", dpms_handler},  
};

Json::Value output_handler(int argc, char **argv, command_handler_context *ctx)
{
	Json::Value error;
    if ((error = ipc_command::checkarg(argc, "output", EXPECTED_AT_LEAST, 1)) != Json::nullValue) {
		return error;
	}

    auto output_config = new_output_config(argv[0]);
	ctx->output_config = output_config;
    error = ipc_json::build_status(RETURN_INVALID_PARAMETER);

    argc--; argv++;

    while (argc > 0) {
		ctx->leftovers.argc = 0;
		ctx->leftovers.argv = nullptr;

        auto cmd_handler = output_handler_map[*argv];

        if (cmd_handler == nullptr)
        {
            auto output = wf::get_core().get_active_output();
            command_signal data;
            data.argv = argv;
            output->emit_signal(std::string(*argv) + "-command", &data);
            break;
        }

        error.clear();
        error = cmd_handler(argc, argv, ctx);

        if (error["status"] != RETURN_SUCCESS)
        {
            goto fail;
        }

		argc = ctx->leftovers.argc;
		argv = ctx->leftovers.argv;
	}

    ctx->output_config = nullptr;
	ctx->leftovers.argc = 0;
	ctx->leftovers.argv = nullptr;

    return ipc_json::build_status(RETURN_SUCCESS);

fail:
	ctx->output_config = nullptr;
	free_output_config(output_config);
	return error;
}