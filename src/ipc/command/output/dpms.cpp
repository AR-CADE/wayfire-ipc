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

static RETURN_STATUS signal_dpms_cmd(Json::Value argv) 
{
    auto output = wf::get_core().get_active_output();
    command_signal data;
    data.argv = argv;
    output->emit_signal("dpms-command", &data);

    return RETURN_SUCCESS;
}

#if 0
static RETURN_STATUS set_state_off(Json::Value ids = Json::arrayValue)
{
    bool apply = false;
    auto config = wf::get_core().output_layout->get_current_configuration();

    for (auto& entry : config)
    {
       /*  if (ids.isArray() && ids.empty() == false)
        {
            auto o = wf::get_core().output_layout->find_output(entry.first);
            
            bool found = false;

            for (auto id : ids)
            {
                if (id.isNumeric() && !(id.asInt() < 0) &&
                    (id.asUInt() == o->get_id()))
                {
                    found = true;
                }
            }                    

            if (found == false)
            {
                continue;
            }
        } */

        if (entry.second.source != wf::OUTPUT_IMAGE_SOURCE_DPMS)
        {
            entry.second.source = wf::OUTPUT_IMAGE_SOURCE_DPMS;
            apply = true;
        }

        return RETURN_NOT_FOUND;
    }

    if (apply)
    {
        wf::get_core().output_layout->apply_configuration(config);
        return RETURN_SUCCESS;
    }
    return RETURN_NOT_FOUND;

}

static RETURN_STATUS set_state_on(Json::Value ids = IPC_ANY_TOCKEN)
{
    bool apply = false;
    auto config = wf::get_core().output_layout->get_current_configuration();

    for (auto& entry : config)
    {
        /* if (ids.isArray() && ids.empty() == false)
        {
            auto o = wf::get_core().output_layout->find_output(entry.first);
            
            bool found = false;

            for (auto id : ids)
            {
                if (id.isNumeric() && !(id.asInt() < 0) &&
                    (id.asUInt() == o->get_id()))
                {
                    found = true;
                }
            }                    

            if (found == false)
            {
                continue;
            }
        }  */

        if (entry.second.source != wf::OUTPUT_IMAGE_SOURCE_SELF)
        {
            entry.second.source = wf::OUTPUT_IMAGE_SOURCE_SELF;
            apply = true;
        }
    }

    if (apply)
    {
        wf::get_core().output_layout->apply_configuration(config);
        return RETURN_SUCCESS;
    }
    return RETURN_NOT_FOUND;
}
#endif

Json::Value dpms_handler(int argc, char **argv, command_handler_context *ctx)
{
    if (!ctx->output_config) 
    {
		return ipc_json::build_status(RETURN_ABORTED, Json::nullValue, "Missing output config");
	}

	if (!(argc > 0)) 
    {
        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue, "Missing dpms argument.");
	}

    Json::Value args = Json::arrayValue;
    args.append("output");
    args.append(ctx->output_config->name);
    for (int i = 0; i < argc; ++i)
    {
        args.append(std::string(argv[i]));
    }

/*     Json::Value option = args.get(Json::ArrayIndex(0), IPC_ON_TOCKEN);
    if (option.isString())
    {
        if (option.asString() == "toggle")
        {

            const char *oc_name = ctx->output_config->name;
       

             wf::output_t *output = all_output_by_name_or_id(oc_name);
            if (!output || !output->handle) {
                std::string error = "Cannot apply toggle to unknown output " + std::string(oc_name);
                return ipc_json::build_status(RETURN_ABORTED, Json::nullValue, error.c_str());
            }

            if (output. && !output->handle->enabled) {
                current_dpms = DPMS_OFF;
            }
        }

        if (option.asString() == "toggle")
        {

            const char *oc_name = ctx->output_config->name;
        

             wf::output_t *output = all_output_by_name_or_id(oc_name);
            if (!output || !output->handle) {
                std::string error = "Cannot apply toggle to unknown output " + std::string(oc_name);
                return ipc_json::build_status(RETURN_ABORTED, Json::nullValue, error.c_str());
            }

            if (output. && !output->handle->enabled) {
                current_dpms = DPMS_OFF;
            }
        }
    }



    if (option.isBool() && option.asBool() == "toggle")
	//if (parse_boolean(argv[0], current_dpms == DPMS_ON)) 
    {
		ctx->output_config->dpms_state = DPMS_ON;
	} else {
		ctx->output_config->dpms_state = DPMS_OFF;
	} */

	ctx->leftovers.argc = argc - 1;
	ctx->leftovers.argv = argv + 1;





/*     enum config_dpms current_dpms = DPMS_ON;

	if (strcasecmp(argv[0], "toggle") == 0) {

		const char *oc_name = ctx->output_config->name;
		if (strcmp(oc_name, "*") == 0) {
            return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue, "Cannot apply toggle to all outputs.");
		}

		auto output = ipc_command::all_output_by_name_or_id(oc_name);

		if (!output || !output->first) {
            std::string error = "Cannot apply toggle to unknown output " + std::string(oc_name);
            return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue, error.c_str());
		}

		if (output->second.source == wf::OUTPUT_IMAGE_SOURCE_SELF && !output->first->enabled) {
			current_dpms = DPMS_OFF;
		}
	}

	if (ipc_command::parse_boolean(argv[0], current_dpms == DPMS_ON)) {
		ctx->output_config->dpms_state = DPMS_ON;
	} else {
		ctx->output_config->dpms_state = DPMS_OFF;
	}

	ctx->leftovers.argc = argc - 1;
	ctx->leftovers.argv = argv + 1; */

    return ipc_json::build_status(signal_dpms_cmd(args));
}