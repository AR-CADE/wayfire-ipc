#ifndef _WAYFIRE_IPC_COMMAND_IMPL_H
#define _WAYFIRE_IPC_COMMAND_IMPL_H

#include <ipc/command.h>
#include <ipc/json.hpp>
#include <ipc/tools.hpp>
#include <json/value.h>
#include <vector>
#include "criteria.hpp"

enum expected_args
{
    EXPECTED_AT_LEAST,
    EXPECTED_AT_MOST,
    EXPECTED_EQUAL_TO,
};

enum config_dpms
{
    DPMS_IGNORE,
    DPMS_ON,
    DPMS_OFF,
};

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
    enum config_dpms dpms_state;
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

Json::Value cmd_core(Json::Value argv);
std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> core_handler(int argc, const char **argv);

class ipc_command : public command
{
  public:
    static Json::Value execute_command(const std::string& _exec);
    static bool parse_boolean(const char *boolean, bool current);
    static wayfire_view find_container(int con_id);
    static wayfire_view find_xwayland_container(int id);
    static wayfire_view container_get_view(Json::Value container);
    static void close_view_container(Json::Value container);
    static bool container_has_ancestor(wayfire_view descendant,
        wayfire_view ancestor);
    static void close_view_container_child(Json::Value container);
    static Json::Value checkarg(int argc, const char *name, enum expected_args type,
        int val);
};

#endif
