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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>

#define PRECISION_SINGLE 1
#define PRECISION_DOUBLE 2
//===================================
// floating-point precision
//===================================
#define PRECISION PRECISION_SINGLE

#define PLATFORM_WIN32 1
#define PLATFORM_LINUX 2

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
  #define PLATFORM PLATFORM_WIN32
#else
  #define PLATFORM PLATFORM_LINUX
#endif

// Windows Settings
#if PLATFORM == PLATFORM_WIN32
  #include <windows.h>
  #pragma comment(lib, "opengl32.lib")
  #pragma comment(lib, "glu32.lib")
  #pragma comment(lib, "winmm.lib")		// timeGetTime()
  #pragma warning(disable:4244)  // conversion from double to float
  #pragma warning(disable:4018)  // signed/unsigned mismatch
  // Win32 compilers use _DEBUG for specifying debug builds.
  #ifdef _DEBUG
    #define DEBUG_MODE 1
  #else
    #define DEBUG_MODE 0
  #endif
  #define snprintf    _snprintf
  #define vsnprintf   _vsnprintf
  #define strcasecmp  _stricmp
  #define	MAX_NUM_ARGVS	128
  #define INLINE __inline
#endif

// GNU/Linux settings
#if PLATFORM == PLATFORM_LINUX
  #ifdef DEBUG
    #define DEBUG_MODE 1
  #else
    #define DEBUG_MODE 0
  #endif
  #ifndef __GNUC__
    #define INLINE inline
  #else
    #define INLINE __inline__
  #endif
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// loglevels
#define LOG_NORMAL 0
#define LOG_WARN   1
#define LOG_ERROR  2

#define STRINGLEN      128
#define BIGSTRINGLEN   1024
#define BIGBUFFERLEN   65536

#define IMG_RGB   1
#define IMG_RGBA  2

typedef enum
{
	false,
	true
} boolean_t;

#if PRECISION == PRECISION_SINGLE
typedef float real_t;
#else
typedef double real_t;
#endif

#define K_KEYPRESS      1
#define K_KEYRELEASE    2

#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131
#define	K_ALT			132
#define	K_CTRL			133
#define	K_SHIFT			134
#define	K_F1			135
#define	K_F2			136
#define	K_F3			137
#define	K_F4			138
#define	K_F5			139
#define	K_F6			140
#define	K_F7			141
#define	K_F8			142
#define	K_F9			143
#define	K_F10			144
#define	K_F11			145
#define	K_F12			146
#define	K_INS			147
#define	K_DEL			148
#define	K_PGDN			149
#define	K_PGUP			150
#define	K_HOME			151
#define	K_END			152

#define K_KP_HOME		160
#define K_KP_UPARROW	161
#define K_KP_PGUP		162
#define	K_KP_LEFTARROW	163
#define K_KP_5			164
#define K_KP_RIGHTARROW	165
#define K_KP_END		166
#define K_KP_DOWNARROW	167
#define K_KP_PGDN		168
#define	K_KP_ENTER		169
#define K_KP_INS   		170
#define	K_KP_DEL		171
#define K_KP_SLASH		172
#define K_KP_MINUS		173
#define K_KP_PLUS		174

#define K_PAUSE			255

#define	K_MOUSE1		200
#define	K_MOUSE2		201
#define	K_MOUSE3		202

typedef struct
{
	real_t x;
	real_t y;
} vec2_t;

typedef struct
{
	real_t x;
	real_t y;
	real_t z;
} vec3_t;

typedef struct
{
	real_t x;
	real_t y;
	real_t z;
	real_t w;
} vec4_t;

typedef struct
{
	real_t m11;
	real_t m12;
	real_t m13;
	real_t m21;
	real_t m22;
	real_t m23;
	real_t m31;
	real_t m32;
	real_t m33;
} mat3x3_t;

typedef struct
{
	real_t m11;
	real_t m12;
	real_t m13;
	real_t m14;
	real_t m21;
	real_t m22;
	real_t m23;
	real_t m24;
	real_t m31;
	real_t m32;
	real_t m33;
	real_t m34;
	real_t m41;
	real_t m42;
	real_t m43;
	real_t m44;
} mat4x4_t;

typedef struct
{
	real_t r;
	real_t g;
	real_t b;
} color3_t;

typedef struct
{
	real_t r;
	real_t g;
	real_t b;
	real_t a;
} color4_t;

typedef struct cvar_s
{
	char *name;
	char *string;
	real_t value;
	struct cvar_s *next;
} cvar_t;

// which faces does a material affect
#define FACE_FRONT           0x0001
#define FACE_BACK            0x0002
#define FACE_FRONT_AND_BACK  (FACE_FRONT | FACE_BACK)

