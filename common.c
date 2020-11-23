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
** common.c
**
** Common platform independant routines
*/

#include "extgl.h"
#include "common.h"
#include "common_gl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#if PLATFORM == PLATFORM_WIN32
  #include <memory.h>  // memset()
#else
  #include <strings.h>
  #include <unistd.h>
#endif
#include <math.h>

void Z_Free(void *);
void Z_Stats_f(void);
void Z_FreeTags(int);
static void *Z_TagMalloc(int, int);
void *Z_Malloc(int);
void Common_PreFrame(void);
void Common_PostFrame(void);
void Common_snprintf(char *, int, char *, ...);
void Common_ProcessCommandLine(int, char **);
void Common_Init(int, char **, char *, char *, char *, char *, char *, char *);
void Common_Shutdown(void);
void Common_SecondsToString(unsigned long int, char *, int);
char *Common_CopyString(char *);
void Common_strtolower(char *);
void Sys_Log(const char *, int);
char *Sys_GetTimeString(void);
INLINE real_t M_Vec3Magnitude(vec3_t *);
INLINE void M_Vec3Add(vec3_t *, vec3_t *, vec3_t *);
INLINE void M_Vec3Subtract(vec3_t *, vec3_t *, vec3_t *);
INLINE void M_Vec3Divide(vec3_t *, real_t, vec3_t *);
INLINE real_t M_Vec3Dot(vec3_t *, vec3_t *);
INLINE void M_Vec3Cross(vec3_t *, vec3_t *, vec3_t *);
INLINE void M_Vec3Normalize(vec3_t *, vec3_t *);
INLINE void M_MakeIdentity4x4(mat4x4_t *);
static cvar_t *Cvar_FindVar(char *);
real_t Cvar_VariableValue(char *);
char *Cvar_VariableString(char *);
cvar_t *Cvar_Get(char *, char *);
cvar_t *Cvar_Set(char *, char *);
void Cvar_SetValue(char *, real_t);
void Cvar_ExecAutoexec(void);
void Cvar_Init(void);
void Cvar_Cleanup(void);
boolean_t IMG_LoadTGA(image_t *, char *);
boolean_t MDL_Load3DS(mesh_t *, char *);
static void ProcessNextChunk(mesh_t *, chunk_t *);
static void ProcessNextObjectChunk(mesh_t *, submesh_t *, chunk_t *);
static void ProcessNextMaterialChunk(material_t *, chunk_t *);
static void AddMaterialMapfile(char *, material_t *, chunk_t *);
static void ReadNextChunk(chunk_t *);
static int GetString(char *);
static void ReadColorChunk(material_t *, chunk_t *);
static void ReadVertexIndices(submesh_t *, chunk_t *);
static void ReadUVCoordinates(submesh_t *, chunk_t *);
static void ReadVertices(submesh_t *, chunk_t *);
static void ReadObjectMaterial(mesh_t *, submesh_t *, chunk_t *);
static void ComputeNormals(mesh_t *);
int FS_FileLength(FILE *);
void FS_FCloseFile(FILE *);
int FS_FOpenFile(char *, FILE **, const char *);

extern int errno;

common_t common;
char demoname[STRINGLEN];
cvar_t *nolog;
cvar_t *nostdout;
static char logfile_name[STRINGLEN];
static char appname[STRINGLEN];
static char datadir[STRINGLEN];
static time_t startuptime;

/*
=======================================================
                Zone Memory Allocation

From the (GPL'd) Quake 2 sources
=======================================================
*/
#define	Z_MAGIC		0x2D2D

typedef struct zhead_s
{
	struct zhead_s *prev, *next;
	short magic;
	short tag; // for group free
	int size;
} zhead_t;

static zhead_t z_chain;
static int z_count, z_bytes;

/*
==========================
Z_Free()
==========================
*/
void Z_Free(void *ptr)
{
	zhead_t	*z;

	z = ((zhead_t *) ptr) - 1;

	if(z->magic != Z_MAGIC)
		Sys_Error("Z_Free: bad magic\n");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free(z);
}

/*
==========================
Z_Stats_f()
==========================
*/
void Z_Stats_f(void)
{
	Sys_Printf("Z_Stats_f: %i bytes in %i blocks\n", z_bytes, z_count);
}

/*
==========================
Z_FreeTags()
==========================
*/
void Z_FreeTags(int tag)
{
	zhead_t	*z, *next;

	for(z = z_chain.next; z != &z_chain; z = next)
	{
		next = z->next;
		if(z->tag == tag)
			Z_Free((void *) (z + 1));
	}
}

/*
==========================
Z_TagMalloc()
==========================
*/
static void *Z_TagMalloc(int size, int tag)
{
	zhead_t	*z;
	
	size = size + sizeof(*z);
	z = malloc(size);
	if(!z)
		Sys_Error("Z_TagMalloc: failed on allocation of %i bytes", size);
	memset(z, 0, size);
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *) (z + 1);
}

/*
==========================
Z_Malloc()
==========================
*/
void *Z_Malloc(int size)
{
	return Z_TagMalloc(size, 0);
}

/*
==========================
Common_PreFrame()
==========================
*/
void Common_PreFrame(void)
{
	IN_MouseMove();
	GL_UpdateCamera();
}

/*
==========================
Common_PostFrame()
==========================
*/
void Common_PostFrame(void)
{
	GL_CalcFPS();
}

/*
==========================
Common_snprintf()
==========================
*/
void Common_snprintf(char *dest, int size, char *fmt, ...)
{
	int len;
	va_list argptr;
	char bigbuffer[BIGBUFFERLEN];

	va_start(argptr, fmt);
	len = vsprintf(bigbuffer, fmt, argptr);
	va_end(argptr);
	if(len >= size)
		Sys_Warn("Common_snprintf: overflow of %i in %i\n", len, size);
	strncpy(dest, bigbuffer, size - 1);
}

/*
==========================
Common_ProcessCommandLine()
==========================
*/
void Common_ProcessCommandLine(int argc, char *argv[])
{
	unsigned int i;

	nolog = Cvar_Get("nolog", "0");
	nostdout = Cvar_Get("nostdout", "0");

	Common_snprintf(appname, STRINGLEN, argv[0]);

	for(i = 1; i < argc; i++)
	{
		if(strstr(argv[i], "-nolog"))
			Cvar_Set("nolog", "1");
		else if(strstr(argv[i], "-nostdout"))
			Cvar_Set("nostdout", "1");
		else
			Sys_Printf("Unrecognized command line option: %s\n", argv[i]);
	}
}

