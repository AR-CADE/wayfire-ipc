#include <json/forwards.h>
#include <json/value.h>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include "commands/commands.hpp"


static Json::Value set_scale_bulk(Json::Value argv, Json::Value value, Json::Value ids = Json::arrayValue)
{
    if (!value.isNumeric())
    {
        return ipc_json::command_return(getCmdBase(argv), argv, RETURN_INVALID_PARAMETER);
    }

    if (ids.empty())
    {
        return ipc_json::command_return(getCmdBase(argv), argv, RETURN_INVALID_PARAMETER);
    }

    auto config = wf::get_core().output_layout->get_current_configuration();

    for (auto& entry : config)
    {
        if (ids.isArray())
        {
            auto output = wf::get_core().output_layout->find_output(entry.first);
            
            bool found = false;

            for (auto id : ids)
            {
                if (id.isNumeric() && !(id.asInt() < 0) &&
                    (id.asUInt() == output->get_id()))
                {
                    found = true;
                }
            }                    

            if (found == false)
            {
                continue;
            }
        }       

        entry.second.scale = value.asDouble();
    }

    wf::get_core().output_layout->apply_configuration(config);

    return ipc_json::command_return(getCmdBase(argv), argv, RETURN_SUCCESS);
}

Json::Value cmd_scale(Json::Value argv) {
    Json::Value scale = argv.get(Json::ArrayIndex(3), 1);
    Json::Value ids = _get_ids_at_index(argv, Json::ArrayIndex(1));

    if (!scale.isNumeric())
    {
        return ipc_json::command_return(getCmdBase(argv), argv,
            RETURN_INVALID_PARAMETER);
    }

    return set_scale_bulk(argv, scale, ids);
}
