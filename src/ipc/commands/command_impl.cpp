#include "command_impl.hpp"
#include "criteria.hpp"

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

        return ipc_json::command_result(RETURN_INVALID_PARAMETER, error.c_str());
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
    auto toplevel_view = wf::toplevel_cast(descendant);
    if (toplevel_view == nullptr)
    {
        return false;
    }

    if (auto chidlren = toplevel_view->children;
        std::any_of(chidlren.begin(), chidlren.end(), [&] (const auto & child)
    {
        return child == ancestor;
    }))
    {
        return true;
    }

    return false;
}

wayfire_view ipc_command::container_get_view(const Json::Value & container)
{
    if (container.isNull())
    {
        return nullptr;
    }

    auto id = container.get("id", Json::nullValue);

    if (id.isNull() || !id.isNumeric())
    {
        return nullptr;
    }

    return find_container(id.asInt());
}

void ipc_command::close_view_container(const Json::Value & container)
{
    auto view = container_get_view(container);

    if (view != nullptr)
    {
        view->close();
    }
}

void ipc_command::close_view_container_child(const Json::Value & container)
{
    close_view_container(container);

    Json::Value nodes_obj = container.get("nodes", Json::nullValue);
    if ((nodes_obj.isNull() == false) && nodes_obj.isArray())
    {
        for (const auto& node : nodes_obj)
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
                auto top = wf::toplevel_cast(view);

                if (top == nullptr)
                {
                    continue;
                }

                wf::point_t ws =
                    view->get_output()->wset()->
                    get_view_main_workspace(top);
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
        return ipc_json::command_result(RETURN_OUT_OF_RESOURCES);
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
                    res_list.append(ipc_json::command_result(RETURN_INVALID_PARAMETER,
                        error));
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
            res_list.append(ipc_json::command_result(RETURN_INVALID_PARAMETER));
            ipc_tools::free_argv(argc, argv);

            goto cleanup;
        }

        auto handler = core_handler(argc, const_cast<const char**>(argv));
        if (!handler)
        {
            std::string error = "Unknown command " + std::string(argv[0]);
            res_list.append(ipc_json::command_result(RETURN_NOT_FOUND, error.c_str()));
            ipc_tools::free_argv(argc, argv);
            goto cleanup;
        }

        if (!using_criteria)
        {
            auto out = wf::get_core().seat->get_active_output();
            assert(out != nullptr);
            if (auto active_view = wf::get_active_view_for_output(out);active_view != nullptr)
            {
                auto v = ipc_json::describe_view(active_view);
                set_config_node(v, false, &handler_context);
            }

            Json::Value res = handler(argc - 1, argv + 1, &handler_context);

            res_list.append(res);

            if (res["status"] != RETURN_SUCCESS)
            {
                ipc_tools::free_argv(argc, argv);
                goto cleanup;
            }
        } else if (containers.empty() || !containers.isArray())
        {
            res_list.append(ipc_json::command_result(RETURN_ABORTED,
                "No matching node."));
        } else
        {
            Json::Value fail_res = Json::nullValue;

            for (const auto& container : containers)
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

            res_list.append(!fail_res.isNull() ? fail_res : ipc_json::command_result(
                RETURN_SUCCESS));
        }

        ipc_tools::free_argv(argc, argv);
    } while (head);

cleanup:
    free(exec);
    return res_list;
}