/*
==========================
Common_Init()
==========================
*/
static FILE *logfile_fp;

void Common_Init(int argc, char *argv[], char *logfile, char *demoname2,
				 char *demoversion, char *democopy1, char *democopy2,
				 char *datadir2)
{
	common.curfps = 0.0f;
	common.frameinterval = 0.0f;
	Sys_Init();
	common.up.x = 0.0f;
	common.up.y = 1.0f;
	common.up.z = 0.0f;
	common.cam_moveforward = 0;
	common.cam_sidestep = 0;
	common.cam_viewanglesdelta[PITCH] = 0.0f;
	common.cam_viewanglesdelta[YAW] = 0.0f;
	common.cam_viewanglesdelta[ROLL] = 0.0f;
	z_chain.next = z_chain.prev = &z_chain;
	startuptime = time(NULL);
	Common_snprintf(logfile_name, STRINGLEN, logfile);
	Common_snprintf(demoname, STRINGLEN, demoname2);
	Common_snprintf(datadir, STRINGLEN, datadir2);

	Common_ProcessCommandLine(argc, argv);

	if(!nolog || !nolog->value)
	{
		if(FS_FOpenFile(logfile, &logfile_fp, "a+") < 0)
		{
			Sys_Warn("unable to open logfile: %s .. disabling logging to file\n", logfile, strerror(errno));
			Cvar_Set("nolog", "1");
		}
	}
	Sys_Printf("---- demo started at %s\n", Sys_GetTimeString());
	Sys_Printf("%s v%s\n", demoname, demoversion);
	Sys_Printf("%s\n", democopy1);
	Sys_Printf("\n");
	Sys_Printf("%s\n\n", democopy2);
	if(DEBUG_MODE)
		Sys_Printf("** Running in debug mode\n\n");

	Cvar_Init();
}

/*
==========================
Common_Shutdown()
==========================
*/
void Common_Shutdown(void)
{
	char runtimestring[STRINGLEN + 1];
	time_t shutdowntime;
	time_t runtime;

	Cvar_Cleanup();
	Z_Stats_f();
	// figure out runtime
	shutdowntime = time(NULL);
	runtime = shutdowntime - startuptime;
	Common_SecondsToString((unsigned long int) runtime, runtimestring, STRINGLEN);
	Sys_Printf("---- runtime: %s\n\n", runtimestring);
	if(logfile_fp)
	{
		fflush(logfile_fp);
		FS_FCloseFile(logfile_fp);
	}
}

/*
==========================
Common_SecondsToString()

From the oer irc bot, http://oer.equnet.org
==========================
*/
void Common_SecondsToString(unsigned long int seconds2, char *string, int maxlen)
{
	boolean_t morethansec;
	int fields;
	time_t seconds;
	time_t safe;
	time_t days;
	time_t hours;
	time_t minutes;
	char stringbuffer[STRINGLEN + 1];

	seconds = (time_t) seconds2;

	safe = seconds;
	days = seconds / 3600 / 24;
	seconds -= days * (3600 * 24);
	hours = seconds / 3600;
	seconds -= hours * (3600);
	minutes = seconds / 60;
	seconds -= minutes * 60;
	memset(string, 0, maxlen);
	fields = 0;
	// handle < 1sec runtimes
	morethansec = false;

	if(days > 0)
	{
		Common_snprintf(string, maxlen, "%lu %s ", days, (days == 1) ? "day" : "days");
		fields++;
		fields++;
		morethansec = true;
	}
	if(hours > 0)
	{
		Common_snprintf(stringbuffer, STRINGLEN, "%lu %s ", hours, (hours == 1) ? "hour" : "hours");
		strncat(string, stringbuffer, maxlen - strlen(string));
		fields++;
		morethansec = true;
	}
	if(minutes > 0)
	{
		Common_snprintf(stringbuffer, STRINGLEN, "%lu %s ", minutes, (minutes == 1) ? "minute" : "minutes");
		strncat(string, stringbuffer, maxlen - strlen(string));
		fields++;
		morethansec = true;
	}
	if(seconds > 0)
	{
		if(fields > 0)
			Common_snprintf(stringbuffer, STRINGLEN, "and %lu %s", seconds, (seconds == 1) ? "second" : "seconds");
		else
			Common_snprintf(stringbuffer, STRINGLEN, "%lu %s", seconds, (seconds == 1) ? "second" : "seconds");
		strncat(string, stringbuffer, maxlen - strlen(string));
		fields++;
		morethansec = true;
	}
	if(!morethansec)
		Common_snprintf(string, STRINGLEN, "< 1 second");
}

/*
==========================
Common_CopyString()

String must be Z_Free()'d explicitly!
==========================
*/
char *Common_CopyString(char *in)
{
	char *out;
	
	out = Z_Malloc(strlen(in) + 1);
	strcpy(out, in);
	return out;
}

/*
==========================
Common_strtolower()
==========================
*/
void Common_strtolower(char *str)
{
	unsigned char tmp;
	unsigned int i;

	if(!str)
		return;

	for(i = 0; i < strlen(str); i++)
	{
		tmp = (unsigned char) str[i];
		if((tmp >= 'A' && tmp <= 'Z'))
		{
			str[i] = tmp + 32;
		}
	}
}

/*
==============================================

                   SYS common

==============================================
*/

/*
==========================
Sys_Log()
==========================
*/
void Sys_Log(const char *logstring, int level)
{
	if((!nolog || !nolog->value) && logfile_fp)
	{
		if(level == LOG_NORMAL)
			fprintf(logfile_fp, logstring);
		else if(level == LOG_WARN)
			fprintf(logfile_fp, "Warning: %s", logstring);
		else if(level == LOG_ERROR)
			fprintf(logfile_fp, "Error: %s", logstring);
		else // default to NORMAL
			fprintf(logfile_fp, logstring);
	}
}

