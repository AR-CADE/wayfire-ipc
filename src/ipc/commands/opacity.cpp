#include "command/command_impl.hpp"
#include "ipc.h"
#include <json/value.h>
#include <strings.h>
#include <wayfire/core.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/view-transform.hpp>

Json::Value opacity_handler(int argc, char **argv, command_handler_context *ctx)
{
    Json::Value error;
    if ((error =
             ipc_command::checkarg(argc, "opacity", EXPECTED_AT_LEAST,
                 1)) != Json::nullValue)
    {
        return error;
    }

    if (!wf::get_core().output_layout->get_num_outputs())
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Can't run this command while there's no outputs connected.");
    }

    Json::Value con = ctx->container;

    if (con.isNull())
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER);
    }

    wayfire_view view = ipc_command::container_get_view(con);

    if (view == nullptr)
    {
        return ipc_json::command_result(RETURN_NOT_FOUND);
    }

    char *err;
    float val   = strtof(argc == 1 ? argv[0] : argv[1], &err);
    float alpha = 0;
    if (*err)
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "opacity float invalid");
    }

    auto transform_2d =
        view->get_transformed_node()->get_transformer<wf::scene::view_2d_transformer_t>(
            "opacity");

    if (!strcasecmp(argv[0], "plus"))
    {
        val = alpha + val;
    } else if (!strcasecmp(argv[0], "minus"))
    {
        val = alpha - val;
    } else if ((argc > 1) && strcasecmp(argv[0], "set"))
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "Expected: set|plus|minus <0..1>");
    } else
    {
        alpha = val;
    }

    if ((val < 0) || (val > 1))
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER,
            "opacity value out of bounds");
    }

    if (val == 1)
    {
        if (transform_2d)
        {
            view->get_transformed_node()->rem_transformer("opacity");
        }
    } else
    {
        if (transform_2d)
        {
            transform_2d->alpha = alpha;
        } else
        {
            auto new_transform_2d =
                std::make_shared<wf::scene::view_2d_transformer_t>(view);
            new_transform_2d->alpha = alpha;
            view->get_transformed_node()->add_transformer(
                new_transform_2d, wf::TRANSFORMER_HIGHLEVEL, "opacity");
        }
    }

    view->damage();

    return ipc_json::command_result(RETURN_SUCCESS);
}
