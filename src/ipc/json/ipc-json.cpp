#include <ipc/tools.hpp>
#include <ipc/json.hpp>
#include <json/value.h>
#include <libinput.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/input-device.hpp>
#include <wayfire/output.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/plugins/common/workspace-wall.hpp>
#include <wayfire/view.hpp>
#include <wayfire/workspace-manager.hpp>
// #include <wlr/backend/libinput.h>


wf::option_wrapper_t<bool> xwayland_enabled("core/xwayland");
wf::option_wrapper_t<int> max_render_time{"core/max_render_time"};
wf::option_wrapper_t<bool> dynamic_delay{"workarounds/dynamic_repaint_delay"};

wf::option_wrapper_t<int> keyboard_repeat_delay{"input/kb_repeat_delay"};
wf::option_wrapper_t<int> keyboard_repeat_rate{"input/kb_repeat_rate"};
wf::option_wrapper_t<double> mouse_scroll_speed{"input/mouse_scroll_speed"};

Json::Value ipc_json::get_abi_version()
{
    Json::Value version;
    struct tm tm;

    std::string version_string = std::to_string(WAYFIRE_API_ABI_VERSION);

    version["human_readable"] = version_string.c_str();
    version["version"] = WAYFIRE_API_ABI_VERSION;

    if (strptime(version_string.c_str(), "%Y%m%d", &tm) == nullptr)
    {
        return version;
    }

    version["year"]  = 1900 + tm.tm_year;
    version["month"] = tm.tm_mon + 1;
    version["day"]   = tm.tm_mday;

    return version;
}

Json::Value ipc_json::get_version()
{
    int major = 0, minor = 0, patch = 0;
    Json::Value version;

    sscanf(WAYFIRE_VERSION, "%d.%d.%d", &major, &minor, &patch);
    version["human_readable"] = WAYFIRE_VERSION;

    // oups ... yet another variant ...
    version["variant"] = "wayfire";

    version["major"] = major;
    version["minor"] = minor;
    version["patch"] = patch;
    version["abi"]   = get_abi_version();

    /*
     *  Json::Value xml_dirs = Json::arrayValue;
     *  for (std::string dir : wf::get_core().config_backend->get_xml_dirs())
     * xml_dirs.append(dir);
     *
     *  version["loaded_config_file_names"] = xml_dirs;
     */

    return version;
}

Json::Value ipc_json::create_empty_rect()
{
    struct wlr_box empty = {
        0, 0, 0, 0
    };

    return describe_wlr_box(empty);
}

Json::Value ipc_json::wayfire_point_to_json(wf::point_t point)
{
    Json::Value root;
    root["x"] = point.x;
    root["y"] = point.y;

    return root;
}

Json::Value ipc_json::args_to_json(char **argv, int argc)
{
    Json::Value cmds = Json::arrayValue;

    if (argv != nullptr)
    {
        for (int i = 0; i < argc; ++i)
        {
            Json::Value cmd;
            auto arg = argv[i];
            if (arg != nullptr)
            {
                std::string s = std::string(arg);

                std::vector<std::string> cmd_call = ipc_tools::split_string(s, "=");
                cmd["cmd"] = cmd_call[0];

                if (cmd_call.size() > 1)
                {
                    Json::Value options = Json::arrayValue;
                    std::vector<std::string> cmd_args =
                        ipc_tools::split_string(cmd_call[1], ":");

                    for (std::string cmd_arg : cmd_args)
                    {
                        options.append(cmd_arg);
                    }

                    cmd["options"] = options;
                }

                cmds.append(cmd);
            }
        }
    }

    return cmds;
}

const char*ipc_json::scale_filter_to_string(enum ipc_scale_filter_mode scale_filter)
{
    switch (scale_filter)
    {
      case SCALE_FILTER_DEFAULT:
        return "smart";

      case SCALE_FILTER_LINEAR:
        return "linear";

      case SCALE_FILTER_NEAREST:
        return "nearest";

      case SCALE_FILTER_SMART:
        return "smart";
    }

    return "unknown";
}

const char*ipc_json::output_transform_description(enum wl_output_transform transform)
{
    switch (transform)
    {
      case WL_OUTPUT_TRANSFORM_NORMAL:
        return "normal";

      case WL_OUTPUT_TRANSFORM_90:
        return "90";

      case WL_OUTPUT_TRANSFORM_180:
        return "180";

      case WL_OUTPUT_TRANSFORM_270:
        return "270";

      case WL_OUTPUT_TRANSFORM_FLIPPED:
        return "flipped";

      case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        return "flipped-90";

      case WL_OUTPUT_TRANSFORM_FLIPPED_180:
        return "flipped-180";

      case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        return "flipped-270";
    }

    return "unknown";
}