/*
==========================
Sys_GetTimeString()
==========================
*/
char *Sys_GetTimeString(void)
{
	time_t curtime = time(NULL);
	return ctime(&curtime);
}

/*
==============================================

                   MATH

==============================================
*/

/*
==========================
M_Vec3Magnitude()
==========================
*/
INLINE real_t M_Vec3Magnitude(vec3_t *vec)
{
	return ((real_t) sqrt((vec->x * vec->x) +
						  (vec->y * vec->y) +
						  (vec->z * vec->z)));
}

/*
==========================
M_Vec3Add()
==========================
*/
INLINE void M_Vec3Add(vec3_t *v1, vec3_t *v2, vec3_t *result)
{
	result->x = v1->x + v2->x;
	result->y = v1->y + v2->y;
	result->z = v1->z + v2->z;
}

/*
==========================
M_Vec3Subtract()
==========================
*/
INLINE void M_Vec3Subtract(vec3_t *v1, vec3_t *v2, vec3_t *result)
{
	result->x = v1->x - v2->x;
	result->y = v1->y - v2->y;
	result->z = v1->z - v2->z;
}

/*
==========================
M_Vec3Divide()
==========================
*/
INLINE void M_Vec3Divide(vec3_t *vec, real_t scalar, vec3_t *result)
{
	result->x = vec->x / scalar;
	result->y = vec->y / scalar;
	result->z = vec->z / scalar;
}

/*
==========================
M_Vec3Dot()
==========================
*/
INLINE real_t M_Vec3Dot(vec3_t *v1, vec3_t *v2)
{
	return ((v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z));
}

/*
==========================
M_Vec3Cross()
==========================
*/
INLINE void M_Vec3Cross(vec3_t *v1, vec3_t *v2, vec3_t *result)
{
	real_t v1x = v1->x;
	real_t v1y = v1->y;
	real_t v1z = v1->z;
	real_t v2x = v2->x;
	real_t v2y = v2->y;
	real_t v2z = v2->z;
	result->x = (v1y * v2z) - (v1z * v2y);
	result->y = (v1z * v2x) - (v1x * v2z);
	result->z = (v1x * v2y) - (v1y * v2x);
}

/*
==========================
M_Vec3Normalize()
==========================
*/
INLINE void M_Vec3Normalize(vec3_t *vec, vec3_t *result)
{
	real_t magnitude;
	magnitude = M_Vec3Magnitude(vec);
	result->x = vec->x / magnitude;
	result->y = vec->y / magnitude;
	result->z = vec->z / magnitude;
}

INLINE void M_MakeIdentity4x4(mat4x4_t *m)
{
	m->m11 = 1.0f; m->m21 = 0.0f; m->m31 = 0.0f; m->m41 = 0.0f;
	m->m12 = 0.0f; m->m22 = 1.0f; m->m32 = 0.0f; m->m42 = 0.0f;
	m->m13 = 0.0f; m->m23 = 0.0f; m->m33 = 1.0f; m->m43 = 0.0f;
	m->m14 = 0.0f; m->m24 = 0.0f; m->m34 = 0.0f; m->m44 = 1.0f;
}

/*
==============================================

                   CVAR

==============================================
*/
cvar_t *cvar_vars = NULL;

/*
==========================
Cvar_FindVar()
==========================
*/
static cvar_t *Cvar_FindVar(char *var_name)
{
	cvar_t	*var;
	
	for(var = cvar_vars; var; var = var->next)
		if(!strcmp(var_name, var->name))
			return var;

	return NULL;
}

/*
==========================
Cvar_VariableValue()
==========================
*/
real_t Cvar_VariableValue(char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar(var_name);
	if(!var)
		return 0;
	return (real_t) atof(var->string);
}

/*
==========================
Cvar_VariableString()
==========================
*/
char *Cvar_VariableString(char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar(var_name);
	if(!var)
		return "";
	return var->string;
}

/*
==========================
Cvar_Get()

If the variable already exists, the value will not be set
==========================
*/
cvar_t *Cvar_Get(char *var_name, char *var_value)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if(var)
		return var;

	if(!var_value)
		return NULL;

	var = (cvar_t *) Z_Malloc(sizeof(*var));
	var->name = Common_CopyString(var_name);
	var->string = Common_CopyString(var_value);
	var->value = (real_t) atof(var->string);

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	return var;
}

/*
==========================
Cvar_Set()
==========================
*/
cvar_t *Cvar_Set(char *var_name, char *value)
{
	cvar_t	*var;

	var = Cvar_FindVar(var_name);
	if(!var)
	{	// create it
		return Cvar_Get(var_name, value);
	}

	if(!strcmp(value, var->string))
		return var;		// not changed

	Z_Free(var->string);	// free the old value string
	
	var->string = Common_CopyString(value);
	var->value = (real_t) atof(var->string);

	return var;
}

/*
==========================
Cvar_SetValue()
==========================
*/
void Cvar_SetValue (char *var_name, real_t value)
{
	char val[STRINGLEN];

	if(value == (int) value)
		Common_snprintf(val, STRINGLEN, "%i", (int) value);
	else
		Common_snprintf(val, STRINGLEN, "%f", value);

	Cvar_Set(var_name, val);
}

/*
==========================
Cvar_ExecAutoexec()

Execs autoexec.cfg
==========================
*/
#define AUTOEXEC_CFG "autoexec.cfg"

