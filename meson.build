project(
	'wayfire-ipc',
    'c',
	'cpp',
	version: '0.1',
	license: 'MIT',
	meson_version: '>=0.61.1',
	default_options: [
		'cpp_std=c++17',
        'c_std=c11',
		'warning_level=2',
		'werror=false',
	],
)

share_dir = join_paths(get_option('prefix'), 'share')
icon_dir = join_paths(share_dir, 'wayfire', 'icons')

c_comp = meson.get_compiler('c')

wayfire        	= dependency('wayfire', version: ['>=0.8.0', '<0.9.0'])
wlroots        	= dependency('wlroots', version: ['>=0.15.0', '<0.17.0'])
wfconfig       	= dependency('wf-config', version: ['>=0.8.0', '<0.9.0'])
jsoncpp        	= dependency('jsoncpp')
threads		   	= dependency('threads')
libinput 	   	= dependency('libinput', version: '>=1.6.0')
wlclient 		= dependency('wayland-client')
wlserver 		= dependency('wayland-server')
wlcursor 		= dependency('wayland-cursor')
wlprotos 		= dependency('wayland-protocols')
xkbcommon      	= dependency('xkbcommon')
#json           	= dependency('nlohmann_json')
pcre           	= dependency('libpcre')

version = '"@0@"'.format(wayfire.version())
add_project_arguments('-DWAYFIRE_VERSION=@0@'.format(version), language: ['cpp', 'c'])
add_project_arguments('-DWF_MSG_VERSION=@0@'.format(meson.project_version()), language: ['cpp', 'c'])

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])
add_project_arguments(['-DWAYFIRE_PLUGIN'], language: ['cpp', 'c'])
add_project_link_arguments(['-rdynamic'], language:'cpp')
subdir('src')
subdir('metadata')

summary = [
	'',
	'----------------',
	'wayfire @0@'.format(version),
	'----------------',
	'',
	'----------------',
	'wayfire-ipc @0@'.format(meson.project_version()),
	'----------------',
	''
]
message('\n'.join(summary))
