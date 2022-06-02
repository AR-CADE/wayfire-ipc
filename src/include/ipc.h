#ifndef _WAYFIRE_IPC_H
#define _WAYFIRE_IPC_H

#include <wlroots_support.h>
#include <sys/types.h>

static const char ipc_magic[] = {'i', '3', '-', 'i', 'p', 'c'};

#define IPC_HEADER_SIZE (sizeof(ipc_magic) + (sizeof(uint32_t) * 2))

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

//
// Status codes common to all execution phases
//
typedef int RETURN_STATUS;

///
/// A value of native width with the highest bit set.
///
#define MAX_BIT std::numeric_limits<RETURN_STATUS>::max()

/**
 *  Produces a RETURN_STATUS code with the highest bit set.
 *
 *  @param  StatusCode    The status code value to convert into a warning code.
 *
 *  @return The value specified by StatusCode with the highest bit set.
 *
 **/
#define ENCODE_ERROR(StatusCode) ((RETURN_STATUS)(MAX_BIT | (StatusCode)))

///
/// Enumeration of IPC_STATUS.
///@{
#define RETURN_SUCCESS 0
#define RETURN_LOAD_ERROR ENCODE_ERROR(1)
#define RETURN_INVALID_PARAMETER ENCODE_ERROR(2)
#define RETURN_UNSUPPORTED ENCODE_ERROR(3)
#define RETURN_BAD_BUFFER_SIZE ENCODE_ERROR(4)
#define RETURN_BUFFER_TOO_SMALL ENCODE_ERROR(5)
#define RETURN_NOT_READY ENCODE_ERROR(6)
#define RETURN_DEVICE_ERROR ENCODE_ERROR(7)
#define RETURN_WRITE_PROTECTED ENCODE_ERROR(8)
#define RETURN_OUT_OF_RESOURCES ENCODE_ERROR(9)
#define RETURN_NO_MEDIA ENCODE_ERROR(12)
#define RETURN_MEDIA_CHANGED ENCODE_ERROR(13)
#define RETURN_NOT_FOUND ENCODE_ERROR(14)
#define RETURN_ACCESS_DENIED ENCODE_ERROR(15)
#define RETURN_NO_RESPONSE ENCODE_ERROR(16)
#define RETURN_NO_MAPPING ENCODE_ERROR(17)
#define RETURN_TIMEOUT ENCODE_ERROR(18)
#define RETURN_NOT_STARTED ENCODE_ERROR(19)
#define RETURN_ALREADY_STARTED ENCODE_ERROR(20)
#define RETURN_ABORTED ENCODE_ERROR(21)
#define RETURN_PROTOCOL_ERROR ENCODE_ERROR(24)
#define RETURN_INCOMPATIBLE_VERSION ENCODE_ERROR(25)
///@}

///
/// Define macro to encode the status code.
///
#define RETURN_ERROR(A) (((RETURN_STATUS)(A)) < 0)

#define IPC_COMMAND_VIEW "view"
#define IPC_METHOD_VIEW_AT "at"
#define IPC_METHOD_VIEW_RESIZE "resize"
#define IPC_METHOD_VIEW_PING "ping"
#define IPC_METHOD_VIEW_ACCEPTS_INPUT "accepts_input"
#define IPC_METHOD_VIEW_MOVE "move"
#define IPC_METHOD_VIEW_SET_OUTPUT "set_output"
#define IPC_METHOD_VIEW_GET "get"
#define IPC_METHOD_VIEW_REQUEST_NATIVE_SIZE "request_native_size"
#define IPC_METHOD_VIEW_ENUMERATE_VIEWS "enumerate_views"
#define IPC_METHOD_VIEW_CLOSE "close"
#define IPC_METHOD_VIEW_TILE_REQUEST "tile_request"

#define IPC_METHOD_OUTPUT_DPMS "dpms"
#define IPC_METHOD_OUTPUT_SCALE "scale"

#define IPC_COMMAND_INPUT "input"
#define IPC_COMMAND_OUTPUT "output"