const char*ipc_json::output_adaptive_sync_status_description(
    enum wlr_output_adaptive_sync_status status)
{
    switch (status)
    {
      case WLR_OUTPUT_ADAPTIVE_SYNC_DISABLED:
        return "disabled";

      case WLR_OUTPUT_ADAPTIVE_SYNC_ENABLED:
        return "enabled";

      default:
        break;
    }

    return nullptr;
}

const char*ipc_json::output_subpixel_description(enum wl_output_subpixel subpixel)
{
    switch (subpixel)
    {
      case WL_OUTPUT_SUBPIXEL_UNKNOWN:
        return "unknown";

      case WL_OUTPUT_SUBPIXEL_NONE:
        return "none";

      case WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB:
        return "rgb";

      case WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR:
        return "bgr";

      case WL_OUTPUT_SUBPIXEL_VERTICAL_RGB:
        return "vrgb";

      case WL_OUTPUT_SUBPIXEL_VERTICAL_BGR:
        return "vbgr";
    }

    return "unknown";
}

bool ipc_json::device_is_touchpad(
    nonstd::observer_ptr<wf::input_device_t> device)
{
    (void)device;
    return false;
}

const char*ipc_json::input_device_get_type_description(
    nonstd::observer_ptr<wf::input_device_t> device)
{
    if (device == nullptr)
    {
        return "unknown";
    }

    wlr_input_device *wlr_handle = device->get_wlr_handle();
    if (wlr_handle == nullptr)
    {
        return "unknown";
    }

    switch (wlr_handle->type)
    {
      case WLR_INPUT_DEVICE_POINTER:
        if (device_is_touchpad(device))
        {
            return "touchpad";
        } else
        {
            return "pointer";
        }

      case WLR_INPUT_DEVICE_KEYBOARD:
        return "keyboard";

      case WLR_INPUT_DEVICE_TOUCH:
        return "touch";

      case WLR_INPUT_DEVICE_TABLET_TOOL:
        return "tablet_tool";

      case WLR_INPUT_DEVICE_TABLET_PAD:
        return "tablet_pad";

      case WLR_INPUT_DEVICE_SWITCH:
        return "switch";
    }

    return "unknown";
}

const char*ipc_json::orientation_description(wf::output_t *output)
{
    if (output != nullptr)
    {
        return orientation_description(output->handle);
    }

    return "none";
}

const char*ipc_json::orientation_description(wlr_output *wlr_output)
{
    if (wlr_output != nullptr)
    {
        return orientation_description(wlr_output->transform);
    }

    return "none";
}

const char*ipc_json::orientation_description(enum wl_output_transform transform)
{
    switch (transform)
    {
      case WL_OUTPUT_TRANSFORM_180:
      case WL_OUTPUT_TRANSFORM_NORMAL:
      case WL_OUTPUT_TRANSFORM_FLIPPED:
      case WL_OUTPUT_TRANSFORM_FLIPPED_180:
        return "horizontal";

      case WL_OUTPUT_TRANSFORM_90:
      case WL_OUTPUT_TRANSFORM_270:
      case WL_OUTPUT_TRANSFORM_FLIPPED_90:
      case WL_OUTPUT_TRANSFORM_FLIPPED_270:
        return "vertical";

      default:
        return "none";
    }

    return "none";
}

Json::Value ipc_json::build_status(RETURN_STATUS status, Json::Value value,
    const char *error)
{
    Json::Value object;
    object["success"] = !RETURN_ERROR(status);
    object["status"]  = status;
    if (object["success"] == false)
    {
        object["parse_error"] = !object["success"];
        // TODO: RETURN_ERROR TO STRING
        object["error"] = "";
    }

    if (value.isNull() == false)
    {
        object["value"] = value;
    }

    if (error != nullptr)
    {
        object["error"] = std::string(error);
    }

    return object;
}

