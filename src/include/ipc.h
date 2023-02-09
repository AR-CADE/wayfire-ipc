#ifndef _WAYFIRE_IPC_H
#define _WAYFIRE_IPC_H

#include <cstdint>
#include <wlroots_support.h>
#include <sys/types.h>

constexpr char ipc_magic[] = {'i', '3', '-', 'i', 'p', 'c'};

constexpr int IPC_HEADER_SIZE = (sizeof(ipc_magic) + (sizeof(uint32_t) * 2));

#define IPC_ANY_TOCKEN "*"
#define IPC_OFF_TOCKEN "off"
#define IPC_ON_TOCKEN "on"
#define IPC_YES_TOCKEN "yes"
#define IPC_TOGGLE_TOCKEN "toggle"
#define IPC_NO_TOCKEN "no"
#define IPC_TRUE_TOCKEN "true"
#define IPC_FALSE_TOCKEN "false"
#define IPC_ONE_TOCKEN "1"
#define IPC_ZERO_TOCKEN "0"
#define IPC_EMPTY_TOCKEN ""
#define IPC_JSON_EMPTY_TOCKEN "{}"
#define IPC_JSON_EMPTY_OBJECT_TOCKEN IPC_JSON_EMPTY_TOCKEN
#define IPC_JSON_EMPTY_ARRAY_TOCKEN "[]"

///
/// Enumeration of RETURN_STATUS.
///
enum RETURN_STATUS {
	RETURN_SUCCESS  = 0,
	RETURN_LOAD_ERROR,
	RETURN_INVALID_PARAMETER,
    RETURN_UNSUPPORTED,
    RETURN_BAD_BUFFER_SIZE,
    RETURN_BUFFER_TOO_SMALL,
    RETURN_NOT_READY,
    RETURN_DEVICE_ERROR,
    RETURN_WRITE_PROTECTED,
    RETURN_OUT_OF_RESOURCES,
    RETURN_NO_MEDIA,
    RETURN_MEDIA_CHANGED,
    RETURN_NOT_FOUND,
    RETURN_ACCESS_DENIED,
    RETURN_NO_RESPONSE,
    RETURN_NO_MAPPING,
    RETURN_TIMEOUT,
    RETURN_NOT_STARTED,
    RETURN_ALREADY_STARTED,
    RETURN_ABORTED,
    RETURN_PROTOCOL_ERROR,
    RETURN_INCOMPATIBLE_VERSION,
    RETURN_BUFFER_DEFER,
    RETURN_BUFFER_BLOCK,
    RETURN_BUFFER_BLOCK_COMMANDS,
    RETURN_BUFFER_BLOCK_END,
};

/*************** Events ****************/
#define IPC_WF_EVENT_WORKSPACE_CHANGED "workspace-changed"
#define IPC_WF_EVENT_OUTPUT_ADDED "output-added"
#define IPC_WF_EVENT_OUTPUT_REMOVED "output-removed"
#define IPC_WF_EVENT_OUTPUT_LAYOUT_CONFIGURATION_CHANGED "configuration-changed"
#define IPC_WF_EVENT_VIEW_MAPPED "view-mapped"
#define IPC_WF_EVENT_VIEW_ABOVE_CHANGED "wm-actions-above-changed"
#define IPC_WF_EVENT_VIEW_TITLE_CHANGED "title-changed"
#define IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED "view-geometry-changed"
#define IPC_WF_EVENT_VIEW_UNMAPPED "view-unmapped"
#define IPC_WF_EVENT_VIEW_FOCUSED "view-focused"
#define IPC_WF_EVENT_VIEW_HINTS_CHANGED "view-hints-changed"
#define IPC_WF_EVENT_VIEW_MINIMIZED "view-minimized"
#define IPC_WF_EVENT_VIEW_FULLSCREENED "view-fullscreen"
#define IPC_WF_EVENT_VIEW_CHANGE_WORKSPACE "view-change-workspace"
#define IPC_WF_EVENT_WORKSPACE_GRID_CHANGED "workspace-grid-changed"
#define IPC_WF_EVENT_INPUT_DEVICE_ADDED "input-device-added"
#define IPC_WF_EVENT_INPUT_DEVICE_REMOVED "input-device-removed"

#define IPC_I3_EVENT_WORKSPACE "workspace"
#define IPC_I3_EVENT_SHUTDOWN "shutdown"
#define IPC_I3_EVENT_WINDOW "window"
#define IPC_I3_EVENT_BINDING "binding"
#define IPC_I3_EVENT_TICK "tick"

#define IPC_SWAY_EVENT_INPUT "input"
#define IPC_I3_EVENT_OUTPUT "output"

#define event_mask(ev) (1 << (ev & 0x8F))

enum ipc_payload_type
{
    // i3 command types - see i3's I3_REPLY_TYPE constants
    IPC_COMMAND             = 0,
    IPC_GET_WORKSPACES      = 1,
    IPC_SUBSCRIBE           = 2,
    IPC_GET_OUTPUTS         = 3,
	IPC_GET_TREE            = 4,
	IPC_GET_MARKS           = 5,
	IPC_GET_BAR_CONFIG      = 6,
    IPC_GET_VERSION         = 7,
    IPC_GET_BINDING_MODES   = 8,
    IPC_GET_CONFIG          = 9,
    IPC_SEND_TICK           = 10,
	IPC_SYNC                = 11,
	IPC_GET_BINDING_STATE   = 12,
    
    // sway-specific command types
    IPC_GET_INPUTS          = 100,
    IPC_GET_SEATS           = 101,

    //
    IPC_UNSUBSCRIBE         = 200,
};

enum ipc_event_type
{
    // Events sent from sway to clients. Events have the highest bits set.
	IPC_I3_EVENT_TYPE_WORKSPACE                    = ((1u << 31) | 0),
	IPC_I3_EVENT_TYPE_OUTPUT                       = ((1u << 31) | 1),
	IPC_I3_EVENT_TYPE_MODE                         = ((1u << 31) | 2),
	IPC_I3_EVENT_TYPE_WINDOW                       = ((1u << 31) | 3),
	IPC_I3_EVENT_TYPE_BINDING                      = ((1u << 31) | 5),
	IPC_I3_EVENT_TYPE_SHUTDOWN                     = ((1u << 31) | 6),
	IPC_I3_EVENT_TYPE_TICK                         = ((1u << 31) | 7),

	// sway-specific event types
	IPC_SWAY_EVENT_TYPE_INPUT                      = ((1u << 31) | 21),

    //
    // wayfire specific event types
    //
    IPC_WF_EVENT_TYPE_UNSUPPORTED              = -1, 
    IPC_WF_EVENT_TYPE_NONE                     = 0, 
};

enum ipc_scale_filter_mode 
{
	SCALE_FILTER_DEFAULT, // the default is currently smart
	SCALE_FILTER_LINEAR,
	SCALE_FILTER_NEAREST,
	SCALE_FILTER_SMART,
};

#endif