typedef struct material_s
{
	char *name;
	boolean_t hastexmap1;
	boolean_t hastexmap2;
	unsigned int texmap1;
	unsigned int texmap2;
	color4_t color;
	color4_t ambient;
	color4_t diffuse;
	color4_t specular;
	color4_t emission;
	real_t shininess;
	unsigned char facebits;
	unsigned int bumpmap;
	boolean_t hasbumpmap;
	struct material_s *next;
} material_t;

typedef struct
{
	unsigned int vertexindex[3];
} face_t;

struct mesh_s;

typedef struct submesh_s
{
	char *name;
	int numvertices;
	int numfaces;
	int numtexcoords;
	material_t *material;
	unsigned int vertexvboid;
	unsigned int normalvboid;
	unsigned int texcoordvboid;
	vec3_t *vertexdata;
	vec3_t *normaldata;
	vec2_t *texcoords;
	face_t *faces;
	boolean_t hasvertexprogram;
	unsigned int vertexprogramid;
	boolean_t hasfragmentprogram;
	unsigned int fragmentprogramid;
	struct mesh_s *parent;
	struct submesh_s *next;
} submesh_t;

typedef struct mesh_s
{
	char *name;
	submesh_t *submeshpool;
	struct mesh_s *next;
} mesh_t;

typedef struct
{
	unsigned int format;
	unsigned int width;
	unsigned int height;
	unsigned char *data;
} image_t;

typedef struct
{
    unsigned short int id;
    unsigned int length;
    unsigned int bytesread;
} chunk_t;

#define PITCH         0
#define YAW           1
#define ROLL          2
#define NUM_ANGLES    3

typedef struct
{
	real_t curfps;
	real_t frameinterval;
	vec3_t up;
	real_t camspeed;
	vec3_t campos;
	vec3_t camlookat;
	int cam_moveforward;
	int cam_sidestep;
	real_t cam_viewanglesdelta[NUM_ANGLES];
} common_t;

extern common_t common;

// demo.c
extern void DM_KeyInput(int, boolean_t);
extern void DM_MouseButtonInput(int, boolean_t, int, int);
extern void DM_MouseMotionInput(int, int);

// common.c
extern void Z_Free(void *);
extern void Z_Stats_f(void);
extern void Z_FreeTags(int);
extern void *Z_Malloc(int);
extern void Common_PreFrame(void);
extern void Common_PostFrame(void);
extern void Common_snprintf(char *, int, char *, ...);
extern void Common_ProcessCommandLine(int, char **);
extern void Common_Init(int, char **, char *, char *, char *, char *, char *, char *);
extern void Common_Shutdown(void);
extern void Common_SecondsToString(unsigned long int, char *, int);
extern char *Common_CopyString(char *);
extern void Common_strtolower(char *);
extern void Sys_Log(const char *, int);
extern char *Sys_GetTimeString(void);
extern INLINE real_t M_Vec3Magnitude(vec3_t *);
extern INLINE void M_Vec3Add(vec3_t *, vec3_t *, vec3_t *);
extern INLINE void M_Vec3Subtract(vec3_t *, vec3_t *, vec3_t *);
extern INLINE void M_Vec3Divide(vec3_t *, real_t, vec3_t *);
extern INLINE real_t M_Vec3Dot(vec3_t *, vec3_t *);
extern INLINE void M_Vec3Cross(vec3_t *, vec3_t *, vec3_t *);
extern INLINE void M_Vec3Normalize(vec3_t *, vec3_t *);
extern INLINE void M_MakeIdentity4x4(mat4x4_t *);
extern real_t Cvar_VariableValue(char *);
extern char *Cvar_VariableString(char *);
extern cvar_t *Cvar_Get(char *, char *);
extern cvar_t *Cvar_Set(char *, char *);
extern void Cvar_SetValue(char *, real_t);
extern void Cvar_ExecAutoexec(void);
extern void Cvar_Init(void);
extern void Cvar_Cleanup(void);
extern boolean_t IMG_LoadTGA(image_t *, char *);
extern boolean_t MDL_Load3DS(mesh_t *, char *);
extern int FS_FileLength(FILE *);
extern void FS_FCloseFile(FILE *);
extern int FS_FOpenFile(char *, FILE **, const char *);

// common_[linux|win32].c
extern void Sys_Printf(char *, ...);
extern void Sys_Error(char *, ...);
extern void Sys_Warn(char *, ...);
extern void Sys_Log(const char *, int);
extern void Sys_Init(void);
extern unsigned long int Sys_GetMilliseconds(void);
extern void GLw_Init(void);
extern void GLw_SetMode(void);
extern void GLw_Shutdown(void);
extern void GLw_SwapBuffers(void);
extern void IN_Init(void);
extern void IN_Shutdown(void);
extern void IN_HandleEvents(void);
extern void IN_MouseMove(void);
extern void IN_Frame(void);

#endif // __COMMON_H__