Json::Value ipc_json::describe_libinput_device(struct libinput_device *device)
{
    Json::Value object;

    if (device == nullptr)
    {
        return object;
    }

    const char *events = "unknown";
    switch (libinput_device_config_send_events_get_mode(device))
    {
      case LIBINPUT_CONFIG_SEND_EVENTS_ENABLED:
        events = "enabled";
        break;

      case LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE:
        events = "disabled_on_external_mouse";
        break;

      case LIBINPUT_CONFIG_SEND_EVENTS_DISABLED:
        events = "disabled";
        break;
    }

    object["send_events"] = events;

    if (libinput_device_config_tap_get_finger_count(device) > 0)
    {
        const char *tap = "unknown";
        switch (libinput_device_config_tap_get_enabled(device))
        {
          case LIBINPUT_CONFIG_TAP_ENABLED:
            tap = "enabled";
            break;

          case LIBINPUT_CONFIG_TAP_DISABLED:
            tap = "disabled";
            break;
        }

        object["tap"] = tap;

        const char *button_map = "unknown";
        switch (libinput_device_config_tap_get_button_map(device))
        {
          case LIBINPUT_CONFIG_TAP_MAP_LRM:
            button_map = "lrm";
            break;

          case LIBINPUT_CONFIG_TAP_MAP_LMR:
            button_map = "lmr";
            break;
        }

        object["tap_button_map"] = button_map;

        const char *drag = "unknown";
        switch (libinput_device_config_tap_get_drag_enabled(device))
        {
          case LIBINPUT_CONFIG_DRAG_ENABLED:
            drag = "enabled";
            break;

          case LIBINPUT_CONFIG_DRAG_DISABLED:
            drag = "disabled";
            break;
        }

        object["tap_drag"] = drag;

        const char *drag_lock = "unknown";
        switch (libinput_device_config_tap_get_drag_lock_enabled(device))
        {
          case LIBINPUT_CONFIG_DRAG_LOCK_ENABLED:
            drag_lock = "enabled";
            break;

          case LIBINPUT_CONFIG_DRAG_LOCK_DISABLED:
            drag_lock = "disabled";
            break;
        }

        object["tap_drag_lock"] = drag_lock;
    }

    if (libinput_device_config_accel_is_available(device))
    {
        double accel = libinput_device_config_accel_get_speed(device);
        object["accel_speed"] = accel;

        const char *accel_profile = "unknown";
        switch (libinput_device_config_accel_get_profile(device))
        {
          case LIBINPUT_CONFIG_ACCEL_PROFILE_NONE:
            accel_profile = "none";
            break;

          case LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT:
            accel_profile = "flat";
            break;

          case LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE:
            accel_profile = "adaptive";
            break;
        }

        object["accel_profile"] = accel_profile;
    }

    if (libinput_device_config_scroll_has_natural_scroll(device))
    {
        const char *natural_scroll = "disabled";
        if (libinput_device_config_scroll_get_natural_scroll_enabled(device))
        {
            natural_scroll = "enabled";
        }

        object["natural_scroll"] = natural_scroll;
    }

    if (libinput_device_config_left_handed_is_available(device))
    {
        const char *left_handed = "disabled";
        if (libinput_device_config_left_handed_get(device) != 0)
        {
            left_handed = "enabled";
        }

        object["left_handed"] = left_handed;
    }

    uint32_t click_methods = libinput_device_config_click_get_methods(device);
    if ((click_methods & ~LIBINPUT_CONFIG_CLICK_METHOD_NONE) != 0)
    {
        const char *click_method = "unknown";
        switch (libinput_device_config_click_get_method(device))
        {
          case LIBINPUT_CONFIG_CLICK_METHOD_NONE:
            click_method = "none";
            break;

          case LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS:
            click_method = "button_areas";
            break;

          case LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER:
            click_method = "clickfinger";
            break;
        }

        object["click_method"] = click_method;
    }

    if (libinput_device_config_middle_emulation_is_available(device))
    {
        const char *middle_emulation = "unknown";
        switch (libinput_device_config_middle_emulation_get_enabled(device))
        {
          case LIBINPUT_CONFIG_MIDDLE_EMULATION_ENABLED:
            middle_emulation = "enabled";
            break;

          case LIBINPUT_CONFIG_MIDDLE_EMULATION_DISABLED:
            middle_emulation = "disabled";
            break;
        }

        object["middle_emulation"] = middle_emulation;
    }

    uint32_t scroll_methods = libinput_device_config_scroll_get_methods(device);
    if ((scroll_methods & ~LIBINPUT_CONFIG_SCROLL_NO_SCROLL) != 0)
    {
        const char *scroll_method = "unknown";
        switch (libinput_device_config_scroll_get_method(device))
        {
          case LIBINPUT_CONFIG_SCROLL_NO_SCROLL:
            scroll_method = "none";
            break;

          case LIBINPUT_CONFIG_SCROLL_2FG:
            scroll_method = "two_finger";
            break;

          case LIBINPUT_CONFIG_SCROLL_EDGE:
            scroll_method = "edge";
            break;

          case LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN:
            scroll_method = "on_button_down";
            break;
        }

        object["scroll_method"] = scroll_method;

        if ((scroll_methods & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) != 0)
        {
            uint32_t button = libinput_device_config_scroll_get_button(device);
            object["scroll_button"] = button;
        }
    }

    if (libinput_device_config_dwt_is_available(device))
    {
        const char *dwt = "unknown";
        switch (libinput_device_config_dwt_get_enabled(device))
        {
          case LIBINPUT_CONFIG_DWT_ENABLED:
            dwt = "enabled";
            break;

          case LIBINPUT_CONFIG_DWT_DISABLED:
            dwt = "disabled";
            break;
        }

        object["dwt"] = dwt;
    }

    if (libinput_device_config_calibration_has_matrix(device))
    {
        float matrix[6];
        libinput_device_config_calibration_get_matrix(device, matrix);
        Json::Value array = Json::arrayValue;
        for (int i = 0; i < 6; i++)
        {
            array.append(matrix[i]);
        }

        object["calibration_matrix"] = array;
    }

    return object;
}

