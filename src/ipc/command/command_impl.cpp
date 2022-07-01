#include "command_impl.hpp"
#include "ipc.h"
#include <cstdint>
#include <json/value.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/geometry.hpp>
#include <wayfire/util/log.hpp>

bool ipc_command::parse_boolean(const char *boolean, bool current)
{
    if ((strcasecmp(boolean, "1") == 0) ||
        (strcasecmp(boolean, "yes") == 0) ||
        (strcasecmp(boolean, "on") == 0) ||
        (strcasecmp(boolean, "true") == 0) ||
        (strcasecmp(boolean, "enable") == 0) ||
        (strcasecmp(boolean, "enabled") == 0) ||
        (strcasecmp(boolean, "active") == 0))
    {
        return true;
    } else if (strcasecmp(boolean, "toggle") == 0)
    {
        return !current;
    }

    // All other values are false to match i3
    return false;
}

// Returns error object, or NULL if check succeeds.
Json::Value ipc_command::checkarg(int argc, const char *name,
    enum expected_args type, int val)
{
    Json::Value error_name = Json::nullValue;
    switch (type)
    {
      case EXPECTED_AT_LEAST:
        if (argc < val)
        {
            error_name = "at least ";
        }

        break;

      case EXPECTED_AT_MOST:
        if (argc > val)
        {
            error_name = "at most ";
        }

        break;

      case EXPECTED_EQUAL_TO:
        if (argc != val)
        {
            error_name = "";
        }
    }

    if (!error_name.isNull())
    {
        std::string error = "Invalid " + std::string(name) + " command "
                                                             "(expected " +
            error_name.asString() + std::to_string(val) + " argument" +
            std::string(val != 1 ? "s" : "") + ", got " + std::to_string(argc) + ")";

        return ipc_json::build_status(RETURN_INVALID_PARAMETER, Json::nullValue,
            error.c_str());
    }

    return error_name;
}

wayfire_view ipc_command::find_container(int con_id)
{
    auto views = wf::get_core().get_all_views();
    auto view  = std::find_if(views.begin(), views.end(), [&] (const wayfire_view& v)
    {
        return ((uint32_t)con_id == v->get_id());
    });

    if (view != views.end())
    {
        return *view;
    }

    return nullptr;
}

wayfire_view ipc_command::find_xwayland_container(int id)
{
    for (const wayfire_view& view : wf::get_core().get_all_views())
    {
        auto obj = ipc_json::describe_view(view);

        Json::Value shell = obj.get("shell", Json::nullValue);

        if (!shell.isNull())
        {
            Json::Value x11_id = obj.get("window", Json::nullValue);

            if (!x11_id.isNull())
            {
                if (x11_id.asInt() != 0)
                {
                    if (id == x11_id.asInt())
                    {
                        return view;
                    }
                }
            }
        }
    }

    return nullptr;
}

bool ipc_command::container_has_ancestor(wayfire_view descendant,
    wayfire_view ancestor)
{
    auto chidlren = descendant->children;

    if (std::any_of(chidlren.begin(), chidlren.end(), [&] (const auto & child)
    {
        return child == ancestor;
    }))
    {
        return true;
    }

    return false;
}

wayfire_view ipc_command::container_get_view(Json::Value container)
{
    int id = container.get("id", Json::nullValue).asInt();
    return find_container(id);
}

void ipc_command::close_view_container(Json::Value container)
{
    auto view = container_get_view(container);

    if (view != nullptr)
    {
        view->close();
    }
}

void ipc_command::close_view_container_child(Json::Value container)
{
    close_view_container(container);

    Json::Value nodes_obj = container.get("nodes", Json::nullValue);
    if ((nodes_obj.isNull() == false) && nodes_obj.isArray())
    {
        for (auto& node : nodes_obj)
        {
            close_view_container_child(node);
        }
    }
}

static void set_config_node(const Json::Value& node, bool node_overridden,
    command_handler_context *handler_context)
{
    if (handler_context == nullptr)
    {
        return;
    }

    handler_context->node = node;
    handler_context->container = Json::nullValue;
    handler_context->workspace = Json::nullValue;
    handler_context->node_overridden = node_overridden;

    if (node.isNull())
    {
        return;
    }

    std::string type = node.get("type", Json::nullValue).asString();

    if ((type == "con") || (type == "floating_con"))
    {
        handler_context->container = node;
        int id = node.get("id", Json::nullValue).asInt();
        for (const wayfire_view& view : wf::get_core().get_all_views())
        {
            if ((uint32_t)id != view->get_id())
            {
                wf::point_t ws =
                    wf::get_core().get_active_output()->workspace->
                    get_view_main_workspace(
                        view);
                handler_context->workspace = ipc_json::describe_workspace(ws,
                    view->get_output());
            }
        }
    } else if (type == "workspace")
    {
        handler_context->workspace = node;
    }
}