void Cvar_ExecAutoexec(void)
{
	char line[STRINGLEN];
	char cvar[STRINGLEN];
	char stringvalue[STRINGLEN];
	real_t floatvalue;
	int numvalue;
	int linenr;
	FILE *fp;

	if(FS_FOpenFile(AUTOEXEC_CFG, &fp, "r") < 0)
		return;
	Sys_Printf("execing %s..\n", AUTOEXEC_CFG);
	linenr = 0;
	// parse all lines
	while(fgets(line, 256, fp))
	{
		memset(stringvalue, 0, STRINGLEN);
		floatvalue = 0.0f;
		numvalue = 0;
		// cvar, numerical floating point argument, enclosed in double quotes
		if(sscanf(line, "%s \"%f\"", cvar, &floatvalue) == 2)
		{
			// Is it a valid cvar?
			if(Cvar_FindVar(cvar))
			{
				Cvar_SetValue(cvar, floatvalue);
				// goto next line
				linenr++;
				continue;
			}
		}
		// cvar, numerical integer argument, enclosed in double quotes
		if(sscanf(line, "%s \"%d\"", cvar, &numvalue) == 2)
		{
			// Is it a valid cvar?
			if(Cvar_FindVar(cvar))
			{
				// yes, set it
				Cvar_SetValue(cvar, numvalue);
				// goto next line
				linenr++;
				continue;
			}
		}
		// cvar, string argument
		if(sscanf(line, "%s \"%s\"", cvar, stringvalue) == 2)
		{
			// Is it a valid cvar?
			if(Cvar_FindVar(cvar))
			{
				/*
				** The above sscanf seems to take the ending double quote with
				** it sometimes for some reason.. chop it off
				*/
				if(stringvalue[strlen(stringvalue) - 1] == '"')
					stringvalue[strlen(stringvalue) - 1] = 0;
				Cvar_Set(cvar, stringvalue);
				// goto next line
				linenr++;
				continue;
			}
		}
		// cvar, numerical floating point argument, not enclosed in double quotes
		if(sscanf(line, "%s %f", cvar, &floatvalue) == 2)
		{
			// Is it a valid cvar?
			if(Cvar_FindVar(cvar))
			{
				// yes, set it
				Cvar_SetValue(cvar, floatvalue);
				// goto next line
				linenr++;
				continue;
			}
		}
		// cvar, numerical integer argument, not enclosed in double quotes
		if(sscanf(line, "%s %d", cvar, &numvalue) == 2)
		{
			// Is it a valid cvar?
			if(Cvar_FindVar(cvar))
			{
				// yes, set it
				Cvar_SetValue(cvar, numvalue);
				// goto next line
				linenr++;
				continue;
			}
		}
		// eat comments
		if(strstr(line, "//"))
			continue;
		/*
		** If we get here we have an unrecognized line
		*/
		// FIXME: should we Sys_Printf here?
		// nolog/logfile/nostdout maybe not yet initialized
		//Sys_Printf("unrecognized variable and/or variable value at line %d\n", linenr);
		linenr++;
	}
}

/*
==========================
Cvar_Init()
==========================
*/
void Cvar_Init(void)
{
	// init default settings
	Cvar_Get("developer", "0");
	Cvar_Get("scr_height", "480");
	Cvar_Get("scr_width", "640");
	Cvar_Get("scr_fullscreen", "0");
	Cvar_Get("scr_xpos", "100");
	Cvar_Get("scr_ypos", "100");
	Cvar_Get("r_nearclip", "0.1");
	Cvar_Get("r_farclip", "4000");
	Cvar_Get("r_texanisotropy", "0.0");
	Cvar_Get("writecfg", "1");
	Cvar_Get("in_mouse", "1");
	Cvar_Get("in_dgamouse", "1");
	Cvar_Get("sensitivity", "0.5");
	Cvar_Get("m_filter", "0");
	Cvar_Get("m_yaw", "0.022");
	Cvar_Get("m_pitch", "0.022");
}

/*
==========================
Cvar_Cleanup()

Writes new autoexec.cfg with current
cvar values and then deletes all cvars
in memory
==========================
*/
void Cvar_Cleanup(void)
{
	cvar_t *writecfg;
	FILE *fp;
	cvar_t *var, *var2;
	char *cur_var;
	int i;
	boolean_t skip;
	// variables that we don't want to write
	const char *vars_to_skip[] = {
		"nolog",
		"nostdout",
		NULL
	};
	
	writecfg = Cvar_Get("writecfg", 0);
	if(writecfg && writecfg->value)
	{
		FS_FOpenFile(AUTOEXEC_CFG, &fp, "w");
		fprintf(fp, "// generated by %s on %s\n", appname, Sys_GetTimeString());
		for(var = cvar_vars; var; var = var->next)
		{
			skip = false;
			i = 0;
			// see if we should skip this
			while((cur_var = (char *) vars_to_skip[i]) != NULL)
			{
				if(!strcmp(cur_var, var->name))
				{
					skip = true;
					break;
				}
				i++;
			}
			if(skip)
				continue;
			fprintf(fp, "%s \"%s\"\n", var->name, var->string);
		}
		FS_FCloseFile(fp);
	}

	// free cvars
	var = cvar_vars;
	while(var != NULL)
	{
		var2 = var->next;
		if(var->name)
			Z_Free(var->name);
		if(var->string)
			Z_Free(var->string);
		Z_Free(var);
		var->next = NULL;
		var = var2;
	}
	// windows seems to think !nolog is false if we don't
	// explicitly null these
	nolog = NULL;
	nostdout = NULL;
}

/*
==========================
IMG_LoadTGA()
==========================
*/
#define  TGA_RGB      2   // uncompressed RGB
#define  TGA_RLERGB   10  // run-lenght encoded RGB

typedef struct {
	unsigned char imageIDLength;
	unsigned char colorMapType;
	unsigned char imageTypeCode;
	signed char colorMapOrigin;
	signed short colorMapLength;
	signed short colorMapEntrySize;
	signed short imageXOrigin;
	signed short imageYOrigin;
	signed short imageWidth;
	signed short imageHeight;
	unsigned char bitCount;
	unsigned char imageDescriptor;
} tgaheader_t;

