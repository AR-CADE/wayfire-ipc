#include <ipc/server.hpp>
#include "ipc.h"
#include "wayfire/singleton-plugin.hpp"
#include <sys/time.h>

wf::option_wrapper_t<std::string> xkb_model{"input/xkb_model"};
wf::option_wrapper_t<std::string> xkb_layout{"input/xkb_layout"};
wf::option_wrapper_t<std::string> xkb_options{"input/xkb_options"};
wf::option_wrapper_t<std::string> xkb_rules{"input/xkb_rules"};

// Damn, another variant ... hoppefully we have our event_mask, in the worst case, i
// can implement a completly useless workaroud (but mandatory ;-) ) to fix it !!!!
wf::option_wrapper_t<std::string> xkb_variant{"input/xkb_variant"};
static struct wl_listener display_destroy;
static struct wl_event_source *fini_event_source;

class ipc_t : public wf::singleton_plugin_t<ipc_server_t>
{
  public:
    void init() override
    {
        singleton_plugin_t::init();

        get_instance().serve();

        bind_events();
    }

    static void handle_display_destroy(struct wl_listener *listener,
        void *data)
    {
        if (fini_event_source != nullptr)
        {
            wl_event_source_remove(fini_event_source);
        }

        wl_list_remove(&display_destroy.link);

        ipc_server_t::handle_display_destroy(listener, data);
    }

    static int handle_fini_timeout(void *data)
    {
        ipc_t *instance = static_cast<ipc_t*>(data);
        instance->fini_timeout();
        return 0;
    }

    void fini() override
    {
        /* Notify exit */
        signal_shutdown_event();

        auto views = wf::get_core().get_all_views();

        for (wayfire_view view : views)
        {
            unbind_view_events(view);
        }

        auto outputs = wf::get_core().output_layout->get_outputs();

        for (wf::output_t *output : outputs)
        {
            unbind_output_events(output);
        }

        unbind_core_events();

        // Set a timeout of 100 ms to give some time to all clients to handle the
        // "exit" signal before disconnecting them and finish this instance of plugin
        display_destroy.notify = handle_display_destroy;
        wl_display_add_destroy_listener(wf::get_core().display, &display_destroy);
        fini_event_source =
            wl_event_loop_add_timer(wf::get_core().ev_loop, handle_fini_timeout,
                this);
        wl_event_source_timer_update(fini_event_source, 100);
    }

    void fini_timeout()
    {
        ipc_t::handle_display_destroy(nullptr, nullptr);
        singleton_plugin_t::fini();
    }

  private:

