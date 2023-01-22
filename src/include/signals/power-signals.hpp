#pragma once

#include <json/value.h>
#include <wayfire/signal-definitions.hpp>

struct power_command_signal
{
    Json::Value argv = Json::arrayValue;
};