Json::Value ipc_json::describe_input(nonstd::observer_ptr<wf::input_device_t> device)
{
    Json::Value object;
    if (!device)
    {
        LOGE("Device must not be null");
        return object;
    }

    wlr_input_device *wlr_handle = device->get_wlr_handle();

    if (wlr_handle == nullptr)
    {
        return object;
    }

    if (wlr_handle->name != nullptr)
    {
        std::string name(wlr_handle->name);
        std::transform(name.begin(), name.end(), name.begin(),
            [] (char c) {return c == ' ' ? '_' : c;});
        object["identifier"] = std::to_string(wlr_handle->vendor) + ":" +
            std::to_string(wlr_handle->product) + ":" + name;
    }

    object["name"]    = wlr_handle->name;
    object["vendor"]  = wlr_handle->vendor;
    object["product"] = wlr_handle->product;
    object["type"]    = input_device_get_type_description(device);
    object["name"]    = wlr_handle->name;

    if (wlr_handle->type == WLR_INPUT_DEVICE_KEYBOARD)
    {
        struct wlr_keyboard *keyboard = wlr_handle->keyboard;

        object["repeat_delay"] = keyboard_repeat_delay.value();
        object["repeat_rate"]  = keyboard_repeat_rate.value();

        Json::Value layouts_arr = Json::arrayValue;
        object["xkb_layout_names"] = layouts_arr;

        if (keyboard != nullptr)
        {
            struct xkb_keymap *keymap = keyboard->keymap;
            struct xkb_state *state   = keyboard->xkb_state;

            if (keymap != nullptr)
            {
                xkb_layout_index_t num_layouts = xkb_keymap_num_layouts(keymap);
                xkb_layout_index_t layout_idx;
                for (layout_idx = 0; layout_idx < num_layouts; layout_idx++)
                {
                    const char *layout = xkb_keymap_layout_get_name(keymap,
                        layout_idx);

                    layouts_arr.append(layout ? layout : "");

                    if (state != nullptr)
                    {
                        bool is_active = xkb_state_layout_index_is_active(
                            state, layout_idx, XKB_STATE_LAYOUT_EFFECTIVE);
                        if (is_active)
                        {
                            object["xkb_active_layout_index"] = layout_idx;
                            object["xkb_active_layout_name"]  = layout ? layout : "";
                        }
                    }
                }
            }
        }
    }

    if (wlr_handle->type == WLR_INPUT_DEVICE_POINTER)
    {
        float sf = 1.0f;
        double scroll_factor = mouse_scroll_speed.value();
        if (!isnan(scroll_factor) &&
            (scroll_factor != std::numeric_limits<float>::min()))
        {
            sf = scroll_factor;
        }

        object["scroll_factor"] = (double)sf;
    }

#if 0
    if (wlr_input_device_is_libinput(wlr_handle))
    {
        libinput_device *libinput_dev = wlr_libinput_get_device_handle(wlr_handle);
        if (libinput_dev != nullptr)
        {
            object["libinput"] = describe_libinput_device(libinput_dev);
        }
    }

#endif

    return object;
}

Json::Value ipc_json::describe_seat(wlr_seat *seat)
{
    Json::Value object;
    if (!seat)
    {
        LOGE("Seat must not be null");
        return object;
    }

    object["name"] = seat->name;
    object["capabilities"] = seat->capabilities;
    object["focus"] = 1;

    Json::Value devices = Json::arrayValue;
    auto inputs = wf::get_core().get_input_devices();
    for (auto& device : inputs)
    {
        devices.append(describe_input(device));
    }

    object["devices"] = devices;

    return object;
}

