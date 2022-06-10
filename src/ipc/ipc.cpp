#include <ipc/server.hpp>
#include "wayfire/singleton-plugin.hpp"
#include <sys/time.h>

// #define IPC_PLUGIN_DEBUG

wf::option_wrapper_t<std::string> xkb_model{"input/xkb_model"};
wf::option_wrapper_t<std::string> xkb_layout{"input/xkb_layout"};
wf::option_wrapper_t<std::string> xkb_options{"input/xkb_options"};
wf::option_wrapper_t<std::string> xkb_rules{"input/xkb_rules"};

// Damn, another variant ... hoppefully we have our event_mask, in the worst case, i
// can implement a completly useless workaroud (but mandatory ;-) ) to fix it !!!!
wf::option_wrapper_t<std::string> xkb_variant{"input/xkb_variant"};
// static struct wl_listener display_destroy;
// static struct wl_event_source *fini_event_source;

class ipc_t : public wf::singleton_plugin_t<ipc_server_t>
{
  public:
    void init() override
    {
        grab_interface->name = "ipc";
        grab_interface->capabilities = 0;
        singleton_plugin_t::init();

        get_instance().serve();
        signal_shutdown_event();

        bind_events();
    }

    static void timeout_handler(int signum)
    {
        (void)signum;
        ipc_server_t::handle_display_destroy(nullptr, nullptr);
    }

    static void handle_display_destroy(struct wl_listener *listener,
        void *data)
    {
        (void)listener;
        (void)data;

        // if (fini_event_source != nullptr)
        // {
        // wl_event_source_remove(fini_event_source);
        // }

        // wl_list_remove(&display_destroy.link);
    }

    static int handle_fini_timeout(void *data)
    {
        (void)data;
        ipc_server_t::handle_display_destroy(nullptr, nullptr);
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

        //// Set a timeout of 100 ms to give so time to all  client to handle the
        // "exit" signal
        // struct itimerval timer;
        // timer.it_value.tv_sec = 0;
        // timer.it_value.tv_usec = 100;
        // timer.it_interval.tv_sec = 0;
        // timer.it_interval.tv_usec = 0;
        // setitimer (ITIMER_VIRTUAL, &timer, 0);

        // struct sigaction sa;
        // memset (&sa, 0, sizeof (sa));
        // sa.sa_handler = &timeout_handler;
        // sigaction (SIGVTALRM, &sa, 0);

        // display_destroy.notify = handle_display_destroy;
        // wl_display_add_destroy_listener(wf::get_core().display, &display_destroy);

        // fini_event_source =
        // wl_event_loop_add_timer(wf::get_core().ev_loop, handle_fini_timeout,
        // nullptr);

        // wl_event_source_timer_update(fini_event_source, 100);

        /*wf::wl_timer timer;
         *  if (!timer.is_connected())
         *  {
         *    timer.set_timeout(100, [&] ()
         *    {
         *        // Destroy plugin ipc_server_t::handle_display_destroy(nullptr,
         * nullptr);
         *        return false;
         *    });
         *  }*/

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
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_MAPPED, data,
        // IPC_WF_EVENT_VIEW_MAPPED);
    };

