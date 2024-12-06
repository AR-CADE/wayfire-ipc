#include <ipc/command.h>

Json::Value output_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value kill_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value fullscreen_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value exit_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value sticky_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value opacity_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value workspace_handler(int argc, char **argv, command_handler_context *ctx);
Json::Value scratchpad_handler(int argc, char **argv, command_handler_context *ctx);

std::map<std::string, std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)>, std::less<>> const core_handler_map{
    {"output", output_handler},
    {"kill", kill_handler},
    {"fullscreen", fullscreen_handler},
    {"exit", exit_handler},
    {"sticky", sticky_handler},
    {"opacity", opacity_handler},
    {"workspace", workspace_handler},
    {"scratchpad", scratchpad_handler}, };


std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> core_handler_map_try_at(const std::string& k)
{
    try {
        return core_handler_map.at(k);
    } catch (const std::out_of_range& ex)
    {
        (void)ex;
        return nullptr;
    }
}

std::function<Json::Value(int argc, char**argv,
    command_handler_context*ctx)> core_handler(int argc, const char **argv)
{
    if (!(argc > 0))
    {
        return nullptr;
    }

    return core_handler_map_try_at(argv[0]);
}
