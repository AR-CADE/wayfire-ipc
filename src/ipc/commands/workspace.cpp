#include "command_impl.hpp"
#include "ipc.h"
#include "ipc/tools.hpp"
#include <json/value.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/geometry.hpp>

void workspace_switch(wf::point_t ws, wf::output_t *output)
{
    output->wset()->request_workspace(ws);
}

wf::point_t workspace_by_number(int index, wf::output_t *output)
{
    return ipc_tools::get_workspace_at_index(index, output);
}

wf::point_t workspace_prev(int index, wf::point_t current, wf::output_t *output)
{
    if (auto prev = ipc_tools::get_workspace_at_index(index - 1, output);prev != INVALID_WORKSPACE)
    {
        return prev;
    }

    return current;
}

wf::point_t workspace_next(int index, wf::point_t current, wf::output_t *output)
{
    if (auto next = ipc_tools::get_workspace_at_index(index + 1, output);next != INVALID_WORKSPACE)
    {
        return next;
    }

    return current;
}

wf::point_t workspace_by_name(const char *name, wf::output_t *output)
{
    wf::point_t current = output->wset()->get_current_workspace();

    if (strcmp(name, "prev") == 0)
    {
        int workspace_index = ipc_tools::get_workspace_index(current, output);
        return workspace_prev(workspace_index, current, output);
    } else if (strcmp(name, "prev_on_output") == 0)
    {
        // unsupported
        return current;
    } else if (strcmp(name, "next") == 0)
    {
        int workspace_index = ipc_tools::get_workspace_index(current, output);
        return workspace_next(workspace_index, current, output);
    } else if (strcmp(name, "next_on_output") == 0)
    {
        // unsupported
        return current;
    } else if (strcmp(name, "current") == 0)
    {
        return current;
    } else if (strcasecmp(name, "back_and_forth") == 0)
    {
        // unsupported
        return current;
    } else
    {
        int workspace_index = std::stoi(name);
        return workspace_by_number(workspace_index, output);
    }
}

Json::Value workspace_handler(int argc, char **argv, command_handler_context *ctx)
{
    (void)ctx;

    if (Json::Value error;(error =
                               ipc_command::checkarg(argc, "workspace", EXPECTED_AT_LEAST,
                                   1)) != Json::nullValue)
    {
        return error;
    }

    auto output = wf::get_core().seat->get_active_output();

    wf::point_t ws;
    if (strcasecmp(argv[0], "number") == 0)
    {
        if (argc < 2)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                "Expected workspace number");
        }

        if (!isdigit(argv[1][0]))
        {
            std::ostringstream stringStream;
            stringStream << "Invalid workspace number ";
            stringStream << argv[1];

            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                stringStream.str().c_str());
        }

        ws = workspace_by_number(std::stoi(argv[1]), output);
    } else if ((strcasecmp(argv[0], "next") == 0) ||
               (strcasecmp(argv[0], "prev") == 0) ||
               (strcasecmp(argv[0], "next_on_output") == 0) ||
               (strcasecmp(argv[0], "prev_on_output") == 0) ||
               (strcasecmp(argv[0], "current") == 0))
    {
        ws = workspace_by_name(argv[0], output);
    } else
    {
        std::string name = ipc_tools::join_args(argv, argc);
        ws = workspace_by_name(name.c_str(), output);
    }

    if (!output->wset()->is_workspace_valid(ws))
    {
        return ipc_json::command_result(RETURN_ABORTED, "No workspace to switch to");
    }

    workspace_switch(ws, output);

    return ipc_json::command_result(RETURN_SUCCESS);
}
