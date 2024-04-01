#include <json/value.h>
#include <signals/power-signals.hpp>
#include <wayfire/output.hpp>
#include "wayfire/per-output-plugin.hpp"
#include "wayfire/plugins/common/shared-core-data.hpp"
#include <wayfire/output-layout.hpp>
#include <wayfire/signal-definitions.hpp>
#include <ipc/command.h>
#include <ipc/json.hpp>
#include <wayfire/toplevel-view.hpp>

class wayfire_power
{
    wf::option_wrapper_t<int> power_timeout{"power/power_timeout"};
    bool is_idle = false;

  public:
    wf::signal::connection_t<wf::seat_activity_signal> on_seat_activity;
    std::optional<wf::idle_inhibitor_t> hotkey_inhibitor;
    wf::wl_timer<false> timeout_power;

    wayfire_power()
    {
        power_timeout.set_callback([=] ()
        {
            create_power_timeout();
        });

        on_seat_activity = [=] (void*)
        {
            create_power_timeout();
        };
        create_power_timeout();
        wf::get_core().connect(&on_seat_activity);
    }

    void create_power_timeout()
    {
        if (power_timeout <= 0)
        {
            timeout_power.disconnect();
            return;
        }

        if (!timeout_power.is_connected() && is_idle)
        {
            is_idle = false;
            set_state(wf::OUTPUT_IMAGE_SOURCE_DPMS, wf::OUTPUT_IMAGE_SOURCE_SELF);

            return;
        }

        timeout_power.disconnect();
        timeout_power.set_timeout(1000 * power_timeout, [=] ()
        {
            is_idle = true;
            set_state(wf::OUTPUT_IMAGE_SOURCE_SELF, wf::OUTPUT_IMAGE_SOURCE_DPMS);
        });
    }

    ~wayfire_power()
    {
        timeout_power.disconnect();
        wf::get_core().disconnect(&on_seat_activity);
    }

    /* Change all outputs with state from to state to */
    void set_state(wf::output_image_source_t from, wf::output_image_source_t to)
    {
        auto config = wf::get_core().output_layout->get_current_configuration();

        for (auto& entry : config)
        {
            if (entry.second.source == from)
            {
                entry.second.source = to;
            }
        }

        wf::get_core().output_layout->apply_configuration(config);
    }

    void toggle_state()
    {
        auto config = wf::get_core().output_layout->get_current_configuration();

        for (auto& entry : config)
        {
            entry.second.source =
                (entry.second.source ==
                    wf::OUTPUT_IMAGE_SOURCE_SELF) ? wf::OUTPUT_IMAGE_SOURCE_DPMS : wf
                ::
                OUTPUT_IMAGE_SOURCE_SELF;
        }

        wf::get_core().output_layout->apply_configuration(config);
    }

    void set_state_off(Json::Value name_or_id)
    {
        bool apply = false;

        auto config = wf::get_core().output_layout->get_current_configuration();

        wf::output_t *output = nullptr;

        if (!(name_or_id.isString() && (name_or_id.asString() == IPC_ANY_TOCKEN)))
        {
            std::string str_name_or_id = name_or_id.asString();
            output = command::all_output_by_name_or_id(str_name_or_id.c_str());

            if (!output)
            {
                return;
            }
        }

        for (auto& entry : config)
        {
            auto o = wf::get_core().output_layout->find_output(entry.first);

            if (output &&
                !(name_or_id.isString() &&
                  (name_or_id.asString() == IPC_ANY_TOCKEN)))
            {
                if (o->get_id() != output->get_id())
                {
                    continue;
                }
            }

            if (entry.second.source != wf::OUTPUT_IMAGE_SOURCE_DPMS)
            {
                entry.second.source = wf::OUTPUT_IMAGE_SOURCE_DPMS;
                apply = true;
            }
        }

        if (apply)
        {
            wf::get_core().output_layout->apply_configuration(config);
        }
    }

    void set_state_on(Json::Value name_or_id)
    {
        bool apply = false;

        auto config = wf::get_core().output_layout->get_current_configuration();

        wf::output_t *output = nullptr;

        if (!(name_or_id.isString() && (name_or_id.asString() == IPC_ANY_TOCKEN)))
        {
            std::string str_name_or_id = name_or_id.asString();
            output = command::all_output_by_name_or_id(str_name_or_id.c_str());

            if (!output)
            {
                return;
            }
        }

        for (auto& entry : config)
        {
            auto o = wf::get_core().output_layout->find_output(entry.first);

            if (output &&
                !(name_or_id.isString() &&
                  (name_or_id.asString() == IPC_ANY_TOCKEN)))
            {
                if (o->get_id() != output->get_id())
                {
                    continue;
                }
            }

            if (entry.second.source != wf::OUTPUT_IMAGE_SOURCE_SELF)
            {
                entry.second.source = wf::OUTPUT_IMAGE_SOURCE_SELF;
                apply = true;
            }
        }

        if (apply)
        {
            wf::get_core().output_layout->apply_configuration(config);
        }
    }
};