Json::Value ipc_json::describe_disabled_output(wf::output_t *output)
{
    Json::Value object;
    wlr_output *wlr_output = output->handle;

    object["non_desktop"] = false;
    object["type"]    = "output";
    object["name"]    = wlr_output->name;
    object["active"]  = false;
    object["dpms"]    = false;
    object["power"]   = false;
    object["primary"] = false;
    object["layout"]  = "output";
    object["orientation"] = "none";
    object["make"]   = wlr_output->make;
    object["model"]  = wlr_output->model;
    object["serial"] = wlr_output->serial;
    object["nodes"]  = Json::arrayValue;

    Json::Value modes_array = Json::arrayValue;

    struct wlr_output_mode *mode = nullptr;
    wl_list_for_each(mode, &wlr_output->modes, link)
    {
        if (mode != nullptr)
        {
            Json::Value mode_object;
            mode_object["width"]   = mode->width;
            mode_object["height"]  = mode->height;
            mode_object["refresh"] = mode->refresh;
            modes_array.append(mode_object);
        }
    }

    object["modes"] = modes_array;

    Json::Value current_mode_object;
    current_mode_object["width"]   = wlr_output->width;
    current_mode_object["height"]  = wlr_output->height;
    current_mode_object["refresh"] = wlr_output->refresh;

    object["rect"] = create_empty_rect();

    return object;
}

Json::Value ipc_json::describe_point(wf::point_t point)
{
    Json::Value object;
    object["x"] = point.x;
    object["y"] = point.y;
    return object;
}

Json::Value ipc_json::describe_pointf(wf::pointf_t point)
{
    Json::Value object;
    object["x"] = point.x;
    object["y"] = point.y;
    return object;
}

Json::Value ipc_json::describe_output(wf::output_t *output)
{
    Json::Value object;
    wlr_output *wlr_output = output->handle;

    //
    // Node attr
    //
    object["id"]     = output->get_id();
    object["type"]   = "output";
    object["border"] = "none";
    object["current_border_width"] = 0;
    object["orientation"] = orientation_description(output);
    object["percent"]     = Json::nullValue;
    object["urgent"]  = false;
    object["sticky"]  = false;
    object["focused"] = wf::get_core().get_active_output() == output;
    object["floating_nodes"] = get_shell_view_nodes(output);
    object["focus"] = Json::arrayValue;
    object["rect"]  = describe_wlr_box(output->get_layout_geometry());
    object["nodes"] = Json::arrayValue;

    //
    // Output specific attr
    //
    object["non_desktop"] = false;
    object["active"]  = true;
    object["dpms"]    = wlr_output->enabled;
    object["power"]   = wlr_output->enabled;
    object["primary"] = false;
    object["layout"]  = "output";
    object["orientation"] = orientation_description(wlr_output->transform);
    object["make"]   = wlr_output->make;
    object["name"]   = wlr_output->name;
    object["model"]  = wlr_output->model;
    object["serial"] = wlr_output->serial;
    object["scale"]  = wlr_output->scale;
    object["scale_filter"] = "unknown";
    object["transform"]    = output_transform_description(wlr_output->transform);
    object["adaptive_sync_status"] =
        output_adaptive_sync_status_description(wlr_output->adaptive_sync_status);

    wf::point_t workspace = output->workspace->get_current_workspace();
    object["current_workspace"] = ipc_tools::get_workspace_index(workspace, output);

    Json::Value modes_array = Json::arrayValue;

    struct wlr_output_mode *mode = nullptr;
    wl_list_for_each(mode, &wlr_output->modes, link)
    {
        if (mode != nullptr)
        {
            Json::Value mode_object;
            mode_object["width"]   = mode->width;
            mode_object["height"]  = mode->height;
            mode_object["refresh"] = mode->refresh;
            modes_array.append(mode_object);
        }
    }

    object["modes"] = modes_array;

    const char *subpixel =
        ipc_json::output_subpixel_description(output->handle->subpixel);
    object["subpixel_hinting"] = subpixel;

    Json::Value current_mode_object;
    current_mode_object["width"]   = wlr_output->width;
    current_mode_object["height"]  = wlr_output->height;
    current_mode_object["refresh"] = wlr_output->refresh;

    object["current_mode"]    = current_mode_object;
    object["max_render_time"] = max_render_time.value();
    object["dynamic_repaint_delay"] = dynamic_delay.value();

    return object;
}

Json::Value ipc_json::describe_geometry(wf::geometry_t geometry)
{
    Json::Value object;
    object["x"]     = geometry.x;
    object["y"]     = geometry.y;
    object["width"] = geometry.width;
    object["height"] = geometry.height;
    return object;
}

Json::Value ipc_json::describe_wlr_box(wlr_box box)
{
    Json::Value object;
    object["x"]     = box.x;
    object["y"]     = box.y;
    object["width"] = box.width;
    object["height"] = box.height;
    return object;
}

