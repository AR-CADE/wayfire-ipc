#ifndef _XWAYLAND_SUPPORT_H
#define _XWAYLAND_SUPPORT_H

#include <wlroots_support.h>

#ifndef WF_HAS_XWAYLAND
#define WF_HAS_XWAYLAND 1
#elif WF_HAS_XWAYLAND != 1
#define WF_HAS_XWAYLAND 1
#endif

/* 
#ifndef HAVE_XWAYLAND
#define HAVE_XWAYLAND 1
#elif HAVE_XWAYLAND != 1
#define HAVE_XWAYLAND 1
#endif 
*/

extern "C" {
#define class class_t
#define static
//#include <X11/Xatom.h>
#include <wlr/xwayland.h>
// #include <xcb/res.h>
// #include <xcb/xcb.h>
#undef static 
#undef class
// #include <sys/socket.h>
#include <wlr/types/wlr_idle.h> 
};

#endif