boolean_t IMG_LoadTGA(image_t *newimage, char *imagefile)
{
	tgaheader_t tgaheader;
	FILE *imgfd;
	int channels;
	int tgasize;
	int i;
	unsigned char *tempbuf;
	unsigned char colorswap;
	unsigned char *tgadata;
	unsigned short pixels;
	unsigned char r, g, b;
	unsigned char rleid;
	int colorsread;
	unsigned char *pcolors;

	tgadata = NULL;
	channels = 0;
	tgasize = 0;
	i = 0;

	if(FS_FOpenFile(imagefile, &imgfd, "rb") < 0)
	{
		Sys_Warn("IMG_LoadTGA: unable to open %s: %s\n", imagefile, strerror(errno));
		return false;
	}

	// fill in the tga header
	tempbuf = (unsigned char *) Z_Malloc(18 * sizeof(unsigned char));
	fread(tempbuf, 18, 1, imgfd);
	memcpy(&tgaheader.imageIDLength, tempbuf, sizeof(tgaheader.imageIDLength));
	memcpy(&tgaheader.colorMapType, tempbuf + 1, sizeof(tgaheader.colorMapType));
	memcpy(&tgaheader.imageTypeCode, tempbuf + 2, sizeof(tgaheader.imageTypeCode));
	memcpy(&tgaheader.colorMapOrigin, tempbuf + 3, sizeof(tgaheader.colorMapOrigin));
	memcpy(&tgaheader.colorMapLength, tempbuf + 5, sizeof(tgaheader.colorMapLength));
	memcpy(&tgaheader.colorMapEntrySize, tempbuf + 7, sizeof(tgaheader.colorMapEntrySize));
	memcpy(&tgaheader.imageXOrigin, tempbuf + 9, sizeof(tgaheader.imageXOrigin));
	memcpy(&tgaheader.imageYOrigin, tempbuf + 11, sizeof(tgaheader.imageYOrigin));
	memcpy(&tgaheader.imageWidth, tempbuf + 12, sizeof(tgaheader.imageWidth));
	memcpy(&tgaheader.imageHeight, tempbuf + 14, sizeof(tgaheader.imageHeight));
	memcpy(&tgaheader.bitCount, tempbuf + 16, sizeof(tgaheader.bitCount));
	memcpy(&tgaheader.imageDescriptor, tempbuf + 17, sizeof(tgaheader.imageDescriptor));
	Z_Free(tempbuf);

	// see if this is a supported TGA file
	if((tgaheader.imageTypeCode != TGA_RGB) &&
	   (tgaheader.imageTypeCode != TGA_RLERGB))
	{
		Sys_Warn("IMG_LoadTGA: %s doesn't look like a TGA image, skipping\n", imagefile);
		FS_FCloseFile(imgfd);
		return false;
	}

	// Check if the image is RLE compressed or not
	if(tgaheader.imageTypeCode != TGA_RLERGB)
	{
		// Check if the image is a 24 or 32-bit image
		if(tgaheader.bitCount == 24 || tgaheader.bitCount == 32)
		{
			channels = tgaheader.bitCount / 8;
			tgasize = tgaheader.imageWidth * tgaheader.imageHeight * channels;
			tgadata = (unsigned char *) Z_Malloc(tgasize * sizeof(unsigned char));

			// Read in the TGA image data
			fread(tgadata, sizeof(unsigned char), tgasize, imgfd);

			// Change BGR to RGB
			for(i = 0; i < tgasize; i += channels)
			{
				colorswap = tgadata[i];
				tgadata[i] = tgadata[i + 2];
				tgadata[i + 2] = colorswap;
			}
		}
		// Check if the image is a 16 bit image (RGB stored in 1 unsigned short)
		else if(tgaheader.bitCount == 16)
		{
			pixels = 0;
			r = g = b = 0;

			// Since we convert 16-bit images to 24 bit, we hardcode the channels to 3.
			channels = 3;
			tgasize = tgaheader.imageWidth * tgaheader.imageHeight * channels;
			tgadata = (unsigned char *) Z_Malloc(tgasize * sizeof(unsigned char));

			// Load in all the pixel data pixel by pixel
			for(i = 0; i < tgaheader.imageWidth * tgaheader.imageHeight; i++)
			{
				// Read in the current pixel
				fread(&pixels, sizeof(unsigned short), 1, imgfd);

				// To convert a 16-bit pixel into an R, G, B, we need to
				// do some masking and such to isolate each color value.
				// 0x1f = 11111 in binary, so since 5 bits are reserved in
				// each unsigned short for the R, G and B, we bit shift and mask
				// to find each value.  We then bit shift up by 3 to get the full color.
				b = (pixels & 0x1f) << 3;
				g = ((pixels >> 5) & 0x1f) << 3;
				r = ((pixels >> 10) & 0x1f) << 3;

				// This essentially assigns the color to our array and swaps the
				// B and R values at the same time.
				tgadata[i * 3 + 0] = r;
				tgadata[i * 3 + 1] = g;
				tgadata[i * 3 + 2] = b;
			}
		}
		else
		{
			Sys_Warn("IMG_LoadTGA: %s has unsupported pixel format, not loading\n");
			return false;
		}
	}
	// Else, it must be Run-Length Encoded (RLE)
	else
	{
		rleid = 0;
		i = 0;
		colorsread = 0;
		channels = tgaheader.bitCount / 8;
		tgasize = tgaheader.imageWidth * tgaheader.imageHeight * channels;

		tgadata = (unsigned char *) Z_Malloc(tgasize * sizeof(unsigned char));
		pcolors = (unsigned char *) Z_Malloc(channels * sizeof(unsigned char));

		// Load in all the pixel data
		while(i < tgaheader.imageWidth * tgaheader.imageHeight) {
			// Read in the current color count + 1
			fread(&rleid, sizeof(unsigned char), 1, imgfd);

			// Check if we don't have an encoded string of colors
			if(rleid < 128)
			{
				// Increase the count by 1
				rleid++;

				// Go through and read all the unique colors found
				while(rleid)
				{
					// Read in the current color
					fread(pcolors, sizeof(unsigned char) * channels, 1, imgfd);

					// Store the current pixel in our image array
					tgadata[colorsread + 0] = pcolors[2];
					tgadata[colorsread + 1] = pcolors[1];
					tgadata[colorsread + 2] = pcolors[0];

					// If we have a 4 channel 32-bit image, assign one more for the alpha
					if(tgaheader.bitCount == 32)
						tgadata[colorsread + 3] = pcolors[3];

					// Increase the current pixels read, decrease the amount
					// of pixels left, and increase the starting index for the next pixel.
					i++;
					rleid--;
					colorsread += channels;
				}
			}
			// Else, let's read in a string of the same character
			else
			{
				// Minus the 128 ID + 1 (127) to get the color count that needs to be read
				rleid -= 127;

				// Read in the current color, which is the same for a while
				fread(pcolors, sizeof(unsigned char) * channels, 1, imgfd);

				// Go and read as many pixels as are the same
				while(rleid) {
                   // Assign the current pixel to the current index in our pixel array
                   tgadata[colorsread + 0] = pcolors[2];
                   tgadata[colorsread + 1] = pcolors[1];
                   tgadata[colorsread + 2] = pcolors[0];

                   // If we have a 4 channel 32-bit image, assign one more for the alpha
                   if(tgaheader.bitCount == 32)
                       tgadata[colorsread + 3] = pcolors[3];

                   // Increase the current pixels read, decrease the amount
                   // of pixels left, and increase the starting index for the next pixel.
                   i++;
                   rleid--;
                   colorsread += channels;
				}
			}
		}
		Z_Free(pcolors);
	}

	FS_FCloseFile(imgfd);

	newimage->format = (channels == 3) ? IMG_RGB : IMG_RGBA;
	newimage->width = tgaheader.imageWidth;
	newimage->height = tgaheader.imageHeight;
	newimage->data = tgadata;

	return true;
}

