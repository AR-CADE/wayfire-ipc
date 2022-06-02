#ifndef _WAYFIRE_IPC_COMMAND_H
#define _WAYFIRE_IPC_COMMAND_H

#include <json/value.h>
#include <ipc.h>
#include <string>

class command {

public:
    static Json::Value _get_ids_at_index(Json::Value argv, Json::ArrayIndex index) 
    {
        Json::Value value = argv.get(Json::ArrayIndex(index), IPC_ANY_TOCKEN);
        if (value.isNumeric() == false && (value.isBool() || value.empty() || (value.isString() && value.asString() == IPC_ANY_TOCKEN)))
        {
            return IPC_ANY_TOCKEN;
        }

        if (!value.isArray())
        {
            Json::Value array = Json::arrayValue;
            return array.append(value);
        }

        return value;
    }

    static std::string getCmdBase(Json::Value argv) 
    {
        if (!argv.isArray()) 
        {
            return "";
        }
        Json::Value arg = argv.get(Json::ArrayIndex(0), IPC_EMPTY_TOCKEN);
        if (!arg.isString()) 
        {
            return "";
        }
        return arg.asString();
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