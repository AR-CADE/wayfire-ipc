
server = shared_module(
    'ipc-server', 
    'ipc.cpp', 
    'server/ipc-server.cpp', 
    'json/ipc-json.cpp', 
    'commands/core.cpp', 
    'commands/command_impl.cpp',
    'commands/criteria.cpp', 
    'commands/output.cpp', 
    'commands/kill.cpp', 
    'commands/exit.cpp', 
    'commands/sticky.cpp', 
    'commands/opacity.cpp', 
    'commands/fullscreen.cpp', 
    'commands/output/power.cpp', 
    'commands/output/scale.cpp', 
    'commands/output/transform.cpp', 
    'commands/output/position.cpp', 
    'commands/output/mode.cpp', 
    dependencies: [wayfire, jsoncpp, pcre2],
    install: true, install_dir: wayfire.get_variable(pkgconfig: 'plugindir'), include_directories : incdir)

subdir('client')