/*
=========================================================

3DS to mesh_t loader

REFERENCES:

  "3DS File Loader", www.gametutorials.com
   http://www.gametutorials.com/Tutorials/opengl/OpenGL_Pg4.htm

   "3D Studio File Format Information", Jochen Wilhelmy
   Available at http://www.wotsit.org/search.asp?s=3d

   "3ds format file reader", www.spacesimulator.net
   http://www.spacesimulator.net/tut4_3dsloader.html

=========================================================
*/

/*
** 3ds chunks
*/
#define CHUNK_MAIN                        0x4D4D    // main
#define   CHUNK_VERSION                   0x0002    // 3ds version
#define   CHUNK_OBJECTINFO                0x3D3D    // 3D editor
#define     CHUNK_MATERIAL                0xAFFF    // material block
#define       CHUNK_MATNAME               0xA000    // - material name
#define       CHUNK_MATAMBIENT            0xA010    // - material ambient
#define       CHUNK_MATDIFFUSE            0xA020    // - material diffuse
#define       CHUNK_MATSPECULAR           0xA030    // - material specular
#define       CHUNK_MATSHININESS          0xA040    // - material specular shininess
#define       CHUNK_MATTEXMAP1            0xA200    // - material texture map 1
#define       CHUNK_MATTEXMAP2            0xA33A    // - material texture map 2
#define       CHUNK_MATBUMPMAP            0xA230    // - material bump map
#define         CHUNK_MATMAPFILE          0xA300    //   - material tex/bump map texture filename
#define     CHUNK_OBJECT                  0x4000    // object block 
#define       CHUNK_OBJECT_MESH           0x4100    // - triangle mesh
#define         CHUNK_OBJECT_VERTICES     0x4110    //   - vertex list
#define         CHUNK_OBJECT_FACES        0x4120    //   - face list
#define           CHUNK_OBJECT_MATERIAL   0x4130    //     - faces material list
#define         CHUNK_OBJECT_UV           0x4140    //   - UV tex coords
#define   CHUNK_EDITKEYFRAME              0xB000    // keyframer block

static chunk_t *curchunk;
static chunk_t *tmpchunk;
static FILE *mesh_fd;

/*
==========================
MDL_Load3DS()
==========================
*/
boolean_t MDL_Load3DS(mesh_t *newmesh, char *modelfile)
{
	if(FS_FOpenFile(modelfile, &mesh_fd, "rb") < 0)
	{
		Sys_Warn("MDL_Load3DS: unable to open %s: %s\n", modelfile, strerror(errno));
		return false;
	}

	curchunk = (chunk_t *) Z_Malloc(sizeof(chunk_t));
	ReadNextChunk(curchunk);

    if(curchunk->id != CHUNK_MAIN)
	{
		Sys_Warn("MDL_Load3DS: invalid main chunk (!=0x4D4D) for %s, not loading\n", modelfile);
		FS_FCloseFile(mesh_fd);
		Z_Free(curchunk);
		return false;
	}

	tmpchunk = (chunk_t *) Z_Malloc(sizeof(*tmpchunk));

	ProcessNextChunk(newmesh, curchunk);
	ComputeNormals(newmesh);
	GL_PostProcessMesh(newmesh);

	Z_Free(tmpchunk);
	Z_Free(curchunk);
	FS_FCloseFile(mesh_fd);

	return true;
}