    wf::signal_connection_t view_unmapped = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "close");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_UNMAPPED, data,
        // IPC_WF_EVENT_VIEW_UNMAPPED);
    };

    wf::signal_connection_t view_tiled = [=] (wf::signal_data_t *data)
    {
        // signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_TILED, data,
            IPC_WF_EVENT_VIEW_TILED);
    };

    wf::signal_connection_t view_minimized = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_MINIMIZED, data,
        // IPC_WF_EVENT_VIEW_MINIMIZED);
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
            // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_FULLSCREENED, data,
            // IPC_WF_EVENT_VIEW_FULLSCREENED);
        }
    };

    wf::signal_connection_t view_layer_attached = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_LAYER_ATTACHED, data,
            IPC_WF_EVENT_VIEW_LAYER_ATTACHED);
    };

    wf::signal_connection_t view_layer_detached = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_LAYER_DETACHED, data,
            IPC_WF_EVENT_VIEW_LAYER_DETACHED);
    };

    wf::signal_connection_t view_focused = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "focus");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_FOCUSED, data,
        // IPC_WF_EVENT_VIEW_FOCUSED);
    };

    wf::signal_connection_t view_change_workspace = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_CHANGE_WORKSPACE, data,
        // IPC_WF_EVENT_VIEW_CHANGE_WORKSPACE);
    };

    wf::signal_connection_t view_pre_moved_to_output = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_PRE_MOVED_TO_OUTPUT, data,
            IPC_WF_EVENT_VIEW_PRE_MOVED_TO_OUTPUT);
    };

    wf::signal_connection_t view_attached = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_ATTACHED, data,
            IPC_WF_EVENT_VIEW_ATTACHED);
    };

    wf::signal_connection_t view_detached = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_DETACHED, data,
            IPC_WF_EVENT_VIEW_DETACHED);
    };

    wf::signal_connection_t view_set_sticky = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_SET_STICKY, data,
            IPC_WF_EVENT_VIEW_SET_STICKY);
    };

    wf::signal_connection_t wm_actions_above_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "floating");
    };

    wf::signal_connection_t view_app_id_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_APP_ID_CHANGED, data,
            IPC_WF_EVENT_VIEW_APP_ID_CHANGED);
    };


    wf::signal_connection_t view_title_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "title");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_TITLE_CHANGED, data,
        // IPC_WF_EVENT_VIEW_TITLE_CHANGED);
    };

    wf::signal_connection_t view_geometry_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "move");

        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_GEOMETRY_CHANGED, data,
        // IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED);
    };

    wf::signal_connection_t view_above_changed = [=] (wf::signal_data_t *data)
    {
        // signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "focus");

        signal_window_event(IPC_WF_EVENT_TYPE_VIEW_ABOVE_CHANGED, data,
            IPC_WF_EVENT_VIEW_ABOVE_CHANGED);
    };

    wf::signal_connection_t view_hints_changed = [=] (wf::signal_data_t *data)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, data, "urgent");
        // signal_window_event(IPC_WF_EVENT_TYPE_VIEW_HINTS_CHANGED, data,
        // IPC_WF_EVENT_VIEW_HINTS_CHANGED);
    };


    Json::Value output_json_data(wf::signal_data_t *data, const std::string & change)
    {
        if (data == nullptr)
        {
            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        auto output = get_signaled_output(data);

        if (output == nullptr)
        {
            LOGE("Output is null.");

            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;
        object["output"] = ipc_json::describe_output(output);

        return object;
    }

    void signal_output_event(const enum ipc_event_type & signal,
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

        Json::Value json = output_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal_connection_t output_added = [=] (wf::signal_data_t *data)
    {
        bind_output_events(get_signaled_output(data));
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, data, "new");
        // signal_output_event(IPC_WF_EVENT_TYPE_OUTPUT_ADDED, data,
        // IPC_WF_EVENT_OUTPUT_ADDED);
    };

    wf::signal_connection_t output_removed = [=] (wf::signal_data_t *data)
    {
        // should we explicitly disconnect the event handler
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, data, "close");
        // signal_output_event(IPC_WF_EVENT_TYPE_OUTPUT_REMOVED, data,
        // IPC_WF_EVENT_OUTPUT_REMOVED);
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
            // signal.output =
            // static_cast<wf::workspace_changed_signal*>(data)->output;
            // signal.new_viewport =
            // static_cast<wf::workspace_changed_signal*>(data)->new_viewport;
            // signal.old_viewport =
            // static_cast<wf::workspace_changed_signal*>(data)->old_viewport;
            // signal.carried_out =
            // static_cast<wf::workspace_changed_signal*>(data)->carried_out;
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

    std::string binding_json_data(wf::signal_data_t *data,
        const std::string & change = "run")
    {
        if (data == nullptr)
        {
            return nullptr;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return nullptr;
        }

        Json::Value object;
        object["change"] = change;

        if (change == "run")
        {
            // object["binding"] = ;
        }

        return ipc_json::json_to_string(object);
    }

    void signal_binding_event(const enum ipc_event_type & signal,
        wf::signal_data_t *data, const std::string & change = "run")
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

        Json::Value json = binding_json_data(data, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

/*     wf::signal_connection_t output_configuration_changed{
 *       [=] (wf::signal_data_t *data)
 *       {
 *           (void)data;
 *
 *           if (client_count() == 0)
 *           {
 *               return;
 *           }
 *
 *           Json::Value object;
 *           std::string json_string = ipc_json::json_to_string(object);
 *           send_event(json_string.c_str(),
 * IPC_WF_EVENT_TYPE_OUTPUT_CONFIGURATION_CHANGED);
 *       }
 *   };
 *
 *   wf::signal_connection_t view_moved_to_output{[=] (wf::signal_data_t *data)
 *       {
 *
 *           wf::view_moved_to_output_signal *signal;
 *           wayfire_view view;
 *
 *           if (client_count() == 0)
 *           {
 *               return;
 *           }
 *
 *           signal = static_cast<wf::view_moved_to_output_signal*>(data);
 *           view   = signal->view;
 *
 *           if (!view)
 *           {
 *               LOGE("view_moved_to_output no view");
 *               return;
 *           }
 *
 *           Json::Value object;
 *           object["view"] = ipc_json::describe_view(view);
 *           object["old_output"] = ipc_json::describe_output(signal->old_output);
 *           object["new_output"] = ipc_json::describe_output(signal->new_output);
 *
 *           std::string json_string = ipc_json::json_to_string(object);
 *           send_event(json_string.c_str(), IPC_WF_EVENT_TYPE_VIEW_MOVED_TO_OUTPUT);
 *       }
 *   };
 *
 *
 *   wf::signal_connection_t pointer_button{[=] (wf::signal_data_t *data)
 *       {
 *           wf::pointf_t cursor_position;
 *           wf::input_event_signal<wlr_event_pointer_button> *wf_ev;
 *           wlr_event_pointer_button *wlr_signal;
 *           wlr_button_state button_state;
 *           bool button_released;
 *           uint32_t button;
 *
 *           if (client_count() == 0)
 *           {
 *               return;
 *           }
 *
 *           cursor_position = wf::get_core().get_cursor_position();
 *           wf_ev =
 *
 *              static_cast<wf::input_event_signal<wlr_event_pointer_button>*>(data);
 *           wlr_signal   = static_cast<wlr_event_pointer_button*>(wf_ev->event);
 *           button_state = wlr_signal->state;
 *           button = wlr_signal->button;
 *           button_released = (button_state == WLR_BUTTON_RELEASED);
 *
 *           // if (find_view_under_action && button_released) {
 *           // GVariant *_signal_data;
 *           // wayfire_view view;
 *           // view = core.get_view_at(cursor_position);
 *           // _signal_data = g_variant_new("(u)", view ? view->get_id() : 0);
 *           // g_variant_ref(_signal_data);
 *           // bus_emit_signal("view_pressed", _signal_data);
 *           // }
 *
 *           Json::Value object;
 *           object["cursor_position"] = ipc_json::describe_pointf(cursor_position);
 *           object["button"] = button;
 *           object["button_released"] = button_released;
 *           std::string json_string = ipc_json::json_to_string(object);
 *           send_event(json_string.c_str(), IPC_WF_EVENT_TYPE_POINTER_BUTTON);
 *       }
 *   };
 *
 *   wf::signal_connection_t tablet_button{[=] (wf::signal_data_t *data)
 *       {
 *           (void)data;
 *
 *           if (client_count() == 0)
 *           {
 *               return;
 *           }
 *
 *           Json::Value object;
 *           std::string json_string = ipc_json::json_to_string(object);
 *           send_event(json_string.c_str(), IPC_WF_EVENT_TYPE_TABLET_BUTTON);
 *       }
 *   }; */


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
        output->connect_signal(IPC_WF_EVENT_VIEW_TILED, &view_tiled);
        output->connect_signal(IPC_WF_EVENT_VIEW_FOCUSED, &view_focused);
        output->connect_signal(IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED,
            &view_geometry_changed);

        output->connect_signal(IPC_WF_EVENT_VIEW_MINIMIZED, &view_minimized);
        output->connect_signal(IPC_WF_EVENT_VIEW_FULLSCREENED, &view_fullscreened);
        output->connect_signal(IPC_WF_EVENT_VIEW_SET_STICKY, &view_set_sticky);
        output->connect_signal("wm-actions-above-changed",
            &wm_actions_above_changed);
        output->connect_signal(IPC_WF_EVENT_VIEW_CHANGE_WORKSPACE,
            &view_change_workspace);

        // output->connect_signal(IPC_WF_EVENT_VIEW_LAYER_ATTACHED,
        // &view_layer_attached);
        // output->connect_signal(IPC_WF_EVENT_VIEW_LAYER_DETACHED,
        // &view_layer_detached);
        // output->connect_signal(IPC_WF_EVENT_VIEW_ATTACHED, &view_attached);
        // output->connect_signal(IPC_WF_EVENT_VIEW_DETACHED, &view_detached);
        // output->connect_signal(IPC_WF_EVENT_VIEW_ABOVE_CHANGED, &view_focused);
        // output->connect_signal(IPC_WF_EVENT_OUTPUT_CONFIGURATION_CHANGED,
        // &output_configuration_changed);
        // output->connect_signal("configuration-changed",
        // &on_output_config_changed);
        // output->connect_signal("workarea-changed", &on_workarea_changed);
        // output->connect_signal("scale-update", &update_cb);

        output->connect_signal("workspace-grid-changed", &workspace_grid_changed);
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
        output->disconnect_signal(&view_tiled);
        output->disconnect_signal(&view_focused);
        output->disconnect_signal(&view_geometry_changed);

        output->disconnect_signal(&view_minimized);
        output->disconnect_signal(&view_fullscreened);
        output->disconnect_signal(&view_set_sticky);
        output->disconnect_signal(&wm_actions_above_changed);
        output->disconnect_signal(&view_change_workspace);

        // output->disconnect_signal(&view_layer_attached);
        // output->disconnect_signal(&view_layer_detached);
        // output->disconnect_signal(&view_attached);
        // output->disconnect_signal(&view_detached);
        // output->disconnect_signal(&view_focused);
        // output->disconnect_signal(&output_configuration_changed);
        // output->disconnect_signal(&on_output_config_changed);
        // output->disconnect_signal(&on_workarea_changed);
        // output->disconnect_signal(&update_cb);

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

        // view->connect_signal("fullscreen", &view_fullscreened);
        // view->connect_signal(IPC_WF_EVENT_VIEW_APP_ID_CHANGED,
        // &view_app_id_changed);
        // view->connect_signal("decoration-changed", &on_decoration_changed);
    }

    void unbind_view_events(wayfire_view view)
    {
        if (view == nullptr)
        {
            return;
        }

        view->disconnect_signal(&view_title_changed);
        view->disconnect_signal(&view_geometry_changed);

        // view->disconnect_signal(&view_fullscreened);
        // view->disconnect_signal(&view_app_id_changed);
        // view->disconnect_signal(&on_decoration_changed);
    }

    void bind_core_events()
    {
        wf::get_core().connect_signal(IPC_WF_EVENT_VIEW_PRE_MOVED_TO_OUTPUT,
            &view_pre_moved_to_output);
        wf::get_core().output_layout->connect_signal(IPC_WF_EVENT_OUTPUT_ADDED,
            &output_added);
        wf::get_core().output_layout->connect_signal(IPC_WF_EVENT_OUTPUT_REMOVED,
            &output_removed);
        wf::get_core().connect_signal(IPC_WF_EVENT_VIEW_HINTS_CHANGED,
            &view_hints_changed);
        wf::get_core().connect_signal("input-device-added", &input_device_added);
        wf::get_core().connect_signal("input-device-removed", &input_device_removed);

        // wf::get_core().connect_signal(IPC_WF_EVENT_VIEW_MOVED_TO_OUTPUT,
        // &view_moved_to_output);
        // wf::get_core().connect_signal(IPC_WF_EVENT_POINTER_BUTTON,
        // &pointer_button);
        // wf::get_core().connect_signal(IPC_WF_EVENT_TABLET_BUTTON, &tablet_button);

/*
 *
 *       wf::get_core().connect_signal("pointer_motion", &on_motion_event);
 *       wf::get_core().connect_signal("tablet_axis", &on_motion_event);
 *       wf::get_core().connect_signal("touch_motion", &on_touch_motion_event);
 *
 *       wf::get_core().connect_signal("reload-config", &_reload_config);
 *       wf::get_core().connect_signal("keyboard_key", &on_key_event);
 *
 *       wf::get_core().connect_signal("input-device-added", &input_device_added);
 *       wf::get_core().connect_signal("input-device-removed",
 * &input_device_removed);
 *
 *       wf::get_core().connect_signal("pointer_swipe_begin", &on_swipe_begin);
 *       wf::get_core().connect_signal("pointer_swipe_update", &on_swipe_update);
 *       wf::get_core().connect_signal("pointer_swipe_end", &on_swipe_end);
 *
 *       wf::get_core().connect_signal("output-stack-order-changed",
 * &on_views_updated);
 *       wf::get_core().connect_signal("view-geometry-changed", &on_views_updated);
 */
    }

    void unbind_core_events()
    {
        wf::get_core().disconnect_signal(&view_pre_moved_to_output);
        wf::get_core().output_layout->disconnect_signal(&output_added);
        wf::get_core().output_layout->disconnect_signal(&output_removed);
        wf::get_core().disconnect_signal(&view_hints_changed);
        wf::get_core().disconnect_signal(&input_device_added);
        wf::get_core().disconnect_signal(&input_device_removed);

        // wf::get_core().disconnect_signal(&view_moved_to_output);
        // wf::get_core().disconnect_signal(&pointer_button);
        // wf::get_core().disconnect_signal(&tablet_button);

/*
 *
 *       wf::get_core().disconnect_signal(&on_motion_event);
 *       wf::get_core().disconnect_signal(&on_motion_event);
 *       wf::get_core().disconnect_signal(&on_touch_motion_event);
 *
 *       wf::get_core().disconnect_signal(&_reload_config);
 *       wf::get_core().disconnect_signal(&on_key_event);
 *
 *       wf::get_core().disconnect_signal(&input_device_added);
 *       wf::get_core().disconnect_signal(&input_device_removed);
 *
 *       wf::get_core().disconnect_signal(&on_swipe_begin);
 *       wf::get_core().disconnect_signal( &on_swipe_update);
 *       wf::get_core().disconnect_signal(&on_swipe_end);
 *
 *       wf::get_core().disconnect_signal(&on_views_updated);
 *       wf::get_core().disconnect_signal(&on_views_updated);
 */
    }
};

DECLARE_WAYFIRE_PLUGIN(ipc_t);