    void signal_event(const enum ipc_event_type & signal,
        const std::string & json_string)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        ipc_server_t::send_event(json_string.c_str(), signal);
    }

    Json::Value window_json_data(wf::signal_data_t *data, const std::string & change)
    {
        if (data == nullptr)
        {
            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        auto view = get_signaled_view(data);
        if (view == nullptr)
        {
            LOGE("View is null.");

            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;

        auto container = ipc_json::describe_view(view);
        if (container.isNull())
        {
            LOGE("Unable to parse container.");

            return Json::nullValue;
        }

        if (change == "urgent")
        {
            wf::view_hints_changed_signal *d = (wf::view_hints_changed_signal*)data;
            container["urgent"] = d->demands_attention;
        }

        object["container"] = container;

        return object;
    }

    Json::Value shutdown_json_data()
    {
        Json::Value object;
        object["change"] = "exit";
        return object;
    }

    void signal_shutdown_event()
    {
        if (!ipc_server_t::has_event_listeners(IPC_I3_EVENT_TYPE_SHUTDOWN))
        {
            return;
        }

        Json::Value json = shutdown_json_data();
        if (json.isNull() == false)
        {
            signal_event(IPC_I3_EVENT_TYPE_SHUTDOWN, ipc_json::json_to_string(json));
        }
    }

    void signal_window_event(const enum ipc_event_type & signal,
        wf::signal_data_t *data, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        if (data == nullptr)
        {
            return;
        }

        Json::Value json = window_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal_connection_t view_mapped = [=] (wf::signal_data_t *data)
    {
        bind_view_events(wf::get_signaled_view(data));
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "new");
    };

    wf::signal_connection_t view_unmapped = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "close");
    };

    wf::signal_connection_t view_minimized = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
    };

    wf::signal_connection_t view_fullscreened = [=] (wf::signal_data_t *data)
    {
        auto signal = static_cast<wf::view_fullscreen_signal*>(data);
        if (signal == nullptr)
        {
            return;
        }

        auto view = signal->view;
        if (view == nullptr)
        {
            return;
        }

        if (view->fullscreen != signal->state)
        {
            signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "fullscreen_mode");
        }
    };

    wf::signal_connection_t view_focused = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "focus");
    };

    wf::signal_connection_t view_change_workspace = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
    };


    wf::signal_connection_t wm_actions_above_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "floating");
    };

    wf::signal_connection_t view_title_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "title");
    };

    wf::signal_connection_t view_geometry_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
    };

    // wf::signal_connection_t view_above_changed = [=] (wf::signal_data_t *data)
    // {
    // signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "focus");
    // };

    wf::signal_connection_t view_hints_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "urgent");
    };


    Json::Value output_json_data(wf::signal_data_t *data, const std::string & change)
    {
        (void)data;
        // if (data == nullptr)
        // {
        // return Json::nullValue;
        // }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        // auto output = get_signaled_output(data);

        // if (output == nullptr)
        // {
        // LOGE("Output is null.");

        // return Json::nullValue;
        // }

        Json::Value object;
        object["change"] = change;
        // object["output"] = ipc_json::describe_output(output);

        return object;
    }

    void signal_output_event(const enum ipc_event_type & signal,
        wf::signal_data_t *data, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        // if (data == nullptr)
        // {
        // return;
        // }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        Json::Value json = output_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal_connection_t output_added = [=] (wf::signal_data_t *data)
    {
        bind_output_events(get_signaled_output(data));
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, data, /*"new"*/ "unspecified");
    };

    wf::signal_connection_t output_removed = [=] (wf::signal_data_t *data)
    {
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, data,
            /*"close"*/ "unspecified");
    };

    wf::signal_connection_t output_layout_config_changed =
        [=] (wf::signal_data_t *data)
    {
        (void)data;
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, nullptr,
            /*"update"*/ "unspecified");
    };

    Json::Value input_json_data(wf::signal_data_t *data, const std::string & change)
    {
        if (data == nullptr)
        {
            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        auto ev = static_cast<wf::input_device_signal*>(data);

        auto device = ev->device;

        if (device == nullptr)
        {
            LOGE("Device is null.");

            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;
        object["input"]  = ipc_json::describe_input(device);

        return object;
    }

    void signal_input_event(const enum ipc_event_type & signal,
        wf::signal_data_t *data, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (data == nullptr)
        {
            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        Json::Value json = input_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal_connection_t input_device_added = [=] (wf::signal_data_t *data)
    {
        signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, data, "added");
    };


    wf::signal_connection_t input_device_removed = [=] (wf::signal_data_t *data)
    {
        signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, data, "removed");
    };

    void xkb_change()
    {
        auto devices = wf::get_core().get_input_devices();
        for (auto & dev : devices)
        {
            auto wlr_handle = dev->get_wlr_handle();

            if (wlr_handle != nullptr)
            {
                if ((wlr_handle->product == 1) && (wlr_handle->vendor == 1))
                {
                    wf::input_device_signal data;
                    data.device = dev;
                    signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, &data,
                        "xkb_layout");
                    signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, &data,
                        "xkb_keymap");
                }
            }
        }
    }

    Json::Value workspace_json_data(wf::signal_data_t *data,
        const std::string & change)
    {
        if (data == nullptr)
        {
            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;

        wf::workspace_changed_signal signal;

        if (change == "focus")
        {
            signal = *(static_cast<wf::workspace_changed_signal*>(data));
        }

        if (change == "reload")
        {
            signal.output = wf::get_core().get_active_output();
            signal.new_viewport = signal.output->workspace->get_current_workspace();
        }

        auto outputs = wf::get_core().output_layout->get_outputs();

        for (wf::output_t *output : outputs)
        {
            if (signal.output == output)
            {
                auto wsize = output->workspace->get_workspace_grid_size();
                for (int x = 0; x < wsize.width; x++)
                {
                    for (int y = 0; y < wsize.height; y++)
                    {
                        wf::point_t point{x, y};
                        if (point == signal.new_viewport)
                        {
                            Json::Value workspace = ipc_json::describe_workspace(
                                wf::point_t{x, y}, output);
                            object["current"] = workspace;
                        }

                        if (point == signal.old_viewport)
                        {
                            Json::Value workspace = ipc_json::describe_workspace(
                                wf::point_t{x, y}, output);
                            object["old"] = workspace;
                        }
                    }
                }
            }
        }

        return object;
    }

    void signal_workspace_event(const enum ipc_event_type & signal,
        wf::signal_data_t *data, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (data == nullptr)
        {
            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        Json::Value json = workspace_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal_connection_t workspace_changed = [=] (wf::signal_data_t *data)
    {
        signal_workspace_event(IPC_I3_EVENT_TYPE_WORKSPACE, data, "focus");
    };

    wf::signal_connection_t workspace_grid_changed = [=] (wf::signal_data_t *data)
    {
        signal_workspace_event(IPC_I3_EVENT_TYPE_WORKSPACE, data, "reload");
    };

    void bind_events()
    {
        auto outputs = wf::get_core().output_layout->get_outputs();

        for (wf::output_t *output : outputs)
        {
            bind_output_events(output);
        }

        auto views = wf::get_core().get_all_views();

        for (wayfire_view view : views)
        {
            bind_view_events(view);
        }

        //
        // Connect core signals
        //
        bind_core_events();

        // When the configuration options change, mark them as dirty.
        // They are applied at the config-reloaded signal.
        xkb_model.set_callback([=] () { xkb_change(); });
        xkb_variant.set_callback([=] () { xkb_change(); });
        xkb_layout.set_callback([=] () { xkb_change(); });
        xkb_options.set_callback([=] () { xkb_change(); });
        xkb_rules.set_callback([=] () { xkb_change(); });
    }

    void bind_output_events(wf::output_t *output)
    {
        if (output == nullptr)
        {
            return;
        }

        output->connect_signal(IPC_WF_EVENT_VIEW_MAPPED, &view_mapped);
        output->connect_signal(IPC_WF_EVENT_VIEW_UNMAPPED, &view_unmapped);
        output->connect_signal(IPC_WF_EVENT_VIEW_FOCUSED, &view_focused);
        output->connect_signal(IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED,
            &view_geometry_changed);

        output->connect_signal(IPC_WF_EVENT_VIEW_MINIMIZED, &view_minimized);
        output->connect_signal(IPC_WF_EVENT_VIEW_FULLSCREENED, &view_fullscreened);
        output->connect_signal(IPC_WF_EVENT_VIEW_ABOVE_CHANGED,
            &wm_actions_above_changed);
        output->connect_signal(IPC_WF_EVENT_VIEW_CHANGE_WORKSPACE,
            &view_change_workspace);
        output->connect_signal(IPC_WF_EVENT_WORKSPACE_GRID_CHANGED,
            &workspace_grid_changed);
        output->connect_signal(IPC_WF_EVENT_WORKSPACE_CHANGED, &workspace_changed);
    }

    void unbind_output_events(wf::output_t *output)
    {
        if (output == nullptr)
        {
            return;
        }

        output->disconnect_signal(&view_mapped);
        output->disconnect_signal(&view_unmapped);
        output->disconnect_signal(&view_focused);
        output->disconnect_signal(&view_geometry_changed);
        output->disconnect_signal(&view_minimized);
        output->disconnect_signal(&view_fullscreened);
        output->disconnect_signal(&wm_actions_above_changed);
        output->disconnect_signal(&view_change_workspace);
        output->disconnect_signal(&workspace_grid_changed);
        output->disconnect_signal(&workspace_changed);
    }

    void bind_view_events(wayfire_view view)
    {
        if (view == nullptr)
        {
            return;
        }

        view->connect_signal(IPC_WF_EVENT_VIEW_TITLE_CHANGED, &view_title_changed);
        view->connect_signal(IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED,
            &view_geometry_changed);
    }

    void unbind_view_events(wayfire_view view)
    {
        if (view == nullptr)
        {
            return;
        }

        view->disconnect_signal(&view_title_changed);
        view->disconnect_signal(&view_geometry_changed);
    }

    void bind_core_events()
    {
        wf::get_core().output_layout->connect_signal(IPC_WF_EVENT_OUTPUT_ADDED,
            &output_added);
        wf::get_core().output_layout->connect_signal(IPC_WF_EVENT_OUTPUT_REMOVED,
            &output_removed);
        wf::get_core().output_layout->connect_signal(
            IPC_WF_EVENT_OUTPUT_LAYOUT_CONFIGURATION_CHANGED,
            &output_layout_config_changed);
        wf::get_core().connect_signal(IPC_WF_EVENT_VIEW_HINTS_CHANGED,
            &view_hints_changed);
        wf::get_core().connect_signal(IPC_WF_EVENT_INPUT_DEVICE_ADDED,
            &input_device_added);
        wf::get_core().connect_signal(IPC_WF_EVENT_INPUT_DEVICE_REMOVED,
            &input_device_removed);
    }

    void unbind_core_events()
    {
        wf::get_core().output_layout->disconnect_signal(&output_added);
        wf::get_core().output_layout->disconnect_signal(&output_removed);
        wf::get_core().output_layout->disconnect_signal(&output_layout_config_changed);
        wf::get_core().disconnect_signal(&view_hints_changed);
        wf::get_core().disconnect_signal(&input_device_added);
        wf::get_core().disconnect_signal(&input_device_removed);
    }
};

DECLARE_WAYFIRE_PLUGIN(ipc_t);