/*
==========================
ProcessNextChunk()
==========================
*/
void ProcessNextChunk(mesh_t *mesh, chunk_t *prevchunk)
{
	chunk_t *parentchunk;
	material_t *tmpmat;
	submesh_t *newsubmesh;
    unsigned int version = 0;
    int buffer[BIGBUFFERLEN];
	char strbuffer[STRINGLEN + 1];

	parentchunk = prevchunk;
	curchunk = (chunk_t *) Z_Malloc(sizeof(chunk_t));
	while(prevchunk->bytesread < prevchunk->length)
	{
		ReadNextChunk(curchunk);
		switch(curchunk->id)
		{
		case CHUNK_VERSION:
			curchunk->bytesread += fread(&version, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			if(version > 0x03)
				Sys_Warn("ProcessNextChunk: version > 3\n");
			break;
		case CHUNK_OBJECTINFO:
			ReadNextChunk(tmpchunk);
			tmpchunk->bytesread += fread(&version, 1, tmpchunk->length - tmpchunk->bytesread, mesh_fd);
			curchunk->bytesread += tmpchunk->bytesread;
			ProcessNextChunk(mesh, curchunk);
			break;
		case CHUNK_MATERIAL:
			tmpmat = GL_CreateNULLMaterial();
			ProcessNextMaterialChunk(tmpmat, curchunk);
			break;
		case CHUNK_OBJECT:
			newsubmesh = (submesh_t *) Z_Malloc(sizeof(*newsubmesh));
			curchunk->bytesread += GetString(strbuffer);
			newsubmesh->name = Common_CopyString(strbuffer);
			newsubmesh->parent = mesh;
			newsubmesh->next = NULL;
			GL_AddSubmesh(mesh, newsubmesh);
			ProcessNextObjectChunk(mesh, newsubmesh, curchunk);
			break;
		case CHUNK_EDITKEYFRAME:
			//ProcessNextKeyFrameChunk(mesh, curchunk);
			curchunk->bytesread += fread(buffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			break;
		default:
			curchunk->bytesread += fread(buffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			break;
		}
		// Add the bytes read from the last chunk to the previous chunk passed in.
		prevchunk->bytesread += curchunk->bytesread;
	}
	Z_Free(curchunk);
	curchunk = parentchunk;
}

/*
==========================
ProcessNextObjectChunk()
==========================
*/
void ProcessNextObjectChunk(mesh_t *mesh, submesh_t *submesh, chunk_t *prevchunk)
{
	chunk_t *parentchunk;
	int buffer[BIGBUFFERLEN];

	parentchunk = prevchunk;
	curchunk = (chunk_t *) Z_Malloc(sizeof(chunk_t));

	while (prevchunk->bytesread < prevchunk->length)
	{
		ReadNextChunk(curchunk);
		switch (curchunk->id)
		{
		case CHUNK_OBJECT_MESH:
			ProcessNextObjectChunk(mesh, submesh, curchunk);
			break;
		case CHUNK_OBJECT_VERTICES:
			ReadVertices(submesh, curchunk);
			break;
		case CHUNK_OBJECT_FACES:
			ReadVertexIndices(submesh, curchunk);
			break;
		case CHUNK_OBJECT_MATERIAL:
			ReadObjectMaterial(mesh, submesh, curchunk);
			break;
		case CHUNK_OBJECT_UV:
			ReadUVCoordinates(submesh, curchunk);
			break;
		default:  
			curchunk->bytesread += fread(buffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			break;
		}
		prevchunk->bytesread += curchunk->bytesread;
	}
	Z_Free(curchunk);
	curchunk = parentchunk;
}

/*
==========================
ProcessNextMaterialChunk()
==========================
*/
void ProcessNextMaterialChunk(material_t *material, chunk_t *prevchunk)
{
	chunk_t *parentchunk;
	int buffer[BIGBUFFERLEN];
	char strbuffer[STRINGLEN + 1];

	parentchunk = prevchunk;
	curchunk = (chunk_t *) Z_Malloc(sizeof(chunk_t));

	while(prevchunk->bytesread < prevchunk->length)
	{
		ReadNextChunk(curchunk);
		switch (curchunk->id)
		{
		case CHUNK_MATNAME:
			curchunk->bytesread += fread(strbuffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			if(material->name)
				Z_Free(material->name);
			material->name = Common_CopyString(strbuffer);
			break;
		case CHUNK_MATAMBIENT:
		case CHUNK_MATDIFFUSE:
		case CHUNK_MATSPECULAR:
			ReadColorChunk(material, curchunk);
			break;
		case CHUNK_MATTEXMAP1:
		case CHUNK_MATTEXMAP2:
		case CHUNK_MATBUMPMAP:
			ProcessNextMaterialChunk(material, curchunk);
			break;
		case CHUNK_MATMAPFILE:
			curchunk->bytesread += fread(strbuffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			AddMaterialMapfile(strbuffer, material, prevchunk);
			break;
		default:  
			curchunk->bytesread += fread(buffer, 1, curchunk->length - curchunk->bytesread, mesh_fd);
			break;
		}
		prevchunk->bytesread += curchunk->bytesread;
	}
	Z_Free(curchunk);
	curchunk = parentchunk;
}

void AddMaterialMapfile(char *mapfile, material_t *material, chunk_t *chunk)
{
	cvar_t *dev;

	Common_strtolower(mapfile);
	dev = Cvar_Get("developer", 0);
	switch(chunk->id)
	{
	case CHUNK_MATTEXMAP1:
		if(dev && dev->value)
			Sys_Printf("AddMaterialMapFile: texmap1\n");
		GL_LoadTexture(&material->texmap1, mapfile, true);
		material->hastexmap1 = true;
		break;
	case CHUNK_MATTEXMAP2:
		if(dev && dev->value)
			Sys_Printf("AddMaterialMapFile: texmap2\n");
		GL_LoadTexture(&material->texmap2, mapfile, true);
		material->hastexmap2 = true;
		break;
	case CHUNK_MATBUMPMAP:
		if(dev && dev->value)
			Sys_Printf("AddMaterialMapFile: bumpmap\n");
		GL_LoadTexture(&material->bumpmap, mapfile, true);
		material->hasbumpmap = true;
		break;
	default:
		break;
	}
}

/*
==========================
ReadNextChunk()

Reads in a chunk id and the chunks length
==========================
*/
void ReadNextChunk(chunk_t *chunk)
{
	chunk->bytesread = fread(&chunk->id, 1, 2, mesh_fd);
	chunk->bytesread += fread(&chunk->length, 1, 4, mesh_fd);
}

/*
==========================
GetString()
==========================
*/
int GetString(char *buffer)
{
	int index = 0;

	fread(buffer, 1, 1, mesh_fd);
	while(*(buffer + index++) != 0)
		fread(buffer + index, 1, 1, mesh_fd);
	return strlen(buffer) + 1;
}

/*
==========================
ReadColorChunk()
==========================
*/
void ReadColorChunk(material_t *material, chunk_t *chunk)
{
	unsigned char color[3];
	color4_t matcolor;

	ReadNextChunk(tmpchunk);
	tmpchunk->bytesread += fread(color, 1, tmpchunk->length - tmpchunk->bytesread, mesh_fd);

	matcolor.r = (real_t) ((real_t) color[0] / 255.0f);
	matcolor.g = (real_t) ((real_t) color[1] / 255.0f);
	matcolor.b = (real_t) ((real_t) color[2] / 255.0f);
	matcolor.a = (real_t) 1.0f;
	switch(chunk->id)
	{
	case CHUNK_MATAMBIENT:
		material->ambient = matcolor;
		break;
	case CHUNK_MATDIFFUSE:
		material->diffuse = matcolor;
		break;
	case CHUNK_MATSPECULAR:
		material->specular = matcolor;
		break;
	default:
		break;
	}

	chunk->bytesread += tmpchunk->bytesread;
}

/*
==========================
ReadVertexIndices()
==========================
*/
void ReadVertexIndices(submesh_t *submesh, chunk_t *prevchunk)
{
	unsigned short index = 0;
	unsigned int i, j;

	prevchunk->bytesread += fread(&submesh->numfaces, 1, 2, mesh_fd);
	submesh->faces = (face_t *) Z_Malloc(sizeof(face_t) * submesh->numfaces);
	memset(submesh->faces, 0, sizeof(face_t) * submesh->numfaces);

	for(i = 0; i < submesh->numfaces; i++)
	{
		for(j = 0; j < 4; j++)
		{
			prevchunk->bytesread += fread(&index, 1, sizeof(index), mesh_fd);
			if(j < 3)
			{
				submesh->faces[i].vertexindex[j] = index;
			}
		}
	}
}

/*
==========================
ReadUVCoordinates()
==========================
*/
void ReadUVCoordinates(submesh_t *submesh, chunk_t *prevchunk)
{
	prevchunk->bytesread += fread(&submesh->numtexcoords, 1, 2, mesh_fd);
	submesh->texcoords = (vec2_t *) Z_Malloc(sizeof(vec2_t) * submesh->numtexcoords);
	prevchunk->bytesread += fread(submesh->texcoords, 1, prevchunk->length - prevchunk->bytesread, mesh_fd);
}

/*
==========================
ReadVertices()
==========================
*/
void ReadVertices(submesh_t *submesh, chunk_t *prevchunk)
{
	real_t tmp;
	unsigned int i;

	prevchunk->bytesread += fread(&(submesh->numvertices), 1, 2, mesh_fd);
	submesh->vertexdata = (vec3_t *) Z_Malloc(sizeof(vec3_t) * submesh->numvertices);
	memset(submesh->vertexdata, 0, sizeof(vec3_t) * submesh->numvertices);
	prevchunk->bytesread += fread(submesh->vertexdata, 1, prevchunk->length - prevchunk->bytesread, mesh_fd);

	// swap Y and Z
	for(i = 0; i < submesh->numvertices; i++)
	{
		tmp = submesh->vertexdata[i].y;
		submesh->vertexdata[i].y = submesh->vertexdata[i].z;
		submesh->vertexdata[i].z = -tmp;
	}
}

/*
==========================
ReadObjectMaterial()
==========================
*/
void ReadObjectMaterial(mesh_t *mesh, submesh_t *submesh, chunk_t *prevchunk)
{
	char matname[BIGSTRINGLEN + 1];
	int buffer[BIGBUFFERLEN];
	material_t *mat;

	prevchunk->bytesread += GetString(matname);
	mat = GL_GetMaterial(matname);
	if(mat)
		submesh->material = mat;
	else
		submesh->material = NULL;

	prevchunk->bytesread += fread(buffer, 1, prevchunk->length - prevchunk->bytesread, mesh_fd);
}

/*
==========================
ComputeNormals()
==========================
*/
void ComputeNormals(mesh_t *mesh)
{
	vec3_t poly[3], v1, v2, normal, sum, zero;
	submesh_t *submesh;
	vec3_t *normals, *tmpnormals;
	int i, j;
	int shared;

	if(mesh->submeshpool == NULL)
		return;

	for(submesh = mesh->submeshpool; submesh; submesh = submesh->next)
	{
		normals = (vec3_t *) Z_Malloc(sizeof(vec3_t) * submesh->numfaces);
		tmpnormals  = (vec3_t *) Z_Malloc(sizeof(vec3_t) * submesh->numfaces);
		submesh->normaldata = (vec3_t *) Z_Malloc(sizeof(vec3_t) * submesh->numvertices);

		for(i = 0; i < submesh->numfaces; i++)
		{
			poly[0] = submesh->vertexdata[submesh->faces[i].vertexindex[0]];
			poly[1] = submesh->vertexdata[submesh->faces[i].vertexindex[1]];
			poly[2] = submesh->vertexdata[submesh->faces[i].vertexindex[2]];

			// face normals
			M_Vec3Subtract(&poly[0], &poly[2], &v1);
			M_Vec3Subtract(&poly[2], &poly[1], &v2);
			M_Vec3Cross(&v1, &v2, &normal);
			tmpnormals[i] = normal;
			M_Vec3Normalize(&normal, &normal);
			normals[i] = normal;
		}
		sum.x = 0.0;
		sum.y = 0.0;
		sum.z = 0.0;
		zero = sum;
		shared = 0;
		for(i = 0; i < submesh->numvertices; i++)
		{
			for(j = 0; j < submesh->numfaces; j++)
			{
				if(submesh->faces[j].vertexindex[0] == i ||
				   submesh->faces[j].vertexindex[1] == i ||
				   submesh->faces[j].vertexindex[2] == i)
				{
					M_Vec3Add(&sum, &tmpnormals[j], &sum);
					shared++;
				}
			}
			M_Vec3Divide(&sum, (real_t) (-shared), &submesh->normaldata[i]);
			M_Vec3Normalize(&submesh->normaldata[i], &submesh->normaldata[i]);
			sum = zero;
			shared = 0;
		}
		Z_Free(tmpnormals);
		Z_Free(normals);
	}
}

/*
=======================================================

                     Filesystem

=======================================================
*/

/*
==========================
FS_FileLength()
==========================
*/
int FS_FileLength(FILE *f)
{
	int pos;
	int end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}

/*
==========================
FS_FCloseFile()
==========================
*/
void FS_FCloseFile(FILE *f)
{
	fclose(f);
}

/*
==========================
FS_FOpenFile()

Opens file from DEMO_DATADIR.

FS_FOpenFile("data/file.tga", fd);

is equivalent to

FS_FOpenFile("file.tga", fd);
==========================
*/
int FS_FOpenFile(char *filename, FILE **file, const char *mode)
{
	char *tmp;
	char datadir2[STRINGLEN];
	char openfile[STRINGLEN];

	// check if file already includes datadir
	if(strlen(filename) > (strlen(datadir) + 1))
	{
		tmp = Common_CopyString(filename);
		tmp[strlen(datadir) + 1] = 0;
		Common_snprintf(datadir2, STRINGLEN, "%s/", datadir);
		if(!strcasecmp(tmp, datadir2)) // datadir already there
			Common_snprintf(openfile, STRINGLEN, filename);
		else                           // else add datadir
			Common_snprintf(openfile, STRINGLEN, "%s/%s", datadir, filename);
		Z_Free(tmp);
	}

	*file = fopen(openfile, mode);

	if(*file)
	{		
		return FS_FileLength(*file);
	}
	return -1;
}