Json::Value ipc_json::describe_dimension(wf::dimensions_t dimension)
{
    Json::Value object;
    object["width"]  = dimension.width;
    object["height"] = dimension.height;
    return object;
}

Json::Value ipc_json::describe_wlr_surface_state(wlr_surface_state surface_state)
{
    Json::Value object;
    object["committed"]     = surface_state.committed;
    object["buffer_height"] = surface_state.buffer_height;
    object["buffer_width"]  = surface_state.buffer_width;
    object["cached_state_locks"] = surface_state.cached_state_locks;
    object["width"]  = surface_state.width;
    object["height"] = surface_state.height;
    object["dx"]     = surface_state.dx;
    object["dy"]     = surface_state.dy;
    object["scale"]  = surface_state.scale;
    object["seq"]    = surface_state.seq;
    return object;
}

Json::Value ipc_json::describe_wlr_surface(wlr_surface *surface)
{
    Json::Value object;
    if (surface != nullptr)
    {
        object["sx"] = surface->sx;
        object["sy"] = surface->sy;
        object["current"] = describe_wlr_surface_state(surface->current);
    }

    return object;
}

#if HAVE_XWAYLAND
Json::Value ipc_json::describe_wlr_xwayland_surface(wlr_xwayland_surface *surface)
{
    Json::Value object;
    if (surface != nullptr)
    {
        object["window_id"] = surface->window_id;
        // object["instance"] = surface->instance;
        object["mapped"] = surface->mapped;
        object["maximized_horz"] = surface->maximized_horz;
        object["maximized_vert"] = surface->maximized_vert;
        object["minimized"] = surface->minimized;
        object["modal"]     = surface->modal;
        object["override_redirect"] = surface->override_redirect;
        object["pid"]     = surface->pid;
        object["pinging"] = surface->pinging;
        // object["role"] = std::string(surface->role);
        // object["title"] = std::string(surface->title);
        object["X"] = surface->x;
        object["surface_id"]  = surface->surface_id;
        object["decorations"] = surface->decorations;
        object["fullscreen"]  = surface->fullscreen;
        object["has_alpha"]   = surface->has_alpha;
        object["has_utf8_title"] = surface->has_utf8_title;
        object["height"] = surface->height;
        object["width"]  = surface->width;
        object["hints_urgency"] = surface->hints_urgency;
        object["saved_height"]  = surface->saved_height;
        object["saved_width"]   = surface->saved_width;
    }

    return object;
}

#endif

Json::Value ipc_json::describe_wl_display(wl_display *display)
{
    Json::Value object;
    if (display != nullptr)
    {
        object["serial"] = wl_display_get_serial(display);
    }

    return object;
}

Json::Value ipc_json::describe_wl_client(wl_client *client)
{
    Json::Value object;
    if (client != nullptr)
    {
        /* Get pid for native view */
        pid_t wl_pid = 0;
        wl_client_get_credentials(client, &wl_pid, 0, 0);
        object["pid"] = wl_pid;

        object["fd"] = wl_client_get_fd(client);
        wl_display *display = wl_client_get_display(client);
        if (display != nullptr)
        {
            object["display"] = describe_wl_display(display);
        }
    }

    return object;
}

