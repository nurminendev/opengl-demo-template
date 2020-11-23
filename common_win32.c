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
** common_win32.c
**
** Common routines for Windows
*/

#include "extgl.h"
#include "common.h"

void Sys_Printf(char *, ...);
void Sys_Error(char *, ...);
void Sys_Warn(char *, ...);
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
static void IN_ActivateMouse(void);
static void IN_DeactivateMouse(void);
static void IN_StartupMouse(void);
static void IN_MouseEvent(int);
static void IN_ClearStates(void);
static int IN_MapKey(int);
static void IN_Activate(boolean_t);
LONG WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);
void ParseWin32CommandLine(LPSTR lpCmdLine);
static void GLw_AppActivate(boolean_t, boolean_t);
static boolean_t GLw_CreateWindow(void);
static boolean_t GLw_SetupPixelFormat(void);
static boolean_t GLw_CreateContext(void);

extern char demoname[STRINGLEN];

/*
=======================================================

                  Win32 sys routines

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
		MessageBox(NULL, text, "Error", 0);
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
} 

/*
==========================
Sys_Init()
==========================
*/
static DWORD timebase;

void Sys_Init(void)
{
	timebase = timeGetTime();
}

/*
==========================
Sys_GetMilliseconds()

Returns the number of milliseconds elapsed since app startup
==========================
*/
unsigned long int Sys_GetMilliseconds(void)
{
	return (timeGetTime() - timebase);
}

/*
=======================================================

                   Win32 WINDOWING

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

extern HINSTANCE global_hInstance;
boolean_t activeapp;
boolean_t minimized;

static HWND hWnd;          // window handle
static HDC hDC;            // device context
static HGLRC hGLRC;        // rendering context

/*
==========================
GLw_Init()
==========================
*/
void GLw_Init(void)
{
	Sys_Printf("Initializing OpenGL display...\n");
	GLw_SetMode();
}

