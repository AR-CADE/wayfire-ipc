#include <ipc/server.hpp>
#include <signals/wm-actions-signals.hpp>
#include "wayfire/per-output-plugin.hpp"
#include "wayfire/plugins/common/shared-core-data.hpp"
#include <sys/time.h>
#include <wayfire/core.hpp>
#include <wayfire/output.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/view.hpp>

using wf::option_wrapper_t;

option_wrapper_t<std::string> xkb_model{"input/xkb_model"};
option_wrapper_t<std::string> xkb_layout{"input/xkb_layout"};
option_wrapper_t<std::string> xkb_options{"input/xkb_options"};
option_wrapper_t<std::string> xkb_rules{"input/xkb_rules"};

// Damn, another variant ... hoppefully we have our event_mask, in the worst case, i
// can implement a completly useless workaroud (but mandatory ;-) ) to fix it !!!!
option_wrapper_t<std::string> xkb_variant{"input/xkb_variant"};
static struct wl_listener display_destroy;
static struct wl_event_source *fini_event_source;

class ipc_t : public wf::per_output_plugin_instance_t
{
    wf::shared_data::ref_ptr_t<ipc_server_t> get_instance;

  public:
    void init() override
    {
        get_instance->serve();

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
        auto instance = static_cast<ipc_t*>(data);
        instance->fini_timeout();
        return 0;
    }

    void fini() override
    {
        /* Notify exit */
        signal_shutdown_event();

        unbind_events();

        // Set a timeout of 100 ms to give some time to all clients to handle the
        // "exit" signal before disconnecting them and finish this instance of plugin
        display_destroy.notify = handle_display_destroy;
        wl_display_add_destroy_listener(wf::get_core().display, &display_destroy);
        fini_event_source =
            wl_event_loop_add_timer(wf::get_core().ev_loop, handle_fini_timeout,
                this);
        wl_event_source_timer_update(fini_event_source, 100);
    }

    void fini_timeout() const
    {
        ipc_t::handle_display_destroy(nullptr, nullptr);
    }

  private:

    void signal_event(const enum ipc_event_type & signal,
        const std::string & json_string) const
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

    Json::Value window_json_data(wayfire_view view, const std::string & change,
        bool demands_attention = false)
    {
        if (view == nullptr)
        {
            LOGE("View is null.");

            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
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
            container["urgent"] = demands_attention;
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
        wayfire_view view, const std::string & change,
        bool demands_attention = false)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        if (view == nullptr)
        {
            return;
        }

        Json::Value json = window_json_data(view, change, demands_attention);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped =
        [this] (wf::view_mapped_signal *ev)
    {
        bind_view_events(ev->view);
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "new");
    };

