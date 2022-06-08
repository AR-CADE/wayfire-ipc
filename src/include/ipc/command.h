#ifndef _WAYFIRE_IPC_COMMAND_H
#define _WAYFIRE_IPC_COMMAND_H

#include <json/value.h>
#include <ipc.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>

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


#include <wayfire/signal-definitions.hpp>

/* A public signal, shared by the ipc server plugin & ipc commands plugin
 *
 */

struct command_signal : public wf::signal_data_t
{
    Json::Value argv = Json::arrayValue;
};

#endif