#define IPC_COMMAND_CHANGE_VIEW_ABOVE "change_view_above"
#define IPC_COMMAND_GET_VIEW_AT "get_view_at"
/*************** View Actions ****************/
#define IPC_COMMAND_UPDATE_VIEW_MINIMIZE_HINT "update_view_minimize_hint"
#define IPC_COMMAND_SHADE_VIEW "shade_view"
#define IPC_COMMAND_BRING_VIEW_TO_FRONT "bring_view_to_front"
#define IPC_COMMAND_RESTACK_VIEW_ABOVE "restack_view_above"
#define IPC_COMMAND_restack_view_below "restack_view_below"
#define IPC_COMMAND_MINIMIZE_VIEW "minimize_view"
#define IPC_COMMAND_MAXIMIZE_VIEW "maximize_view"
#define IPC_COMMAND_FOCUS_VIEW "focus_view"
#define IPC_COMMAND_FULLSCREEN_VIEW "fullscreen_view"
#define IPC_COMMAND_CLOSE_VIEW "close_view"
#define IPC_COMMAND_CHANGE_VIEW_MINIMIZE_HINT "change_view_minimize_hint"
#define IPC_COMMAND_CHANGE_OUTPUT_VIEW "change_output_view"
#define IPC_COMMAND_CHANGE_WORKSPACE_VIEW "change_workspace_view"
#define IPC_COMMAND_CHANGE_WORKSPACE_OUTPUT "change_workspace_output"
#define IPC_COMMAND_CHANGE_WORKSPACE_ALL_OUTPUTS "change_workspace_all_outputs"
#define IPC_COMMAND_SHOW_DESKTOP "show_desktop"
#define IPC_COMMAND_SCALE "scale"
/*************** Non-reffing actions at end ****************/
#define IPC_COMMAND_ENABLE_PROPERTY_MODE "enable_property_mode"
#define IPC_COMMAND_QUERY_CURSOR_POSITION "query_cursor_position"
#define IPC_COMMAND_QUERY_OUTPUT_IDS "query_output_ids"
#define IPC_COMMAND_QUERY_ACTIVE_OUTPUT "query_active_output"
#define IPC_COMMAND_QUERY_VIEW_VECTOR_IDS "query_view_vector_ids"
#define IPC_COMMAND_QUERY_VIEW_VECTOR_TASKMAN_IDS                              \
    "query_view_vector_taskman_ids"
/*************** Output Properties ****************/
#define IPC_COMMAND_QUERY_OUTPUT_NAME "query_output_name"
#define IPC_COMMAND_QUERY_OUTPUT_MANUFACTURER "query_output_manufacturer"
#define IPC_COMMAND_QUERY_OUTPUT_MODEL "query_output_model"
#define IPC_COMMAND_QUERY_OUTPUT_SERIAL "query_output_serial"
#define IPC_COMMAND_QUERY_OUTPUT_WORKSPACE "query_output_workspace"
#define IPC_COMMAND_QUERY_WORKSPACE_GRID_SIZE "query_workspace_grid_size"
/*************** View Properties ****************/
#define IPC_COMMAND_QUERY_VIEW_ABOVE_VIEW "query_view_above_view"
#define IPC_COMMAND_QUERY_VIEW_BELOW_VIEW "query_view_below_view"
#define IPC_COMMAND_QUERY_VIEW_APP_ID "query_view_app_id"
#define IPC_COMMAND_QUERY_VIEW_APP_ID_GTK_SHELL "query_view_app_id_gtk_shell"
#define IPC_COMMAND_QUERY_VIEW_APP_ID_XWAYLAND_NET_WM_NAME                     \
    "query_view_app_id_xwayland_net_wm_name"
#define IPC_COMMAND_QUERY_VIEW_TITLE "query_view_title"
#define IPC_COMMAND_QUERY_VIEW_ATTENTION "query_view_attention"
#define IPC_COMMAND_QUERY_XWAYLAND_DISPLAY "query_xwayland_display"
#define IPC_COMMAND_QUERY_VIEW_XWAYLAND_WID "query_view_xwayland_wid"
#define IPC_COMMAND_QUERY_VIEW_XWAYLAND_ATOM_CARDINAL                          \
    "query_view_xwayland_atom_cardinal"
#define IPC_COMMAND_QUERY_VIEW_XWAYLAND_ATOM_STRING                            \
    "query_view_xwayland_atom_string"
#define IPC_COMMAND_QUERY_VIEW_CREDENTIALS "query_view_credentials"
#define IPC_COMMAND_QUERY_VIEW_ABOVE "query_view_above"
#define IPC_COMMAND_QUERY_VIEW_MAXIMIZED "query_view_maximized"
#define IPC_COMMAND_QUERY_VIEW_ACTIVE "query_view_active"
#define IPC_COMMAND_QUERY_VIEW_MINIMIZED "query_view_minimized"
#define IPC_COMMAND_QUERY_VIEW_FULLSCREEN "query_view_fullscreen"
#define IPC_COMMAND_QUERY_VIEW_OUTPUT "query_view_output"
#define IPC_COMMAND_QUERY_VIEW_WORKSPACES "query_view_workspaces"
#define IPC_COMMAND_QUERY_VIEW_GROUP_LEADER "query_view_group_leader"
#define IPC_COMMAND_QUERY_VIEW_ROLE "query_view_role"
#define IPC_COMMAND_QUERY_VIEW_TEST_DATA "query_view_test_data"


