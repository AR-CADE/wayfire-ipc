#ifndef _WAYFIRE_IPC_COMMAND_H
#define _WAYFIRE_IPC_COMMAND_H

#include <json/value.h>
#include <ipc.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>

enum expected_args
{
    EXPECTED_AT_LEAST,
    EXPECTED_AT_MOST,
    EXPECTED_EQUAL_TO,
};

// enum config_dpms
// {
// DPMS_IGNORE,
// DPMS_ON,
// DPMS_OFF,
// };

/**
 * Size and position configuration for a particular output.
 *
 * This is set via the `output` command.
 */
struct output_config
{
    char *name = nullptr;
    // int enabled;
    // int width, height;
    // float refresh_rate;
    // int custom_mode;
    // drmModeModeInfo drm_mode;
    // int x, y;
    // float scale;
    // enum scale_filter_mode scale_filter;
    // int32_t transform;
    // enum wl_output_subpixel subpixel;
    // int max_render_time; // In milliseconds
    // int adaptive_sync;
    // enum render_bit_depth render_bit_depth;

    // char *background;
    // char *background_option;
    // char *background_fallback;
    // enum config_dpms dpms_state;
};

struct leftovers_t
{
    int argc    = 0;
    char **argv = nullptr;
};

// Context for command handlers
class command_handler_context
{
  public:
    // struct input_config *input_config;
    struct output_config *output_config = nullptr;
    // struct seat_config *seat_config;
    wlr_seat *seat   = nullptr;
    Json::Value node = Json::nullValue;
    Json::Value container = Json::nullValue;
    Json::Value workspace = Json::nullValue;
    bool node_overridden  = false; // True if the node is selected by means other
                                   // than focus
    struct leftovers_t leftovers;
};

std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> core_handler(int argc, const char **argv);

class command {

public:
     static wf::output_t *all_output_by_name(const char *name)
    {
        if (!name)
        {
            return nullptr;
        }
        
        auto outputs = wf::get_core().output_layout->get_outputs();
        for (wf::output_t *output : outputs)
        {
            if (std::string(name) == std::string(output->handle->name))
            {
                return output;
            }
            if (output != nullptr)
            {
                if (std::string(name) == std::string(output->handle->name))
                {
                    return output;
                }
            }
        } 
        return nullptr;
    }

    static wf::output_t *all_output_by_id(uint32_t id)
    {
        auto outputs = wf::get_core().output_layout->get_outputs();
        for (wf::output_t *output : outputs)
        {
            if (output != nullptr)
            {
                if (output->get_id() == id)
                {
                    return output;
                }
            }
        }

        return nullptr;
    }

    static wf::output_t *all_output_by_name_or_id(const char *name_or_id)
    {
        if (!name_or_id)
        {
            return nullptr;
        }
        
        auto output = all_output_by_name(name_or_id);

        if (output == nullptr) 
        {
            try 
            { 
                output = all_output_by_id(std::stoi(std::string(name_or_id)));
            } catch (...) 
            {
            
            }
        }

        return output;
    }


};


#endif