/*
==========================
GLw_SetMode()
==========================
*/
void GLw_SetMode(void)
{
	DEVMODE dm;
	int bitsperpixel;
	HDC deskhdc;

	screenwidth = 640;
	screenheight = 480;
	fullscreen = false;

	// Close existing window if there is one
	if(hWnd)
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

	// Do a ChangeDisplaySettings if fullscreen
	if(fullscreen)
	{
		Sys_Printf(".. attempting fullscreen\n");
		memset(&dm, 0, sizeof(dm));
		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth  = screenwidth;
		dm.dmPelsHeight = screenheight;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		deskhdc = GetDC(NULL);
		bitsperpixel = GetDeviceCaps(deskhdc, BITSPIXEL);

		Sys_Printf(".. using desktop display depth of %d\n", bitsperpixel);

		ReleaseDC(0, deskhdc);

		Sys_Printf(".. calling ChangeDisplaySettings\n");
		if(ChangeDisplaySettings(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{
			Sys_Printf("ok\n");

			if(!GLw_CreateWindow())
				Sys_Error("failed to create a window\n");
		}
		else
		{
			Sys_Printf("failed\n");

			Sys_Printf(".. calling ChangeDisplaySettings assuming dual monitors: \n");

			dm.dmPelsWidth = screenwidth * 2;
			dm.dmPelsHeight = screenheight;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system 
			*/
			if(ChangeDisplaySettings(&dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL)
			{
				Sys_Printf("failed\n");
				Sys_Printf(".. setting windowed mode\n");

				ChangeDisplaySettings(0, 0);

				fullscreen = false;
				if(!GLw_CreateWindow())
					Sys_Error("failed to create a window\n");
			}
			else
			{
				Sys_Printf("ok\n");
				if(!GLw_CreateWindow())
					Sys_Error("failed to create a window\n");
			}
		}
	}
	else
	{
		Sys_Printf(".. setting windowed mode\n");

		ChangeDisplaySettings(0, 0);

		if(!GLw_CreateWindow())
			Sys_Error("failed to create a window\n");
	}

	/*
	** (re)init input
	** must be done here because resetting video mode
	** calls IN_Shutdown
	*/
	IN_Init();
}

#define WINDOW_STYLE  (WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE)

/*
==========================
GLw_CreateWindow()
==========================
*/
boolean_t GLw_CreateWindow(void)
{
	WNDCLASS wc;
	RECT r;
	int stylebits;
	int x, y, w, h;
	int exstyle;

	/* Register the frame class */
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = global_hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (void *) COLOR_GRAYTEXT;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = demoname;

    if(!RegisterClass(&wc))
	{
		Sys_Printf("GLw_CreateWindow failed: couldn't register window class\n");
		return false;
	}

	if(fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right = screenwidth;
	r.bottom = screenheight;

	AdjustWindowRect(&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	if(fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		x = screenxpos;
		y = screenypos;
	}

	hWnd = CreateWindowEx(
						  exstyle,
						  demoname,
						  demoname,
						  stylebits,
						  x, y, w, h,
						  NULL,
						  NULL,
						  global_hInstance,
						  NULL);

	if(!hWnd)
	{
		Sys_Printf("GLw_CreateWindow failed: couldn't create window\n");
		return false;
	}
	
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// Setup pixel format
	if(!GLw_SetupPixelFormat())
		return false;

	// Create rendering context
	if(!GLw_CreateContext())
		return false;

	SetForegroundWindow(hWnd);
	SetFocus(hWnd);

	return true;
}

/*
==========================
GLw_SetupPixelFormat()
==========================
*/
boolean_t GLw_SetupPixelFormat(void)
{
    int pixelformat;
    PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		32,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		32,								// 32-bit z-buffer	
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
    };

	/*
	** Get a DC for the specified window
	*/
	if(hDC != NULL)
		Sys_Printf("GLw_SetupPixelFormat() - non-NULL DC exists\n");

	if((hDC = GetDC(hWnd))== NULL)
	{
		Sys_Printf("GLw_SetupPixelFormat: GetDC() failed\n");
		return false;
	}
	if((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		Sys_Printf("GLw_SetupPixelFormat: ChoosePixelFormat() failed\n");
		return false;
	}
	if(SetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
	{
		Sys_Printf("GLw_SetupPixelFormat: SetPixelFormat() failed\n");
		return false;
	}

	return true;
}

/*
==========================
GLw_CreateContext()
==========================
*/
static boolean_t GLw_CreateContext(void)
{
	/*
	** startup the OpenGL subsystem by creating a context and making
	** it current
	*/
	if((hGLRC = wglCreateContext(hDC)) == 0)
	{
		Sys_Printf("GLw_CreateContext: wglCreateContext() failed\n");
		return false;
	}

    if(!wglMakeCurrent(hDC, hGLRC))
	{
		Sys_Printf("GLw_CreateContext: wglMakeCurrent() failed\n");
		return false;
	}

	return true;
}

/*
==========================
GLw_Shutdown()
==========================
*/
void GLw_Shutdown(void)
{
	IN_Shutdown();

	if(!wglMakeCurrent(NULL, NULL))
	{
		// We can't call Sys_Error here, so we just log
		Sys_Log("GLw_Shutdown: wglMakeCurrent(NULL, NULL) failed\n", LOG_ERROR);
	}
	if(hGLRC)
	{
		if(!wglDeleteContext(hGLRC))
		{
			// We can't call Sys_Error here, so we just log
			Sys_Log("GLw_Shutdown: wglDeleteContext failed\n", LOG_ERROR);
		}
		hGLRC = NULL;
	}
	if(hDC)
	{
		if(!ReleaseDC(hWnd, hDC))
		{
			// We can't call Sys_Error here, so we just log
			Sys_Log("GLw_Shutdown: ReleaseDC failed\n", LOG_ERROR);
		}
		hDC   = NULL;
	}
	if(hWnd)
	{
		DestroyWindow(hWnd);
		hWnd = NULL;
	}

	UnregisterClass(demoname, global_hInstance);

	if(fullscreen)
	{
		ChangeDisplaySettings(0, 0);
		fullscreen = false;
	}
}

/*
==========================
GLw_SwapBuffers()
==========================
*/
void GLw_SwapBuffers(void)
{
	SwapBuffers(hDC);
}

/*
==========================
GLw_AppActivate()
==========================
*/
void GLw_AppActivate(boolean_t active, boolean_t appminimize)
{
	minimized = appminimize;

	// we don't want to act like we're active if we're minimized
	if(active && !minimized)
		activeapp = true;
	else
		activeapp = false;

	// minimize/restore mouse-capture on demand
	if(!activeapp)
		IN_Activate(false);
	else
		IN_Activate(true);

	if(active)
	{
		SetForegroundWindow(hWnd);
		ShowWindow(hWnd, SW_RESTORE);
	}
	else
	{
		if(fullscreen)
			ShowWindow(hWnd, SW_MINIMIZE);
	}
}

/*
=======================================================

                   Windows INPUT

=======================================================
*/
boolean_t mouse_avail;
int mouse_x, mouse_y;
int old_mouse_x, old_mouse_y;
int mx_accum, my_accum;
static int win_x, win_y;

static boolean_t mouse_active;
static boolean_t in_appactive;

cvar_t *m_filter;
cvar_t *in_mouse;
cvar_t *sensitivity;
cvar_t *m_yaw;
cvar_t *m_pitch;

static int        mouse_buttons;
static int        mouse_oldbuttonstate;
static POINT      current_pos;
static int        old_x, old_y;
static boolean_t  restore_spi;
static boolean_t  mouseinitialized;
static int        originalmouseparms[3], newmouseparms[3] = {0, 0, 1};
static boolean_t  mouseparmsvalid;
static int        window_center_x, window_center_y;
static RECT       window_rect;

/*
==========================
IN_Init()
==========================
*/
void IN_Init(void)
{
	mouse_avail = true;
	mouse_x = mouse_y = 0;
	old_mouse_x = old_mouse_y = 0;
	m_filter = Cvar_Get("m_filter", 0);
	in_mouse = Cvar_Get("in_mouse", 0);
	sensitivity = Cvar_Get("sensitivity", 0);
	m_yaw = Cvar_Get("m_yaw", 0);
	m_pitch = Cvar_Get("m_pitch", 0);
	IN_StartupMouse();
	/*
	** Put DirectInput initializion here
	*/
}

/*
==========================
IN_Shutdown()
==========================
*/
void IN_Shutdown(void)
{
	IN_DeactivateMouse();
	/*
	** Put DirectInput shutdown here
	*/
}

/*
==========================
IN_ActivateMouse()

Called when the window gains focus or changes in some way
==========================
*/
static void IN_ActivateMouse(void)
{
	int width, height;

	if(!mouseinitialized)
		return;
	if(!in_mouse->value)
	{
		mouse_active = false;
		return;
	}
	if(mouse_active)
		return;

	mouse_active = true;

	if(mouseparmsvalid)
		restore_spi = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(hWnd, &window_rect);
	if(window_rect.left < 0)
		window_rect.left = 0;
	if(window_rect.top < 0)
		window_rect.top = 0;
	if(window_rect.right >= width)
		window_rect.right = width - 1;
	if(window_rect.bottom >= height - 1)
		window_rect.bottom = height - 1;

	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	SetCursorPos(window_center_x, window_center_y);

	old_x = window_center_x;
	old_y = window_center_y;

	SetCapture(hWnd);
	ClipCursor(&window_rect);
	while(ShowCursor(FALSE) >= 0)
		;
}

/*
==========================
IN_DeactivateMouse()

Called when the window loses focus
==========================
*/
static void IN_DeactivateMouse(void)
{
	if(!mouseinitialized)
		return;
	if(!mouse_active)
		return;

	if(restore_spi)
		SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);

	mouse_active = false;

	ClipCursor(NULL);
	ReleaseCapture();
	while(ShowCursor(TRUE) < 0)
		;
}

/*
==========================
IN_StartupMouse()
==========================
*/
static void IN_StartupMouse(void)
{
	if(!in_mouse || !in_mouse->value)
		return;

	mouseinitialized = true;
	mouseparmsvalid = SystemParametersInfo(SPI_GETMOUSE, 0, originalmouseparms, 0);
	mouse_buttons = 3;
}

/*
==========================
IN_MouseEvent()
==========================
*/
static void IN_MouseEvent(int mstate)
{
	int i;

	if(!mouseinitialized)
		return;

	// perform button actions
	for(i = 0; i < mouse_buttons ;i++)
	{
		if((mstate & (1 << i)) &&
		   !(mouse_oldbuttonstate & (1 << i)))
		{
			DM_MouseButtonInput(K_MOUSE1 + i, true, old_mouse_x, old_mouse_y);
		}

		if(!(mstate & (1 << i)) &&
		   (mouse_oldbuttonstate & (1 << i)))
		{
			DM_MouseButtonInput(K_MOUSE1 + i, false, old_mouse_x, old_mouse_y);
		}
	}

	mouse_oldbuttonstate = mstate;
}

/*
==========================
IN_MouseMove()
==========================
*/
void IN_MouseMove(void)
{
	int mx, my;

	if(in_mouse && in_mouse->value)
	{
		if(!mouse_avail || !mouse_active)
			return;

		// find mouse movement
		if(!GetCursorPos (&current_pos))
			return;

		mx = current_pos.x - window_center_x;
		my = current_pos.y - window_center_y;

#if 0
		if(!mx && !my)
			return;
#endif

		if(m_filter && m_filter->value)
		{
			mouse_x = (mx + old_mouse_x) * 0.5;
			mouse_y = (my + old_mouse_y) * 0.5;
		}
		else
		{
			mouse_x = mx;
			mouse_y = my;
		}

		old_mouse_x = mx;
		old_mouse_y = my;

		mouse_x *= sensitivity->value;
		mouse_y *= sensitivity->value;

		common.cam_viewanglesdelta[YAW] -= m_yaw->value * mouse_x;
		common.cam_viewanglesdelta[PITCH] += m_pitch->value * mouse_y;

		// force the mouse to the center, so there's room to move
		if(mx || my)
			SetCursorPos(window_center_x, window_center_y);
	}
}

/*
==========================
IN_ClearStates()
==========================
*/
static void IN_ClearStates(void)
{
	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}

/*
==========================
IN_Frame()
==========================
*/
void IN_Frame(void)
{
	if(!mouseinitialized)
		return;

	if(!in_mouse || !in_mouse->value || !in_appactive)
	{
		IN_DeactivateMouse();
		return;
	}

	IN_ActivateMouse();
}

/*
==========================
IN_MapKey()

Map from windows keys to our keys
==========================
*/
static unsigned char scantokey[128] = {
	0,           27,     '1',        '2',         '3',    '4',         '5',         '6',
	'7',         '8',    '9',        '0',         '-',    '=',         K_BACKSPACE, 9,      // 0
	'q',         'w',    'e',        'r',         't',    'y',         'u',         'i',
	'o',         'p',    '[',        ']',         13,     K_CTRL,      'a',         's',    // 1
	'd',         'f',    'g',        'h',         'j',    'k',         'l',         ';',
	'\'',        '`',    K_SHIFT,    '\\',        'z',    'x',         'c',         'v',    // 2
	'b',         'n',    'm',        ',',         '.',    '/',          K_SHIFT,    '*',
	K_ALT,       ' ',    0,          K_F1,        K_F2,   K_F3,         K_F4,       K_F5,   // 3
	K_F6,        K_F7,   K_F8,       K_F9,        K_F10,  K_PAUSE,      0,          K_HOME,
	K_UPARROW,   K_PGUP, K_KP_MINUS, K_LEFTARROW, K_KP_5, K_RIGHTARROW, K_KP_PLUS,  K_END,  // 4
	K_DOWNARROW, K_PGDN, K_INS,      K_DEL,       0,      0,            0,          K_F11,
	K_F12,       0,      0,          0,           0,      0,            0,          0,      // 5
	0,           0,      0,          0,           0,      0,            0,          0,
	0,           0,      0,          0,           0,      0,            0,          0,      // 6
	0,           0,      0,          0,           0,      0,            0,          0,
	0,           0,      0,          0,           0,      0,            0,          0       // 7
};

static int IN_MapKey(int key)
{
	int result;
	int modified = (key >> 16) & 255;
	boolean_t is_extended = false;

	if(modified > 127)
		return 0;

	if(key & (1 << 24))
		is_extended = true;

	result = scantokey[modified];

	if(!is_extended)
	{
		switch(result)
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else
	{
		switch(result)
		{
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

/*
==========================
IN_Activate()

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
==========================
*/
static void IN_Activate(boolean_t active)
{
	in_appactive = active;
	mouse_active = !active;
}

/*
==========================
IN_HandleEvents()

Handle windows events
==========================
*/
void IN_HandleEvents(void)
{
	MSG msg;

	/*
	** Put DirectInput handling here
	*/

	PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE);
	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

/*
==========================
MainWndProc()
==========================
*/
LONG WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int	active, isminimized;
	boolean_t isactive;
	int xPos, yPos;
	RECT r;
	int style;
	int temp;

	switch(uMsg)
	{
	case WM_ACTIVATE:
		active = LOWORD(wParam);
		isminimized = (BOOL) HIWORD(wParam);
		isactive = (active != WA_INACTIVE) ? true : false;
		GLw_AppActivate(isactive, isminimized);
		break;
	case WM_MOVE:
		if(!scr_fullscreen || !scr_fullscreen->value)
		{
			xPos = (short) LOWORD(lParam);    // horizontal position 
			yPos = (short) HIWORD(lParam);    // vertical position 

			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;

			style = GetWindowLong(hWnd, GWL_STYLE);
			AdjustWindowRect(&r, style, FALSE);

			Cvar_SetValue("scr_xpos", xPos + r.left);
			Cvar_SetValue("scr_ypos", yPos + r.top);
			if(activeapp)
				IN_Activate(true);
		}
	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		temp = 0;
		if(wParam & MK_LBUTTON)
			temp |= 1;
		if(wParam & MK_RBUTTON)
			temp |= 2;
		if(wParam & MK_MBUTTON)
			temp |= 4;
		IN_MouseEvent(temp);
		break;
	case WM_SYSCOMMAND:
		if(wParam == SC_SCREENSAVE)
			return 0;
		break;
	case WM_SYSKEYDOWN:
		if(wParam == 13)
		{
			if(scr_fullscreen && scr_fullscreen->value)
			{
				Cvar_SetValue("scr_fullscreen", 0.0f);
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		DM_KeyInput(IN_MapKey(lParam), true);
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		DM_KeyInput(IN_MapKey(lParam), false);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
==========================
ParseWin32CommandLine()
==========================
*/
extern int argc;
extern char *argv[MAX_NUM_ARGVS];

void ParseWin32CommandLine(LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while(*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while(*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if(*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while(*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if(*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
		}
	}
}