Json::Value ipc_json::describe_view(wayfire_view view)
{
    if (view == nullptr)
    {
        return Json::nullValue;
    }

    Json::Value object;

    //
    // Node attr
    //
    object["id"] = view->get_id();
    // NOTE for now we support only support the wm-actions plugin
    object["type"] = view->has_data("wm-actions-above") ||
        (view->get_output()->workspace->get_view_layer(view) &
            (wf::LAYER_TOP | wf::LAYER_DESKTOP_WIDGET)) ? "floating_con" : "con";
    // TODO support tiling
    object["layout"] = "stacked";
    // TODO: figure out what if the border type for the container
    object["border"] = "none";
    object["current_border_width"] = 0;
    object["orientation"] = orientation_description(view->get_output());
    object["urgent"] = false;
    object["sticky"] = view->sticky;
    object["floating_nodes"] = get_view_nodes(view, true);
    object["focus"] = Json::arrayValue;
    object["name"]  = view->get_title();
    object["nodes"] = Json::arrayValue;

    wf::output_t *output = view->get_output();
    if (output != nullptr)
    {
        object["focused"] = (view->get_output()->get_top_view() == view) &&
            view->is_visible();
    }

    wayfire_view parent = view->parent;
    if (parent != nullptr)
    {
        wlr_box parent_box = parent->get_bounding_box();

        if ((parent_box.width != 0) && (parent_box.height != 0))
        {
            double percent =
                ((double)view->get_bounding_box().width / parent_box.width) *
                ((double)view->get_bounding_box().height / parent_box.height);
            object["percent"] = percent;
        }
    }

    //
    // Container specific attr
    //
    wl_client *client = view->get_client();
    if (client != nullptr)
    {
        /* Get pid for native view */
        pid_t wl_pid = 0;
        wl_client_get_credentials(client, &wl_pid, 0, 0);
        object["pid"] = wl_pid;
    }

    auto wlr_surface = view->get_wlr_surface();
    if (wlr_surface != nullptr)
    {
        wf::geometry_t rect;
        rect.x     = wlr_surface->pending.dx;
        rect.y     = wlr_surface->pending.dy;
        rect.width = wlr_surface->pending.width;
        rect.height    = wlr_surface->pending.height;
        object["rect"] = describe_geometry(rect);
    }

    object["app_id"]  = view->get_app_id();
    object["visible"] = view->is_visible();
    object["window_rect"] = describe_wlr_box(view->get_bounding_box());
    object["geometry"]    = describe_geometry(view->get_wm_geometry());

    object["fullscreen_mode"] = view->fullscreen;

    object["shell"] = "xdg_shell";
    object["inhibit_idle"] = false;

    Json::Value idle_inhibitors;
    idle_inhibitors["user"] = "none";
    idle_inhibitors["application"] = "none";
    object["idle_inhibitors"] = idle_inhibitors;

#if HAVE_XWAYLAND
    if (xwayland_enabled == 1)
    {
        auto main_surface = view->get_main_surface();
        if (main_surface != nullptr)
        {
            auto main_wlr_surface = main_surface->get_wlr_surface();
            if (main_wlr_surface != nullptr)
            {
                if (wlr_surface_is_xwayland_surface(main_wlr_surface))
                {
                    object["is_xwayland"] = true;
                    object["shell"] = "xwayland";

                    struct wlr_xwayland_surface *main_xsurf =
                        wlr_xwayland_surface_from_wlr_surface(main_wlr_surface);
                    if (main_xsurf != nullptr)
                    {
                        Json::Value window_props;

                        object["window"] = main_xsurf->window_id;
                        object["urgent"] = !!main_xsurf->hints_urgency;

                        auto clazz = main_xsurf->class_t;
                        if (clazz != nullptr)
                        {
                            object["class"] = std::string(clazz);
                        }

                        auto instance = main_xsurf->instance;
                        if (instance != nullptr)
                        {
                            window_props["instance"] = std::string(instance);
                        }

                        auto title = main_xsurf->title;
                        if (title != nullptr)
                        {
                            window_props["title"] = std::string(title);
                        }

                        auto role = main_xsurf->role;
                        if (role != nullptr)
                        {
                            window_props["window_role"] = std::string(role);
                        }

                        if (parent != nullptr)
                        {
                            window_props["transient_for"] = parent->get_id();
                        }

                        window_props["type"] = "unknown";

                        object["window_properties"] = window_props;
                    }
                }
            }
        }
    }

#endif

    return object;
}

Json::Value ipc_json::get_root_node()
{
    Json::Value object;
    auto active_output = wf::get_core().get_active_output();

    if (active_output == nullptr)
    {
        return Json::nullValue;
    }

    //
    // Node attr
    //
    object["id"]     = 1;
    object["layout"] = "root";
    object["name"]   = "root";
    object["type"]   = "root";
    object["border"] = "none";
    object["current_border_width"] = 0;
    object["orientation"] = orientation_description(active_output);
    object["percent"]     = Json::nullValue;
    object["urgent"]  = false;
    object["sticky"]  = false;
    object["focused"] = true;
    object["floating_nodes"] = Json::arrayValue;
    object["focus"] = Json::arrayValue;

    auto wsize = active_output->workspace->get_workspace_grid_size();
    if ((wsize.width > 0) && (wsize.height > 0))
    {
        auto workspace = describe_workspace(wf::point_t{0, 0}, active_output);
        if (!workspace.isNull())
        {
            object["rect"] = workspace["rect"];
        }
    }

    return object;
}

Json::Value ipc_json::get_shell_view_nodes(wf::output_t *output)
{
    Json::Value nodes = Json::arrayValue;
    for (auto& view :
         wf::get_core().get_all_views())
    {
        if ((view->role != wf::VIEW_ROLE_DESKTOP_ENVIRONMENT) || !view->is_mapped())
        {
            continue;
        }

        if (view->get_output() != output)
        {
            continue;
        }

        auto container = describe_view(view);
        if (!container.isNull())
        {
            container["nodes"] = get_view_nodes(view);
        }

        nodes.append(container);
    }

    return nodes;
}

