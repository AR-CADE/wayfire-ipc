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
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Can't run this command while there's no outputs connected.");
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
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "opacity float invalid");
    }

    wf::view_2D *transform_2d = nullptr;

    auto transform = view->get_transformer("opacity");
    if (transform)
    {
        transform_2d =
            dynamic_cast<wf::view_2D*>(transform.get());
    }

    if (!strcasecmp(argv[0], "plus"))
    {
        val = alpha + val;
    } else if (!strcasecmp(argv[0], "minus"))
    {
        val = alpha - val;
    } else if ((argc > 1) && strcasecmp(argv[0], "set"))
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "Expected: set|plus|minus <0..1>");
    } else
    {
        alpha = val;
    }

    if ((val < 0) || (val > 1))
    {
        return ipc_json::command_result(RETURN_INVALID_PARAMETER, "opacity value out of bounds");
    }

    if (val == 1)
    {
        if (transform_2d)
        {
            view->pop_transformer("opacity");
        }
    } else
    {
        if (transform_2d)
        {
            transform_2d->alpha = alpha;
        } else
        {
            wf::view_2D *new_transform_2d = new wf::view_2D(view,
                wf::TRANSFORMER_HIGHLEVEL);
            new_transform_2d->alpha = alpha;
            view->add_transformer(std::unique_ptr<wf::view_2D>(
                new_transform_2d), "opacity");
        }
    }

    view->damage();

    return ipc_json::command_result(RETURN_SUCCESS);
}
