# wayfire-ipc
An experimental ipc plugin for wayfire, aims to be kind of I3/sway compatible.

## Dependencies

- [Wayfire](https://github.com/WayfireWM/wayfire)
- [JsonCpp](https://github.com/open-source-parsers/jsoncpp)
- [pcre2](https://github.com/PCRE2Project/pcre2)

Build dependencies

- [meson](https://mesonbuild.com/)
- [ninja](https://ninja-build.org/)

## Building & install

```
$ git clone https://github.com/AR-CADE/wayfire-ipc.git 
$ cd wayfire-ipc
$ meson build --prefix=/usr --buildtype=release
$ ninja -C build 
$ sudo ninja -C build install
```

## Usage

Add `ipc-server` to the plugins list in ~/.config/wayfire.ini (alternatively you can also use [wcm](https://github.com/WayfireWM/wcm))

This project provide a command line tool similar to `swaymsg` called `wf-msg`, to use it open a terminal window and type:

```
$ wf-msg -h

Usage: wf-msg [options] [message]

  -h, --help             Show help message and quit.
  -m, --monitor          Monitor until killed (-t SUBSCRIBE only)
  -p, --pretty           Use pretty output even when not using a tty
  -q, --quiet            Be quiet.
  -r, --raw              Use raw output even if using a tty
  -s, --socket <socket>  Use the specified socket.
  -t, --type <type>      Specify the message type.
  -v, --version          Show the version number and quit.
```

You can for example show the configuration of your output(s) by typing:

```
$ wf-msg -t get_outputs
```

# contact
arm-cade@proton.me