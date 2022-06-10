#include <json/value.h>
#include <string>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include "core.hpp"

Json::Value output_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value kill_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value fullscreen_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value exit_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value sticky_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value opacity_handler(int argc, char **argv, command_handler_context *ctx);

std::map<std::string, std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)>> core_handler_map
{
    {"output", output_handler},
    {"kill", kill_handler},
    {"fullscreen", fullscreen_handler},
    {"exit", exit_handler},
    {"sticky", sticky_handler},
    {"opacity", opacity_handler},
};

std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> core_handler(int argc, const char **argv)
{
    if (!(argc > 0))
    {
        return nullptr;
    }

    return core_handler_map[argv[0]];
}
