#include "../command.hpp"
#include <json/value.h>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>

Json::Value cmd_list_outputs(Json::Value argv) 
{
    (void)argv;
    auto outputs = wf::get_core().output_layout->get_outputs();

    Json::Value array = Json::arrayValue;
    for (auto o : outputs)
    {
        array.append(ipc_json::describe_output(o));
    }

    return ipc_json::build_status(RETURN_SUCCESS, array);

}