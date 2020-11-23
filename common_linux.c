/*

Demo Template

Copyright (C) 2003 Riku "Rakkis" Nurminen <rakkis@rakkis.net>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
** common_linux.c
**
** Common routines for GNU/Linux
*/

#include "extgl.h"
#include "common.h"
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86dga.h>
#include <X11/extensions/xf86vmode.h>
#include <GL/glx.h>

void Sys_Printf(char *, ...);
void Sys_Error(char *, ...);
void Sys_Warn(char *, ...);
void Sys_Log(const char *, int);
void Sys_Init(void);
unsigned long int Sys_GetMilliseconds(void);
void GLw_Init(void);
void GLw_SetMode(void);
void GLw_Shutdown(void);
void GLw_SwapBuffers(void);
void IN_Init(void);
void IN_Shutdown(void);
void IN_HandleEvents(void);
void IN_MouseMove(void);
void IN_Frame(void);
static Cursor IN_CreateNullCursor(Display *, Window);
static void IN_InstallGrabs(void);
static void IN_UninstallGrabs(void);
static void IN_DeactivateMouse(void);
static void IN_ActivateMouse(void);
static int IN_XLateKey(XKeyEvent *);
static void GLw_CreateContext(XVisualInfo *);
static void signal_handler(int);
static void InitSignals(void);

extern char demoname[STRINGLEN];

/*
=======================================================

                GNU/Linux sys routines

=======================================================
*/

extern cvar_t *nolog;
extern cvar_t *nostdout;

/*
==========================
Sys_Printf()
==========================
*/
void Sys_Printf(char *fmt, ...)
{
	va_list argptr;
	char text[BIGSTRINGLEN + 1];

	if(fmt == NULL)
		return;

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	if(strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf\n");

    if(!nolog || !nolog->value)
		Sys_Log(text, LOG_NORMAL);

#if PLATFORM == PLATFORM_LINUX
	if(!nostdout || !nostdout->value)
		fputs(text, stdout);
#endif
}

/*
==========================
Sys_Error()
==========================
*/
void Sys_Error(char *error, ...)
{ 
    va_list argptr;
	char text[BIGSTRINGLEN + 1];

    va_start(argptr, error);
    vsprintf(text, error, argptr);
    va_end(argptr);

    if(!nolog || !nolog->value)
		Sys_Log(text, LOG_ERROR);

	if(!nostdout || !nostdout->value)
	{
#if PLATFORM == PLATFORM_WIN32
		MessageBox(NULL, text, "Error", 0);
#else
		fprintf(stderr, "Error: %s", text);
#endif
	}

	GLw_Shutdown();
	Common_Shutdown();

	_exit(1);
} 

/*
==========================
Sys_Warn()
==========================
*/
void Sys_Warn(char *warning, ...)
{ 
    va_list argptr;
	char text[BIGSTRINGLEN + 1];
    
    va_start(argptr, warning);
    vsprintf(text, warning, argptr);
    va_end(argptr);

    if(!nolog || !nolog->value)
		Sys_Log(text, LOG_WARN);

#if PLATFORM == PLATFORM_LINUX
	fprintf(stderr, "Warning: %s", text);
#endif
} 

static time_t secbase;
static suseconds_t microsecbase;

/*
==========================
Sys_Init()
==========================
*/
void Sys_Init(void)
{
	struct timeval tp;

	gettimeofday(&tp, NULL);
	secbase = tp.tv_sec;
	microsecbase = tp.tv_usec;
}

/*
==========================
Sys_GetMilliseconds()

Returns the number of milliseconds elapsed since app startup
==========================
*/
unsigned long int Sys_GetMilliseconds(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	// convert seconds and microseconds to milliseconds in return
	return ((unsigned long int) (tp.tv_sec - secbase) * 1000 + (tp.tv_usec - microsecbase) * 0.001);
}

/*
=======================================================

                   WINDOWING

=======================================================
*/
static int screennum;
static int screenwidth;
static int screenheight;
static int screenxpos;
static int screenypos;
static boolean_t fullscreen;

static cvar_t *scr_width;
static cvar_t *scr_height;
static cvar_t *scr_xpos;
static cvar_t *scr_ypos;
static cvar_t *scr_fullscreen;

/*
==========================
GLw_Init()
==========================
*/
void GLw_Init(void)
{
	Sys_Printf("Initializing OpenGL display...\n");
	InitSignals();
	GLw_SetMode();
}

static Display *dpy = NULL;
static Window win;
static GLXContext ctx = NULL;

static XF86VidModeModeInfo **vidmodes;
static boolean_t vidmode_active = false;
static int num_vidmodes;
static boolean_t shouldreceiveconfigurenotify;

// What events do we want to receive?
#define KEY_MASK       (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK     (ButtonPressMask | ButtonReleaseMask | \
                       PointerMotionMask | ButtonMotionMask)
