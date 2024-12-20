#include "command_impl.hpp"

static void scratchpad_toggle_auto(wf::output_t *output)
{
    // Check if the currently focused window is a scratchpad window and should
    // be hidden again.
    auto focus = wf::get_active_view_for_output(output);
    if (focus)
    {
        auto toplevel_view = wf::toplevel_cast(focus);
        if (toplevel_view)
        {
            LOGD("Focus is a scratchpad window - hiding ",
                focus->get_title());
            wf::get_core().default_wm->minimize_request(toplevel_view, false);
            return;
        }
    }

    wf::point_t ws = output->wset()->get_current_workspace();

    if (ws == INVALID_WORKSPACE)
    {
        LOGD("No focused workspace to toggle scratchpad windows on");
        return;
    }

    Json::Value containers =
        ipc_json::get_i3_scratchpad_container_nodes_by_workspace(ws, output);

    // Check if there is an unfocused scratchpad window on the current workspace
    // and focus it.
    for (Json::ArrayIndex i = 0; i < containers.size(); ++i)
    {
        Json::Value container = containers.get(i, Json::nullValue);
        if (!container.isNull())
        {
            auto view = ipc_command::container_get_view(container);

            if (focus != view)
            {
                auto toplevel_view = wf::toplevel_cast(view);
                if (toplevel_view)
                {
                    LOGD("Focusing other scratchpad window (",
                        view->get_title(), ") in this workspace");
                    wf::get_core().default_wm->minimize_request(toplevel_view, true);
                    return;
                }
            }
        }
    }

    containers = ipc_json::get_i3_scratchpad_container_nodes_by_output(output);

    // Check if there is a visible scratchpad window on another workspace.
    // In this case we move it to the current workspace.
    for (Json::ArrayIndex i = 0; i < containers.size(); ++i)
    {
        Json::Value container = containers.get(i, Json::nullValue);
        if (!container.isNull())
        {
            auto view = ipc_command::container_get_view(container);

            if (focus != view)
            {
                auto toplevel_view = wf::toplevel_cast(view);
                if (toplevel_view)
                {
                    LOGD("Focusing other scratchpad window (",
                        view->get_title(), ") to this workspace");
                    output->wset()->move_to_workspace(toplevel_view, ws);
                    wf::get_core().default_wm->minimize_request(toplevel_view, true);
                    return;
                }
            }
        }
    }

    // Take the container at the bottom of the scratchpad list
    // if (!sway_assert(root->scratchpad->length, "Scratchpad is empty")) {
    // return;
    // }
    // struct sway_container *con = root->scratchpad->items[0];
    // sway_log(SWAY_DEBUG, "Showing %s from list", con->title);
    // root_scratchpad_show(con);
    // ipc_event_window(con, "move");
}

static void scratchpad_toggle_container(const wayfire_toplevel_view & toplevel_view)
{
    wf::get_core().default_wm->minimize_request(toplevel_view, !toplevel_view->minimized);
}

Json::Value scratchpad_handler(int argc, char **argv, command_handler_context *ctx)
{
    (void)ctx;

    if (Json::Value error;(error =
                               ipc_command::checkarg(argc, "scratchpad", EXPECTED_AT_LEAST,
                                   1)) != Json::nullValue)
    {
        return error;
    }

    if (strcmp(argv[0], "show") != 0)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Expected 'scratchpad show'");
    }

    if (ctx->node_overridden)
    {
        Json::Value con = ctx->container;

        auto view = ipc_command::container_get_view(ctx->container);

        if (!view)
        {
            return ipc_json::command_result(RETURN_SUCCESS);
        }

        auto toplevel_view = wf::toplevel_cast(view);
        if (!toplevel_view)
        {
            return ipc_json::command_result(RETURN_SUCCESS);
        }

        scratchpad_toggle_container(toplevel_view);
    } else
    {
        auto output = wf::get_core().seat->get_active_output();

        if (!output)
        {
            return ipc_json::command_result(RETURN_INVALID_PARAMETER,
                "Can't run this command while there's no outputs connected.");
        }

        scratchpad_toggle_auto(output);
    }

    return ipc_json::command_result(RETURN_SUCCESS);
}
