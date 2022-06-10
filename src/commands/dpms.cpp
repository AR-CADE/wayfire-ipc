#include <json/value.h>
#include <wayfire/output.hpp>
#include <wayfire/singleton-plugin.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/workspace-manager.hpp>
#include <ipc/command.h>
#include <ipc/json.hpp>

class wayfire_dpms
{
    wf::option_wrapper_t<int> dpms_timeout{"dpms/dpms_timeout"};
    wf::wl_listener_wrapper on_idle_dpms, on_resume_dpms;
    wlr_idle_timeout *timeout_dpms = nullptr;

  public:
    std::optional<wf::idle_inhibitor_t> hotkey_inhibitor;

    wayfire_dpms()
    {
        dpms_timeout.set_callback([=] ()
        {
            create_dpms_timeout(1000 * dpms_timeout);
        });
        create_dpms_timeout(1000 * dpms_timeout);
    }

    wayfire_dpms(const wayfire_dpms&) = delete;
    wayfire_dpms& operator =(const wayfire_dpms&) = delete;
    wayfire_dpms(wayfire_dpms&&) = delete;
    wayfire_dpms& operator =(wayfire_dpms&&) = delete;

    void destroy_dpms_timeout()
    {
        if (timeout_dpms)
        {
            on_idle_dpms.disconnect();
            on_resume_dpms.disconnect();
            wlr_idle_timeout_destroy(timeout_dpms);
        }

        timeout_dpms = nullptr;
    }

    void create_dpms_timeout(int timeout_sec)
    {
        destroy_dpms_timeout();
        if (timeout_sec <= 0)
        {
            return;
        }

        timeout_dpms = wlr_idle_timeout_create(wf::get_core().protocols.idle,
            wf::get_core().get_current_seat(), timeout_sec);

        on_idle_dpms.set_callback([&] (void*)
        {
            set_state(wf::OUTPUT_IMAGE_SOURCE_SELF, wf::OUTPUT_IMAGE_SOURCE_DPMS);
        });
        on_idle_dpms.connect(&timeout_dpms->events.idle);

        on_resume_dpms.set_callback([&] (void*)
        {
            set_state(wf::OUTPUT_IMAGE_SOURCE_DPMS, wf::OUTPUT_IMAGE_SOURCE_SELF);
        });
        on_resume_dpms.connect(&timeout_dpms->events.resume);
    }

    ~wayfire_dpms()
    {
        destroy_dpms_timeout();
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

class wayfire_dpms_singleton : public wf::singleton_plugin_t<wayfire_dpms>
{
    wf::option_wrapper_t<bool> disable_on_fullscreen{"dpms/disable_on_fullscreen"};

    std::optional<wf::idle_inhibitor_t> fullscreen_inhibitor;
    bool has_fullscreen = false;

    wf::activator_callback toggle = [=] (auto)
    {
        if (!output->can_activate_plugin(grab_interface))
        {
            return false;
        }

        /* Toggle DPMS for all outputs **/
        get_instance().toggle_state();

        return true;
    };

    wf::signal_connection_t fullscreen_state_changed{[this] (wf::signal_data_t *data)
        {
            has_fullscreen = data != nullptr;
            update_fullscreen();
        }
    };

    wf::config::option_base_t::updated_callback_t disable_on_fullscreen_changed =
        [=] ()
    {
        update_fullscreen();
    };

    wf::signal_connection_t on_dpms_command = [=] (wf::signal_data_t *data)
    {
        if (!output->can_activate_plugin(grab_interface))
        {
            return;
        }

        command_signal *signal = dynamic_cast<command_signal*>(data);
        Json::Value argv = signal->argv;
        //
        // argv[0] = "output"
        // argv[1] = eg. "*" or 1 or "eDP-1"
        // argv[2] = "dpms"
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
            get_instance().set_state_off(name_or_id);
        } else
        {
            get_instance().set_state_on(name_or_id);
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

    void init() override
    {
        grab_interface->name = "dpms";
        grab_interface->capabilities = 0;

        singleton_plugin_t::init();

        output->add_activator(
            wf::option_wrapper_t<wf::activatorbinding_t>{"dpms/toggle"},
            &toggle);
        output->connect_signal("fullscreen-layer-focused",
            &fullscreen_state_changed);
        disable_on_fullscreen.set_callback(disable_on_fullscreen_changed);

        auto fs_views = output->workspace->get_promoted_views(
            output->workspace->get_current_workspace());

        /* Currently, the fullscreen count would always be 0 or 1, since
         * fullscreen-layer-focused is only emitted on changes between 0 and 1
         **/
        has_fullscreen = fs_views.size() > 0;
        update_fullscreen();

        output->connect_signal("dpms-command", &on_dpms_command);
    }

    void fini() override
    {
        output->rem_binding(&toggle);
        output->disconnect_signal(&fullscreen_state_changed);
        output->disconnect_signal(&on_dpms_command);
        singleton_plugin_t::fini();
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_dpms_singleton);