    wf::signal::connection_t<wf::view_unmapped_signal> on_view_unmapped =
        [this] (wf::view_unmapped_signal *ev)
    {
        unbind_view_events(ev->view);
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "close");
    };

    wf::signal::connection_t<wf::view_minimized_signal> on_view_minimized =
        [this] (wf::view_minimized_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "move");
    };

    wf::signal::connection_t<wf::view_fullscreen_signal> on_view_fullscreened =
        [this] (wf::view_fullscreen_signal *ev)
    {
        auto view = ev->view;
        if (view == nullptr)
        {
            return;
        }

        auto toplevel_view = wf::toplevel_cast(view);
        if (toplevel_view == nullptr)
        {
            return;
        }

        if (toplevel_view->toplevel()->current().fullscreen != ev->state)
        {
            signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view,
                "fullscreen_mode");
        }
    };

    wf::signal::connection_t<wf::keyboard_focus_changed_signal> on_keyboard_focus_changed =
        [this] (wf::keyboard_focus_changed_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, wf::node_to_view(ev->new_focus), "focus");
    };

    wf::signal::connection_t<wf::view_change_workspace_signal>
    on_view_change_workspace = [this] (wf::view_change_workspace_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "move");
    };

    wf::signal::connection_t<wf::wm_actions_above_changed_signal>
    on_wm_actions_above_changed = [this] (wf::wm_actions_above_changed_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "floating");
    };

    wf::signal::connection_t<wf::view_title_changed_signal> on_view_title_changed =
        [this] (wf::view_title_changed_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "title");
    };
    wf::signal::connection_t<wf::view_geometry_changed_signal>
    on_view_geometry_changed = [this] (wf::view_geometry_changed_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "move");
    };

    wf::signal::connection_t<wf::view_hints_changed_signal> on_view_hints_changed =
        [=] (wf::view_hints_changed_signal *ev)
    {
        signal_window_event(IPC_I3_EVENT_TYPE_WINDOW, ev->view, "urgent",
            ev->demands_attention);
    };

    Json::Value output_json_data(wf::output_t *output, const std::string & change)
    {
        (void)output;
        // if (output == nullptr)
        // {
        // LOGE("Output is null.");

        // return Json::nullValue;
        // }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;
        // object["output"] = ipc_json::describe_output(output);

        return object;
    }

    void signal_output_event(const enum ipc_event_type & signal,
        wf::output_t *output, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        // if (output == nullptr)
        // {
        // return;
        // }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        Json::Value json = output_json_data(output, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal::connection_t<wf::output_added_signal> on_output_added =
        [=] (wf::output_added_signal *ev)
    {
        bind_output_events(ev->output);
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, ev->output,
            /*"new"*/ "unspecified");
    };

    wf::signal::connection_t<wf::output_removed_signal> on_output_removed =
        [=] (wf::output_removed_signal *ev)
    {
        unbind_output_events(ev->output);
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, ev->output,
            /*"close"*/ "unspecified");
    };

    wf::signal::connection_t<wf::output_layout_configuration_changed_signal>
    on_output_layout_configuration_changed =
        [=] (wf::output_layout_configuration_changed_signal *ev)
    {
        (void)ev;
        signal_output_event(IPC_I3_EVENT_TYPE_OUTPUT, nullptr,
            /*"change"*/ "unspecified");
    };

    Json::Value input_json_data(nonstd::observer_ptr<wf::input_device_t> device,
        const std::string & change)
    {
        if (device == nullptr)
        {
            return Json::nullValue;
        }

        if (ipc_server_t::client_count() == 0)
        {
            return Json::nullValue;
        }

        Json::Value object;
        object["change"] = change;
        object["input"]  = ipc_json::describe_input(device);

        return object;
    }

    void signal_input_event(const enum ipc_event_type & signal,
        nonstd::observer_ptr<wf::input_device_t> device, const std::string & change)
    {
        if (signal == IPC_WF_EVENT_TYPE_NONE)
        {
            return;
        }

        if (device == nullptr)
        {
            LOGE("Device is null.");

            return;
        }

        if (!ipc_server_t::has_event_listeners(signal))
        {
            return;
        }

        Json::Value json = input_json_data(device, change);
        if (json.isNull() == false)
        {
            signal_event(signal, ipc_json::json_to_string(json));
        }
    }

    wf::signal::connection_t<wf::input_device_added_signal> on_input_device_added =
        [=] (wf::input_device_added_signal *ev)
    {
        signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, ev->device, "added");
    };

    wf::signal::connection_t<wf::input_device_removed_signal> on_input_device_removed
        =
            [=] (wf::input_device_removed_signal *ev)
    {
        signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, ev->device, "removed");
    };

    void xkb_change()
    {
        auto devices = wf::get_core().get_input_devices();
        for (auto dev : devices)
        {
            auto wlr_handle = dev->get_wlr_handle();

            if (wlr_handle != nullptr)
            {
                if ((wlr_handle->product == 1) && (wlr_handle->vendor == 1))
                {
                    signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, dev,
                        "xkb_layout");
                    signal_input_event(IPC_SWAY_EVENT_TYPE_INPUT, dev,
                        "xkb_keymap");
                }
            }
        }
    }

    Json::Value workspace_json_data(wf::workspace_changed_signal *data,
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

        auto outputs = wf::get_core().output_layout->get_outputs();

        for (wf::output_t *output : outputs)
        {
            if (data->output == output)
            {
                auto wsize = output->wset()->get_workspace_grid_size();
                for (int x = 0; x < wsize.width; x++)
                {
                    for (int y = 0; y < wsize.height; y++)
                    {
                        wf::point_t point{x, y};
                        if (point == data->new_viewport)
                        {
                            Json::Value workspace = ipc_json::describe_workspace(
                                wf::point_t{x, y}, output);
                            object["current"] = workspace;
                        }

                        if (point == data->old_viewport)
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
        wf::workspace_changed_signal *data, const std::string & change)
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

    wf::signal::connection_t<wf::workspace_changed_signal> on_workspace_changed =
        [this] (wf::workspace_changed_signal *ev)
    {
        signal_workspace_event(IPC_I3_EVENT_TYPE_WORKSPACE, ev, "focus");
    };

    wf::signal::connection_t<wf::workspace_grid_changed_signal>
    on_workspace_grid_changed = [this] (wf::workspace_grid_changed_signal *ev)
    {
        (void)ev;
        wf::workspace_changed_signal signal;
        signal.output = wf::get_core().seat->get_active_output();
        signal.new_viewport = signal.output->wset()->get_current_workspace();
        signal_workspace_event(IPC_I3_EVENT_TYPE_WORKSPACE, &signal, "reload");
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

        //
        // Connect options change signal
        //
        xkb_model.set_callback([this] () { xkb_change(); });
        xkb_variant.set_callback([this] () { xkb_change(); });
        xkb_layout.set_callback([this] () { xkb_change(); });
        xkb_options.set_callback([this] () { xkb_change(); });
        xkb_rules.set_callback([this] () { xkb_change(); });
    }

    void unbind_events()
    {
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

        //
        // Disconnect core signals
        //
        unbind_core_events();

        //
        // Disconnect options change signal
        //
        xkb_model.set_callback(nullptr);
        xkb_variant.set_callback(nullptr);
        xkb_layout.set_callback(nullptr);
        xkb_options.set_callback(nullptr);
        xkb_rules.set_callback(nullptr);
    }

    void bind_output_events(wf::output_t *output)
    {
        if (output == nullptr)
        {
            return;
        }

        output->connect(&on_view_mapped);
        output->connect(&on_view_unmapped);
        output->connect(&on_keyboard_focus_changed);
        output->connect(&on_view_geometry_changed);
        output->connect(&on_view_minimized);
        output->connect(&on_view_fullscreened);
        output->connect(&on_wm_actions_above_changed);
        output->connect(&on_view_change_workspace);
        output->connect(&on_workspace_grid_changed);
        output->connect(&on_workspace_changed);
    }

    void unbind_output_events(wf::output_t *output)
    {
        if (output == nullptr)
        {
            return;
        }

        output->disconnect(&on_view_mapped);
        output->disconnect(&on_view_unmapped);
        output->disconnect(&on_keyboard_focus_changed);
        output->disconnect(&on_view_geometry_changed);
        output->disconnect(&on_view_minimized);
        output->disconnect(&on_view_fullscreened);
        output->disconnect(&on_wm_actions_above_changed);
        output->disconnect(&on_view_change_workspace);
        output->disconnect(&on_workspace_grid_changed);
        output->disconnect(&on_workspace_changed);
    }

    void bind_view_events(wayfire_view view)
    {
        if (view == nullptr)
        {
            return;
        }

        view->connect(&on_view_title_changed);
        view->connect(&on_view_geometry_changed);
    }

    void unbind_view_events(wayfire_view view)
    {
        if (view == nullptr)
        {
            return;
        }

        view->disconnect(&on_view_title_changed);
        view->disconnect(&on_view_geometry_changed);
    }

    void bind_core_events()
    {
        wf::get_core().output_layout->connect(&on_output_added);
        wf::get_core().output_layout->connect(&on_output_removed);
        wf::get_core().output_layout->connect(&on_output_layout_configuration_changed);
        wf::get_core().connect(&on_view_hints_changed);
        wf::get_core().connect(&on_input_device_added);
        wf::get_core().connect(&on_input_device_removed);
    }

    void unbind_core_events()
    {
        wf::get_core().output_layout->disconnect(&on_output_added);
        wf::get_core().output_layout->disconnect(&on_output_removed);
        wf::get_core().output_layout->disconnect(
            &on_output_layout_configuration_changed);
        wf::get_core().disconnect(&on_view_hints_changed);
        wf::get_core().disconnect(&on_input_device_added);
        wf::get_core().disconnect(&on_input_device_removed);
    }
};

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<ipc_t>);
