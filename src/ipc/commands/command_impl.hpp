#ifndef _WAYFIRE_IPC_COMMAND_IMPL_H
#define _WAYFIRE_IPC_COMMAND_IMPL_H

#include <ipc/command.h>
#include <ipc/json.hpp>
#include <ipc/tools.hpp>
#include <json/value.h>

class ipc_command : public command
{
  public:
    static Json::Value execute_command(const std::string& _exec);
    static bool parse_boolean(const char *boolean, bool current);
    static wayfire_view find_container(int con_id);
    static wayfire_view find_xwayland_container(int id);
    static wayfire_view container_get_view(const Json::Value & container);
    static void close_view_container(const Json::Value & container);
    static bool container_has_ancestor(wayfire_view descendant,
        wayfire_view ancestor);
    static void close_view_container_child(const Json::Value & container);
    static Json::Value checkarg(int argc, const char *name, enum expected_args type,
        int val);
};

#endif