#define MYXEVENTMASK   (KEY_MASK | MOUSE_MASK | VisibilityChangeMask | \
                       StructureNotifyMask)

/*
==========================
GLw_SetMode()

Creates/sets/resets the window and rendering context
according to scr_width, scr_height, and scr_fullscreen.

Defaults to 640x480 non-fullscreen window.
==========================
*/
void GLw_SetMode(void)
{
	int glxattrs[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 1,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_ACCUM_RED_SIZE, 1,
		GLX_ACCUM_GREEN_SIZE, 1,
		GLX_ACCUM_BLUE_SIZE, 1,
		GLX_ACCUM_ALPHA_SIZE, 1,
		None
	};
	Window rootwin;
	XVisualInfo *vi;
	XSetWindowAttributes attr;
	int glxmajorversion, glxminorversion;
	int xf86vmmajorversion, xf86vmminorversion;
	int i;
	boolean_t resok;
	unsigned long valuemask;

	screenwidth = 640;
	screenheight = 480;
	screenxpos = 100;
	screenypos = 100;
	fullscreen = false;
	resok = false;

	// Close existing window if there is one
	GLw_Shutdown();

	// Check for user specified screen resolution
	scr_width = Cvar_Get("scr_width", 0);
	scr_height = Cvar_Get("scr_height", 0);
	if((scr_width && scr_width->value) &&
	   (scr_height && scr_height->value))
	{
		screenwidth = (int) scr_width->value;
		screenheight = (int) scr_height->value;
	}
	// Check for user specified screen x/y pos
	scr_xpos = Cvar_Get("scr_xpos", 0);
	scr_ypos = Cvar_Get("scr_ypos", 0);
	if((scr_xpos && scr_xpos->value) &&
	   (scr_ypos && scr_ypos->value))
	{
		screenxpos = (int) scr_xpos->value;
		screenypos = (int) scr_ypos->value;
	}
	scr_fullscreen = Cvar_Get("scr_fullscreen", 0);
	if(scr_fullscreen && scr_fullscreen->value)
		fullscreen = true;

	dpy = XOpenDisplay(0);
	if(!dpy)
		Sys_Error("can't open X display\n");

	screennum = DefaultScreen(dpy);
	rootwin = RootWindow(dpy, screennum);

	glxmajorversion = glxminorversion = 0;
    glXQueryVersion(dpy, &glxmajorversion, &glxminorversion);
	Sys_Printf("GLX version %d.%d\n", glxmajorversion, glxminorversion);

	vi = glXChooseVisual(dpy, screennum, glxattrs);
	if(!vi)
	{
		Sys_Error("can't create an RGBA, DoubleBuffered, depth visual\n");
	}

	xf86vmmajorversion = xf86vmminorversion = 0;
	// Check for VidMode extension
	if(!XF86VidModeQueryVersion(dpy, &xf86vmmajorversion, &xf86vmminorversion))
	{
		Sys_Warn("couldn't query XFree86 VidMode extension version.. fullscreen disabled\n");
		fullscreen = false;
	}
	else
	{
		Sys_Printf("Using XFree86 VidMode extension version %d.%d\n",
				   xf86vmmajorversion, xf86vmminorversion);
	}

	if(fullscreen)
	{
		Sys_Printf(".. attempting fullscreen\n");
		// get supported video modes
		XF86VidModeGetAllModeLines(dpy, screennum, &num_vidmodes, &vidmodes);
		// see if desired resolution matches any of the supported
		// video modes
		for(i = 0; i < num_vidmodes; i++)
		{
			if((screenwidth == vidmodes[i]->hdisplay) &&
			   (screenheight == vidmodes[i]->vdisplay))
			{
				resok = true;
				break;
			}
		}
		if(resok)
		{
			// change to the mode.. vidmodes[i] == our mode
			XF86VidModeSwitchToMode(dpy, screennum, vidmodes[i]);
			vidmode_active = true;
			Sys_Printf("ok\n");

			// Move the viewport to top left
			XF86VidModeSetViewPort(dpy, screennum, 0, 0);
		}
		else
		{
			Sys_Warn("couldn't set fullscreen: no modeline for requested resolution %dx%d found on current display\n",
					 screenwidth, screenheight);
			fullscreen = false;
			Sys_Printf(".. setting windowed mode\n");
		}
	}
	else
	{
		Sys_Printf(".. setting windowed mode\n");
	}

	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap(dpy, rootwin, vi->visual, AllocNone);
	attr.event_mask = MYXEVENTMASK;
	if(vidmode_active)
	{
		attr.override_redirect = True;
		attr.backing_store = NotUseful;
		attr.save_under = False;
		valuemask = CWBackPixel | CWColormap | CWSaveUnder | CWBackingStore |
			CWEventMask | CWOverrideRedirect;
	}
	else
	{
		valuemask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
	}

	win = XCreateWindow(dpy, rootwin, screenxpos, screenypos, screenwidth, screenheight,
						0, vi->depth, InputOutput, vi->visual, valuemask, &attr);

	XMapWindow(dpy, win);

	// set window title
	XStoreName(dpy, win, demoname);

	if(vidmode_active)
	{
		XMoveWindow(dpy, win, 0, 0);
		XRaiseWindow(dpy, win);
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
		XFlush(dpy);
		// Move the viewport to top left
		XF86VidModeSetViewPort(dpy, screennum, 0, 0);
	}

	XFlush(dpy);

	// Create our GLX context
	GLw_CreateContext(vi);

	// handle events once
	shouldreceiveconfigurenotify = true;
	IN_HandleEvents();

	/*
	** (re)init input
	** must be done here because resetting video mode
	** calls IN_Shutdown
	*/
	IN_Init();
}

