
server = shared_module(
    'ipc-server', 
    'ipc.cpp', 
    'server/ipc-server.cpp', 
    'json/ipc-json.cpp', 
    'command/core.cpp', 
    'command/command_impl.cpp',
    'command/criteria.cpp', 
    'command/output.cpp', 
    'command/kill.cpp', 
    'command/exit.cpp', 
    'command/sticky.cpp', 
    'command/fullscreen.cpp', 
    'command/output/dpms.cpp', 
    'command/output/scale.cpp', 
    'command/output/transform.cpp', 
    dependencies: [wayfire, wlroots, jsoncpp, pcre],
    install: true, install_dir: wayfire.get_variable(pkgconfig: 'plugindir'), include_directories : incdir)

subdir('client')
