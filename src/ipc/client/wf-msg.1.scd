wf-msg(1)

# NAME

wf-msg - Send messages to a running instance of wayfire over the IPC socket.

# SYNOPSIS

_wf-msg_ [options...] [message]

# OPTIONS

*-h, --help*
	Show help message and quit.

*-m, --monitor*
	Monitor for responses until killed instead of exiting after the first
	response. This can only be used with the IPC message type _subscribe_. If
	there is a malformed response or an invalid event type was requested,
	wf-msg will stop monitoring and exit.

*-p, --pretty*
	Use pretty output even when not using a tty.

*-q, --quiet*
	Sends the IPC message but does not print the response from wayfire.

*-r, --raw*
	Use raw output even if using a tty.

*-s, --socket* <path>
	Use the specified socket path. Otherwise, wf-msg will ask wayfire where the
	socket is (which is the value of $WAYFIRESOCK).

*-t, --type* <type>
	Specify the type of IPC message. See below.

*-v, --version*
	Print the version (of wf-msg) and quit.

# IPC MESSAGE TYPES

*<command>*
	The message is a wayfire command (the same commands you can bind to keybindings
	in your wayfire config file). It will be executed immediately.

	See *wayfire*(5) for a list of commands.

	Tips:
	- Command expansion is performed twice: once by wf-msg, and again by wayfire.
	  If you have quoted multi-word strings in your command, enclose the entire
	  command in single-quotes. For example, use
	  _wf-msg 'output "Foobar Display" enable'_ instead of
	  _wf-msg output "Foobar Display" enable_. Furthermore, note that comma
	  separated options also count as multi-word strings, because commas can be
	  used to execute commands on the same line.
	- If you are providing a command that contains a leading hyphen (_-_), insert
	  two hyphens (_--_) before the command to signal to wf-msg not to parse
	  anything beyond that point as an option. For example, use
	  _wf-msg -- mark --add test_ instead of _wf-msg mark --add test_.

*get\_workspaces*
	Gets a JSON-encoded list of workspaces and their status.

*get\_inputs*
	Gets a JSON-encoded list of current inputs.

*get\_outputs*
	Gets a JSON-encoded list of current outputs.

*get\_tree*
	Gets a JSON-encoded layout tree of all open windows, containers, outputs,
	workspaces, and so on.

*get\_seats*
	Gets a JSON-encoded list of all seats,
	its properties and all assigned devices.

*get\_version*
	Get JSON-encoded version information for the running instance of wayfire.

*get\_config*
	Gets a JSON-encoded copy of the current configuration.

*send\_tick*
	Sends a tick event to all subscribed clients.

*subscribe*
	Subscribe to a list of event types. The argument for this type should be
	provided in the form of a valid JSON array. If any of the types are invalid
	or if a valid JSON array is not provided, this will result in a failure.

# RETURN CODES

*0*
	Success

*1*
	wf-msg errors such as invalid syntax, unable to connect to the ipc socket
	or unable to parse wayfire's reply

*2*
	wayfire-ipc returned an error when processing the command (ex. invalid command,
	command failed, and invalid subscription request)

# SEE ALSO

*wayfire-ipc*(7)