/*
==========================
GLw_CreateContext()
==========================
*/
static void GLw_CreateContext(XVisualInfo *vi)
{
	ctx = glXCreateContext(dpy, vi, NULL, True);
	glXMakeCurrent(dpy, win, ctx);
}

/*
==========================
GLw_Shutdown()
==========================
*/
void GLw_Shutdown(void)
{
	IN_Shutdown();

	if(dpy)
	{
		if(ctx)
			glXDestroyContext(dpy, ctx);
		if(win)
			XDestroyWindow(dpy, win);
		if(vidmode_active)
			XF86VidModeSwitchToMode(dpy, screennum, vidmodes[0]);
		XCloseDisplay(dpy);
	}
	dpy = NULL;
	win = 0;
	ctx = NULL;
}

/*
==========================
GLw_SwapBuffers()
==========================
*/
void GLw_SwapBuffers(void)
{
	glXSwapBuffers(dpy, win);
}

/*
==========================
signal_handler()
==========================
*/
static void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	GLw_Shutdown();
	_exit(0);
}

/*
==========================
InitSignals()
==========================
*/
static void InitSignals(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

/*
=======================================================

                   GNU/Linux INPUT

=======================================================
*/

boolean_t mouse_avail;
int mouse_x, mouse_y;
int old_mouse_x, old_mouse_y;

static int win_x, win_y;

static boolean_t mouse_active;
static boolean_t dgamouse;

cvar_t *m_filter;
cvar_t *in_mouse;
static cvar_t *in_dgamouse;
cvar_t *sensitivity;
cvar_t *m_yaw;
cvar_t *m_pitch;

/*
==========================
IN_Init()
==========================
*/
void IN_Init(void)
{
	mouse_x = mouse_y = 0;
	old_mouse_x = old_mouse_y = 0;
	mouse_avail = true;
	m_filter = Cvar_Get("m_filter", 0);
	m_yaw = Cvar_Get("m_yaw", 0);
	m_pitch = Cvar_Get("m_pitch", 0);
	sensitivity = Cvar_Get("sensitivity", 0);
	in_mouse = Cvar_Get("in_mouse", 0);
	in_dgamouse = Cvar_Get("in_dgamouse", 0);
	IN_InstallGrabs();
}

/*
==========================
IN_Shutdown()
==========================
*/
void IN_Shutdown(void)
{
	mouse_avail = false;
	IN_UninstallGrabs();
}

/*
==========================
IN_CreateNullCursor()
==========================
*/
static Cursor IN_CreateNullCursor(Display *display, Window root)
{
    Pixmap cursormask; 
    XGCValues xgc;
    GC gc;
    XColor dummycolour;
    Cursor cursor;

    cursormask = XCreatePixmap(display, root, 1, 1, 1);
    xgc.function = GXclear;
    gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
    XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
    dummycolour.pixel = 0;
    dummycolour.red = 0;
    dummycolour.flags = 04;
    cursor = XCreatePixmapCursor(display, cursormask, cursormask,
								 &dummycolour, &dummycolour, 0, 0);
    XFreePixmap(display,cursormask);
    XFreeGC(display,gc);
    return cursor;
}

/*
==========================
IN_InstallGrabs()
==========================
*/
static void IN_InstallGrabs(void)
{
	int majorversion, minorversion;

	XDefineCursor(dpy, win, IN_CreateNullCursor(dpy, win));

	XGrabPointer(dpy, win,
				 True,
				 0,
				 GrabModeAsync, GrabModeAsync,
				 win,
				 None,
				 CurrentTime);

	in_dgamouse = Cvar_Get("in_dgamouse", 0);

	if(in_dgamouse && in_dgamouse->value)
	{
		if(!XF86DGAQueryVersion(dpy, &majorversion, &minorversion))
		{ 
			Sys_Warn("failed to query XFree86-DGA extension.. DGA mouse disabled\n");
			Cvar_Set("in_dgamouse", "0");
		}
		else
		{
			dgamouse = true;
			XF86DGADirectVideo(dpy, DefaultScreen(dpy), XF86DGADirectMouse);
			XWarpPointer(dpy, None, win, 0, 0, 0, 0, 0, 0);
			Sys_Printf("Using XFree86-DGA extension version %d.%d\n",
					   majorversion, minorversion);
		}
	}
	else
	{
		XWarpPointer(dpy, None, win,
					 0, 0, 0, 0,
					 screenwidth / 2, screenheight / 2);
	}

	XGrabKeyboard(dpy, win,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);

	mouse_active = true;
}

/*
==========================
IN_UninstallGrabs()
==========================
*/
static void IN_UninstallGrabs(void)
{
	if(!dpy || !win)
		return;

	if(dgamouse)
	{
		dgamouse = false;
		XF86DGADirectVideo(dpy, DefaultScreen(dpy), 0);
	}

	XUngrabPointer(dpy, CurrentTime);
	XUngrabKeyboard(dpy, CurrentTime);

	XUndefineCursor(dpy, win);

	mouse_active = false;
}

/*
==========================
IN_DeactivateMouse()
==========================
*/
static void IN_DeactivateMouse(void)
{
	if(!mouse_avail || !dpy || !win)
		return;

	if(mouse_active)
	{
		IN_UninstallGrabs();
		mouse_active = false;
	}
}

/*
==========================
IN_ActivateMouse()
==========================
*/
static void IN_ActivateMouse(void)
{
	if(!mouse_avail || !dpy || !win)
		return;

	if(!mouse_active)
	{		
		mouse_x = mouse_y = 0; // don't spazz
		IN_InstallGrabs();
		mouse_active = true;
	}
}

/*
==========================
IN_MouseMove()
==========================
*/
void IN_MouseMove(void)
{
	if(in_mouse && in_mouse->value)
	{
		if(!mouse_avail || !mouse_active)
			return;

		if(m_filter && m_filter->value)
		{
			mouse_x = (mouse_x + old_mouse_x) * 0.5;
			mouse_y = (mouse_y + old_mouse_y) * 0.5;
		}

		old_mouse_x = mouse_x;
		old_mouse_y = mouse_y;

		mouse_x *= sensitivity->value;
		mouse_y *= sensitivity->value;

		common.cam_viewanglesdelta[YAW] -= m_yaw->value * mouse_x;
		common.cam_viewanglesdelta[PITCH] += m_pitch->value * mouse_y;
	}
	mouse_x = mouse_y = 0;
}

/*
==========================
IN_Frame()
==========================
*/
void IN_Frame(void)
{
	if(!in_mouse || !in_mouse->value)
	{
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();
}

/*
==========================
IN_XLateKey()

Translate XKeyEvent into our keycode
==========================
*/
static int IN_XLateKey(XKeyEvent *ev)
{
	int key;
	char buf[64];
	KeySym keysym;

	key = 0;

	XLookupString(ev, buf, sizeof(buf), &keysym, 0);
	
	switch(keysym)
	{
	case XK_KP_Page_Up:       key = K_KP_PGUP; break;
	case XK_Page_Up:          key = K_PGUP; break;
	case XK_KP_Page_Down:     key = K_KP_PGDN; break;
	case XK_Page_Down:        key = K_PGDN; break;
	case XK_KP_Home:          key = K_KP_HOME; break;
	case XK_Home:             key = K_HOME; break;
	case XK_KP_End:           key = K_KP_END; break;
	case XK_End:              key = K_END; break;
	case XK_KP_Left:          key = K_KP_LEFTARROW; break;
	case XK_Left:             key = K_LEFTARROW; break;
	case XK_KP_Right:         key = K_KP_RIGHTARROW; break;
	case XK_Right:            key = K_RIGHTARROW; break;
	case XK_KP_Down:          key = K_KP_DOWNARROW; break;
	case XK_Down:             key = K_DOWNARROW; break;
	case XK_KP_Up:            key = K_KP_UPARROW; break;
	case XK_Up:               key = K_UPARROW; break;
	case XK_Escape:           key = K_ESCAPE; break;
	case XK_KP_Enter:         key = K_KP_ENTER; break;
	case XK_Return:           key = K_ENTER; break;
	case XK_Tab:              key = K_TAB; break;
	case XK_F1:               key = K_F1; break;
	case XK_F2:               key = K_F2; break;
	case XK_F3:               key = K_F3; break;
	case XK_F4:               key = K_F4; break;
	case XK_F5:               key = K_F5; break;
	case XK_F6:               key = K_F6; break;
	case XK_F7:               key = K_F7; break;
	case XK_F8:               key = K_F8; break;
	case XK_F9:               key = K_F9; break;
	case XK_F10:              key = K_F10; break;
	case XK_F11:              key = K_F11; break;
	case XK_F12:              key = K_F12; break;
	case XK_BackSpace:        key = K_BACKSPACE; break;
	case XK_KP_Delete:        key = K_KP_DEL; break;
	case XK_Delete:           key = K_DEL; break;
	case XK_Pause:            key = K_PAUSE; break;
	case XK_Shift_L:
	case XK_Shift_R:          key = K_SHIFT; break;
	case XK_Execute:
	case XK_Control_L:
	case XK_Control_R:        key = K_CTRL; break;
	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Alt_R:
	case XK_Meta_R:           key = K_ALT; break;
	case XK_KP_Begin:         key = K_KP_5; break;
	case XK_Insert:           key = K_INS; break;
	case XK_KP_Insert:        key = K_KP_INS; break;
	case XK_KP_Multiply:      key = '*'; break;
	case XK_KP_Add:           key = K_KP_PLUS; break;
	case XK_KP_Subtract:      key = K_KP_MINUS; break;
	case XK_KP_Divide:        key = K_KP_SLASH; break;
	default:
		key = *(unsigned char *) buf;
		if(key >= 'A' && key <= 'Z')
			key = key - 'A' + 'a';
		break;
	}

	return key;
}

/*
==========================
IN_HandleEvents()

Handle X events
==========================
*/
void IN_HandleEvents(void)
{
	XEvent event;
	int b;
	boolean_t dowarp = false;
	int mwx = screenwidth / 2;
	int mwy = screenheight / 2;

	if(!dpy)
		return;

	while(XPending(dpy))
	{
		XNextEvent(dpy, &event);

		switch(event.type)
		{
		case KeyPress:
		case KeyRelease:
			DM_KeyInput(IN_XLateKey(&event.xkey), event.type == KeyPress);
			break;
		case MotionNotify:
			if(mouse_active)
			{
				if(dgamouse)
				{
					mouse_x += (event.xmotion.x + win_x) * 2;
					mouse_y += (event.xmotion.y + win_y) * 2;
				}
				else 
				{
					mouse_x += (event.xmotion.x - mwx) * 2;
					mouse_y += (event.xmotion.y - mwy) * 2;
					mwx = event.xmotion.x;
					mwy = event.xmotion.y;
					if(mouse_x || mouse_y)
						dowarp = true;
				}
				DM_MouseMotionInput(mouse_x, mouse_y);
			}
			break;
		case ButtonPress:
			b = -1;
			if(event.xbutton.button == 1)
				b = 0;
			else if(event.xbutton.button == 2)
				b = 2;
			else if(event.xbutton.button == 3)
				b = 1;
			if(b >= 0)
				DM_MouseButtonInput(K_MOUSE1 + b, true, old_mouse_x, old_mouse_y);
			break;
		case ButtonRelease:
			b = -1;
			if(event.xbutton.button == 1)
				b = 0;
			else if(event.xbutton.button == 2)
				b = 2;
			else if(event.xbutton.button == 3)
				b = 1;
			if(b >= 0)
				DM_MouseButtonInput(K_MOUSE1 + b, false, old_mouse_x, old_mouse_y);
			break;
		case CreateNotify:
			win_x = event.xcreatewindow.x;
			win_y = event.xcreatewindow.y;
			break;
		case ConfigureNotify:
			if(shouldreceiveconfigurenotify)
			{
				win_x = event.xconfigure.x;
				win_y = event.xconfigure.y;
				shouldreceiveconfigurenotify = false;
			}
			break;
		}
	}
	if(dowarp)
		XWarpPointer(dpy, None, win, 0, 0, 0, 0, screenwidth / 2, screenheight / 2);
}