Json::Value ipc_command::execute_command(const std::string& _exec)
{
    char matched_delim  = ';';
    bool using_criteria = false;
    char *exec = strdup(_exec.c_str());
    char *head = exec;
    Json::Value res_list;
    command_handler_context handler_context;

    Json::Value containers;

    auto seat = wf::get_core().get_current_seat();
    assert(seat != nullptr);

    if (exec == nullptr)
    {
        return ipc_json::build_status(RETURN_OUT_OF_RESOURCES);
    }

    handler_context.seat = seat;

    do {
        for (; isspace(*head); ++head)
        {}

        if (matched_delim == ';')
        {
            using_criteria = false;
            if (*head == '[')
            {
                char *error = nullptr;
                criteria criteria;
                bool parsed = criteria.parse(head, &error);

                if (!parsed)
                {
                    res_list.append(ipc_json::build_status(RETURN_INVALID_PARAMETER,
                        Json::nullValue, error));
                    free(error);
                    goto cleanup;
                }

                containers = criteria.get_containers();
                head += strlen(criteria.raw);
                using_criteria = true;

                // Skip leading whitespace
                for (; isspace(*head); ++head)
                {}
            }
        }

        // Split command list
        char *cmd = ipc_tools::argsep(&head, ";,", &matched_delim);
        for (; isspace(*cmd); ++cmd)
        {}

        if (strcmp(cmd, "") == 0)
        {
            LOGI("Ignoring empty command.");
            continue;
        }

        LOGI("Handling command ", cmd);

        // TODO better handling of argv
        int argc    = 0;
        char **argv = ipc_tools::split_args(cmd, &argc);
        if ((strcmp(argv[0], "exec") != 0) &&
            (strcmp(argv[0], "exec_always") != 0) &&
            (strcmp(argv[0], "mode") != 0))
        {
            for (int i = 1; i < argc; ++i)
            {
                if ((*argv[i] == '\"') || (*argv[i] == '\''))
                {
                    ipc_tools::strip_quotes(argv[i]);
                }
            }
        }

        if (!(argc > 0))
        {
            res_list.append(ipc_json::build_status(RETURN_INVALID_PARAMETER));
            ipc_tools::free_argv(argc, argv);

            goto cleanup;
        }

        auto handler = core_handler(argc, const_cast<const char**>(argv));
        if (!handler)
        {
            auto output = wf::get_core().get_active_output();
            command_signal data;
            Json::Value obj;
            for (int i = 0; i < argc; ++i)
            {
                obj.append(argv[i]);
            }

            data.argv = obj;
            auto arg = obj.get(Json::ArrayIndex(0), Json::nullValue);
            if (!arg.isNull() && !arg.empty() && arg.isString())
            {
                output->emit_signal(arg.asString() + "-command", &data);
            }

            std::string error = "Unknown command " + arg.asString();
            res_list.append(ipc_json::build_status(RETURN_SUCCESS, Json::nullValue,
                error.c_str()));
            ipc_tools::free_argv(argc, argv);
            goto cleanup;
        }

        if (!using_criteria)
        {
            auto out = wf::get_core().get_active_output();
            assert(out != nullptr);
            auto top_view = out->get_top_view();
            if (top_view != nullptr)
            {
                auto v = ipc_json::describe_view(top_view);
                set_config_node(v, false, &handler_context);
            }

            Json::Value res = handler(argc - 1, argv + 1, &handler_context);

            res_list.append(res);

            if (res["status"] != RETURN_SUCCESS)
            {
                ipc_tools::free_argv(argc, argv);
                goto cleanup;
            }
        } else if ((containers.size() == 0) || !containers.isArray())
        {
            res_list.append(ipc_json::build_status(RETURN_ABORTED, Json::nullValue,
                "No matching node."));
        } else
        {
            Json::Value fail_res = Json::nullValue;

            for (auto& container : containers)
            {
                set_config_node(container, true, &handler_context);

                Json::Value res = handler(argc - 1, argv + 1, &handler_context);

                if (res["status"] == RETURN_SUCCESS)
                {
                    //
                } else
                {
                    // last failure will take precedence
                    if (!fail_res.isNull())
                    {
                        fail_res.clear();
                        fail_res = Json::nullValue;
                    }

                    fail_res.copy(res);
                    if (fail_res["status"] != RETURN_SUCCESS)
                    {
                        res_list.append(fail_res);
                        ipc_tools::free_argv(argc, argv);
                        goto cleanup;
                    }
                }
            }

            res_list.append(!fail_res.isNull() ? fail_res : ipc_json::build_status(
                RETURN_SUCCESS));
        }

        ipc_tools::free_argv(argc, argv);
    } while (head);

cleanup:
    free(exec);
    return res_list;
}
