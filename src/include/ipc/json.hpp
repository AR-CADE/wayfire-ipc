#ifndef _WAYFIRE_IPC_JSON_H
#define _WAYFIRE_IPC_JSON_H

#include <ipc.h>
#include <json.hpp>
#include <wayfire/geometry.hpp>
#include <wayfire/output.hpp>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_compositor.h>
#include <xwayland_support.h>

class ipc_json : public json
{
  public:
    static Json::Value wayfire_point_to_json(wf::point_t point);
    static Json::Value describe_output(wf::output_t *output);
    static const char *output_subpixel_description(enum wl_output_subpixel subpixel);
    static const char *output_transform_description(
        enum wl_output_transform transform);
    static const char *output_adaptive_sync_status_description(
        enum wlr_output_adaptive_sync_status status);
    static Json::Value describe_disabled_output(wf::output_t *output);
    static Json::Value describe_input(
        nonstd::observer_ptr<wf::input_device_t> device);
    static const char *input_device_get_type_description(
        nonstd::observer_ptr<wf::input_device_t> device);
    static bool device_is_touchpad(nonstd::observer_ptr<wf::input_device_t> device);
    static Json::Value describe_libinput_device(struct libinput_device *device);
    static Json::Value get_version();
    static Json::Value describe_seat(wlr_seat *seat);
    static Json::Value args_to_json(char **argv, int argc);
    static Json::Value command_result(RETURN_STATUS status,
        const char *error = nullptr);
    static Json::Value describe_view(wayfire_view view);
    static Json::Value describe_dimension(wf::dimensions_t dimension);
    static Json::Value describe_wlr_box(wf::geometry_t geometry);
    static Json::Value describe_geometry(wf::geometry_t geometry);
    static Json::Value describe_point(wf::point_t point);
    static Json::Value describe_pointf(wf::pointf_t point);
    static Json::Value describe_wlr_surface_state(wlr_surface_state surface_state);
    static Json::Value describe_wlr_surface(wlr_surface *surface);
#if HAVE_XWAYLAND
    static Json::Value describe_wlr_xwayland_surface(wlr_xwayland_surface *surface);
#endif
    static Json::Value describe_wl_client(wl_client *client);
    static Json::Value describe_wl_display(wl_display *display);
    static Json::Value create_empty_rect();
    static const char *orientation_description(enum wl_output_transform transform);
    static Json::Value get_abi_version();
    static const char *orientation_description(wlr_output *wlr_output);
    static const char *orientation_description(wf::output_t *output);
    static Json::Value describe_workspace(wf::point_t point, wf::output_t *output);
    static Json::Value get_tree();
    static const char *scale_filter_to_string(
        enum ipc_scale_filter_mode scale_filter);
    static Json::Value get_workspaces_nodes(wf::output_t *output);
    static Json::Value get_container_nodes(wf::point_t point, wf::output_t *output);
    static Json::Value get_view_nodes(wayfire_view view, bool floating = false);
    static Json::Value get_top_view_nodes(wf::point_t point, wf::output_t *output);
    static Json::Value get_shell_view_nodes(wf::output_t *output);
    static Json::Value describe_output_mode(struct wlr_output_mode *mode);

  private:
    static Json::Value get_root_node();
};

#endif