class wayfire_power_plugin : public wf::per_output_plugin_instance_t
{
    wf::option_wrapper_t<bool> disable_on_fullscreen{"power/disable_on_fullscreen"};
    wf::option_wrapper_t<bool> disable_initially{"power/disable_initially"};

    std::optional<wf::idle_inhibitor_t> fullscreen_inhibitor;
    bool has_fullscreen = false;
    wf::shared_data::ref_ptr_t<wayfire_power> global_power;

    wf::plugin_activation_data_t grab_interface = {
        .name = "power",
        .capabilities = 0,
    };

    wf::activator_callback toggle = [=] (auto)
    {
        if (global_power->hotkey_inhibitor.has_value())
        {
            global_power->hotkey_inhibitor.reset();
        } else
        {
            global_power->hotkey_inhibitor.emplace();
        }

        return true;
    };

    wf::signal::connection_t<wf::fullscreen_layer_focused_signal>
    fullscreen_state_changed =
        [=] (wf::fullscreen_layer_focused_signal *ev)
    {
        this->has_fullscreen = ev->has_promoted;
        update_fullscreen();
    };

    wf::signal::connection_t<wf::idle_inhibit_changed_signal> inhibit_changed =
        [=] (wf::idle_inhibit_changed_signal *ev)
    {
        if (!ev)
        {
            return;
        }

        if (ev->inhibit)
        {
            wf::get_core().disconnect(&global_power->on_seat_activity);
            global_power->timeout_power.disconnect();
        } else
        {
            wf::get_core().connect(&global_power->on_seat_activity);
            global_power->create_power_timeout();
        }
    };

    wf::config::option_base_t::updated_callback_t disable_on_fullscreen_changed =
        [=] ()
    {
        update_fullscreen();
    };


    wf::signal::connection_t<power_command_signal> on_power_command =
        [=] (power_command_signal *ev)
    {
        if (!output->can_activate_plugin(&grab_interface))
        {
            return;
        }

        Json::Value argv = ev->argv;
        //
        // argv[0] = "output"
        // argv[1] = eg. "*" or 1 or "eDP-1"
        // argv[2] = "power"
        // argv[3] = "on" or "off"
        //
        Json::Value option     = argv.get(Json::ArrayIndex(3), IPC_ON_TOCKEN);
        Json::Value name_or_id = argv.get(Json::ArrayIndex(1), IPC_ANY_TOCKEN);

        if ((option.isString() == false) || option.empty())
        {
            return;
        }

        if (option.asString() == IPC_OFF_TOCKEN)
        {
            global_power->set_state_off(name_or_id);
        } else
        {
            global_power->set_state_on(name_or_id);
        }
    };

    void update_fullscreen()
    {
        bool want = disable_on_fullscreen && has_fullscreen;
        if (want && !fullscreen_inhibitor.has_value())
        {
            fullscreen_inhibitor.emplace();
        }

        if (!want && fullscreen_inhibitor.has_value())
        {
            fullscreen_inhibitor.reset();
        }
    }

  public:
    void init() override
    {
        if (disable_initially)
        {
            global_power->hotkey_inhibitor.emplace();
        }

        output->add_activator(
            wf::option_wrapper_t<wf::activatorbinding_t>{"power/toggle"},
            &toggle);

        output->connect(&fullscreen_state_changed);
        disable_on_fullscreen.set_callback(disable_on_fullscreen_changed);

        if (auto toplevel = toplevel_cast(wf::get_active_view_for_output(output)))
        {
            /* Currently, the fullscreen count would always be 0 or 1,
             * since fullscreen-layer-focused is only emitted on changes between 0
             * and 1
             **/
            has_fullscreen = toplevel->pending_fullscreen();
        }

        update_fullscreen();

        output->connect(&on_power_command);
        wf::get_core().connect(&inhibit_changed);
    }

    void fini() override
    {
        wf::get_core().disconnect(&inhibit_changed);
        output->disconnect(&fullscreen_state_changed);
        output->disconnect(&on_power_command);
        output->rem_binding(&toggle);
    }
};

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<wayfire_power_plugin>);