/*************** Events ****************/
#define IPC_WF_EVENT_WORKSPACE_CHANGED "workspace-changed"
#define IPC_WF_EVENT_OUTPUT_ADDED "output-added"
#define IPC_WF_EVENT_OUTPUT_REMOVED "output-removed"
#define IPC_WF_EVENT_OUTPUT_CONFIGURATION_CHANGED "output-configuration-changed"
#define IPC_WF_EVENT_VIEW_MAPPED "view-mapped"
#define IPC_WF_EVENT_VIEW_ABOVE_CHANGED "wm-actions-above-changed"
#define IPC_WF_EVENT_VIEW_APP_ID_CHANGED "app-id-changed"
#define IPC_WF_EVENT_VIEW_TITLE_CHANGED "title-changed"
#define IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED "view-geometry-changed"
#define IPC_WF_EVENT_VIEW_UNMAPPED "view-unmapped"
#define IPC_WF_EVENT_VIEW_TILED "view-tiled"
#define IPC_WF_EVENT_VIEW_LAYER_ATTACHED "view-layer-attached"
#define IPC_WF_EVENT_VIEW_LAYER_DETACHED "view-layer-detached"
#define IPC_WF_EVENT_VIEW_FOCUSED "view-focused"
#define IPC_WF_EVENT_VIEW_HINTS_CHANGED "view-hints-changed"
#define IPC_WF_EVENT_VIEW_PRE_MOVED_TO_OUTPUT "view-pre-moved-to-output"
#define IPC_WF_EVENT_VIEW_MOVED_TO_OUTPUT "view-moved-to-output"
#define IPC_WF_EVENT_POINTER_BUTTON "pointer_button"
#define IPC_WF_EVENT_TABLET_BUTTON "tablet_button"

#define IPC_WF_EVENT_VIEW_MINIMIZED "view-minimized"
#define IPC_WF_EVENT_VIEW_FULLSCREENED "view-fullscreen"
#define IPC_WF_EVENT_VIEW_CHANGE_WORKSPACE "view-change-workspace"
#define IPC_WF_EVENT_VIEW_ATTACHED "view-attached"
#define IPC_WF_EVENT_VIEW_DETACHED "view-detached"
#define IPC_WF_EVENT_VIEW_SET_STICKY "view-set-sticky"

#define IPC_I3_EVENT_WORKSPACE "workspace"
#define IPC_I3_EVENT_SHUTDOWN "shutdown"
#define IPC_I3_EVENT_WINDOW "window"
#define IPC_I3_EVENT_BINDING "binding"
#define IPC_I3_EVENT_TICK "tick"

#define IPC_SWAY_EVENT_INPUT "input"
#define IPC_SWAY_EVENT_OUTPUT "output"

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
    IPC_INTERNAL_COMMAND    = 201,

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
    IPC_WF_EVENT_TYPE_WORKSPACE_CHANGED            = ((1u << 31) | 30),

    IPC_WF_EVENT_TYPE_OUTPUT_ADDED                 = ((1u << 31) | 31),
    IPC_WF_EVENT_TYPE_OUTPUT_REMOVED               = ((1u << 31) | 32),
    IPC_WF_EVENT_TYPE_OUTPUT_CONFIGURATION_CHANGED = ((1u << 31) | 33),

    IPC_WF_EVENT_TYPE_VIEW_PRE_MOVED_TO_OUTPUT = ((1u << 31) | 35),
    IPC_WF_EVENT_TYPE_VIEW_MINIMIZED           = ((1u << 31) | 37),
    IPC_WF_EVENT_TYPE_VIEW_FULLSCREENED        = ((1u << 31) | 38),
    IPC_WF_EVENT_TYPE_VIEW_CHANGE_WORKSPACE    = ((1u << 31) | 39),
    IPC_WF_EVENT_TYPE_VIEW_ATTACHED            = ((1u << 31) | 40),
    IPC_WF_EVENT_TYPE_VIEW_ABOVE_CHANGED       = ((1u << 31) | 41),
    IPC_WF_EVENT_TYPE_VIEW_HINTS_CHANGED       = ((1u << 31) | 42),
    IPC_WF_EVENT_TYPE_VIEW_MOVED_TO_OUTPUT     = ((1u << 31) | 43),
    IPC_WF_EVENT_TYPE_VIEW_MAPPED              = ((1u << 31) | 44),
    IPC_WF_EVENT_TYPE_VIEW_LAYER_ATTACHED      = ((1u << 31) | 45),
    IPC_WF_EVENT_TYPE_VIEW_LAYER_DETACHED      = ((1u << 31) | 46),
    IPC_WF_EVENT_TYPE_VIEW_APP_ID_CHANGED      = ((1u << 31) | 47),
    IPC_WF_EVENT_TYPE_VIEW_TITLE_CHANGED       = ((1u << 31) | 48),
    IPC_WF_EVENT_TYPE_VIEW_GEOMETRY_CHANGED    = ((1u << 31) | 49),
    IPC_WF_EVENT_TYPE_VIEW_TILED               = ((1u << 31) | 50),
    IPC_WF_EVENT_TYPE_VIEW_FOCUSED             = ((1u << 31) | 51),
    IPC_WF_EVENT_TYPE_VIEW_CHANGE_VIEWPORT     = ((1u << 31) | 52),
    IPC_WF_EVENT_TYPE_VIEW_UNMAPPED            = ((1u << 31) | 53),

    IPC_WF_EVENT_TYPE_POINTER_BUTTON           = ((1u << 31) | 54),
    IPC_WF_EVENT_TYPE_TABLET_BUTTON            = ((1u << 31) | 55),

    IPC_WF_EVENT_TYPE_VIEW_DETACHED            = ((1u << 31) | 56),
    IPC_WF_EVENT_TYPE_VIEW_SET_STICKY          = ((1u << 31) | 57),

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
