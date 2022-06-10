#ifndef _XWAYLAND_SUPPORT_H
#define _XWAYLAND_SUPPORT_H

#include <wlroots_support.h>
#include <wlr/config.h>
#include <wayfire/config.h>

#if WLR_HAS_XWAYLAND && WF_HAS_XWAYLAND
#define HAVE_XWAYLAND 1
#else
#define HAVE_XWAYLAND 0
#endif

extern "C" {
#define class class_t
#define static
#if HAVE_XWAYLAND
//#include <X11/Xatom.h>
#include <wlr/xwayland.h>
//#include <xcb/res.h>
//#include <xcb/xcb.h>
#endif
#undef static 
#undef class
//#include <sys/socket.h>
#include <wlr/types/wlr_idle.h> 
};

#endif

