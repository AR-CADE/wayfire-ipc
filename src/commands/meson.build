dpms = shared_module(
    'dpms', 
    'dpms.cpp', 
    dependencies: [wayfire, wlroots, jsoncpp],
    install: true, install_dir: wayfire.get_variable(pkgconfig: 'plugindir'), include_directories : incdir)

#autokill = shared_module(
#    'auto-kill', 
#    'auto-kill.cpp', 
#    dependencies: [wayfire, wlroots, jsoncpp],
#    install: false, install_dir: wayfire.get_variable(pkgconfig: 'plugindir'), include_directories : incdir)