Json::Value ipc_json::get_top_view_nodes(wf::point_t point, wf::output_t *output)
{
    Json::Value nodes = Json::arrayValue;
    for (auto& view :
         output->workspace->get_views_on_workspace(point,
             wf::LAYER_TOP | wf::LAYER_DESKTOP_WIDGET))
    {
        if ((view->role != wf::VIEW_ROLE_TOPLEVEL) || !view->is_mapped())
        {
            continue;
        }

        auto container = describe_view(view);
        if (!container.isNull())
        {
            container["nodes"] = get_view_nodes(view);
        }

        nodes.append(container);
    }

    return nodes;
}

Json::Value ipc_json::get_view_nodes(wayfire_view view, bool floating)
{
    Json::Value nodes = Json::arrayValue;

    for (auto& v :
         view->enumerate_views())
    {
        if (v == view)
        {
            continue;
        }

        if ((v->role != wf::VIEW_ROLE_TOPLEVEL) || !v->is_mapped())
        {
            continue;
        }

        if ((floating == true) && !v->has_data("wm-actions-above"))
        {
            continue;
        }

        if ((floating == false) && v->has_data("wm-actions-above"))
        {
            continue;
        }

        auto obj = describe_view(v);
        nodes.append(obj);
    }

    return nodes;
}

Json::Value ipc_json::get_container_nodes(wf::point_t point, wf::output_t *output)
{
    Json::Value nodes = Json::arrayValue;
    for (auto& view :
         output->workspace->get_views_on_workspace(point,
             wf::LAYER_WORKSPACE | wf::LAYER_TOP | wf::LAYER_DESKTOP_WIDGET))
    {
        if ((view->role != wf::VIEW_ROLE_TOPLEVEL) || !view->is_mapped())
        {
            continue;
        }

        auto container = describe_view(view);
        if (!container.isNull())
        {
            container["nodes"] = get_view_nodes(view);
        }

        nodes.append(container);
    }

    return nodes;
}

Json::Value ipc_json::get_workspaces_nodes(wf::output_t *output)
{
    Json::Value nodes = Json::arrayValue;

    auto wsize = output->workspace->get_workspace_grid_size();
    for (int x = 0; x < wsize.width; x++)
    {
        for (int y = 0; y < wsize.height; y++)
        {
            auto workspace = describe_workspace(wf::point_t{x, y}, output);
            workspace["nodes"] = get_container_nodes(wf::point_t{x, y}, output);
            nodes.append(workspace);
        }
    }

    return nodes;
}

Json::Value ipc_json::get_tree()
{
    Json::Value rootNodes = Json::arrayValue;
    Json::Value root = get_root_node();

    if (root.isNull())
    {
        return Json::nullValue;
    }

    auto outputs = wf::get_core().output_layout->get_outputs();
    for (wf::output_t *output : outputs)
    {
        auto out = describe_output(output);
        out["nodes"] = get_workspaces_nodes(output);
        rootNodes.append(out);
    }

    root["nodes"] = rootNodes;
    return root;
}

Json::Value ipc_json::describe_workspace(wf::point_t point, wf::output_t *output)
{
    assert(output != nullptr);
    ASSERT_VALID_WORKSPACE(output->workspace, point);

    auto wall = std::make_unique<wf::workspace_wall_t>(output);
    assert(wall != nullptr);

    auto rect    = wall->get_workspace_rectangle(point);
    bool visible = output->workspace->get_current_workspace() == point;
    bool focused = visible && (wf::get_core().get_active_output() == output);
    int index    = ipc_tools::get_workspace_index(point, output);
    int id = ipc_tools::get_workspace_id(output->get_id(), index);

    Json::Value object = Json::objectValue;

    //
    // Node attr
    //
    object["id"]     = id;
    object["type"]   = "workspace";
    object["layout"] = "stacked";
    object["border"] = "none";
    object["current_border_width"] = 0;
    object["orientation"] = orientation_description(output);
    object["percent"]     = Json::nullValue;
    object["urgent"]  = false;
    object["sticky"]  = false;
    object["focused"] = focused;
    object["representation"] = "";
    object["floating_nodes"] = get_top_view_nodes(point, output);
    object["focus"] = Json::arrayValue;
    object["nodes"] = Json::arrayValue;

    //
    // Workspace specific attr
    //
    object["num"] = index;
    object["fullscreen_mode"] = 1;
    object["name"]    = index;
    object["visible"] = visible;
    object["rect"]    = ipc_json::describe_geometry(rect);
    object["output"]  = output->handle->name;
    object["grid_position"] = ipc_json::wayfire_point_to_json(point);

    return object;
}
