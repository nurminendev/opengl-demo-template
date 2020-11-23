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
** common_gl.c
**
** OpenGL routines
*/

#include "extgl.h"
#include "common.h"
#include "common_gl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <math.h>
#include <GL/glu.h>

mesh_t *GL_LoadMesh(char *, char *);
mesh_t *GL_CreateMesh(char *);
mesh_t *GL_GetMesh(char *);
void GL_AddSubmesh(mesh_t *, submesh_t *);
submesh_t *GL_GetSubmesh(mesh_t *, char *);
void GL_DeleteSubmesh(mesh_t *, submesh_t *);
void GL_DeleteSubmeshPool(mesh_t *);
void GL_DeleteMesh(mesh_t *);
void GL_DeleteMeshPool(void);
void GL_GetSubmeshRenderoperation(submesh_t *, renderoperation_t *);
void GL_RenderRenderoperation(renderoperation_t *);
void GL_PostProcessMesh(mesh_t *);
void GL_PrintMeshInfo(mesh_t *);
static int GL_GuessMeshType(char *);
static void GL_LinkSubmesh(mesh_t *, submesh_t *);
static void GL_UnlinkSubmesh(mesh_t *, submesh_t *);
static void GL_LinkMesh(mesh_t *);
static void GL_UnlinkMesh(mesh_t *);
image_t *GL_LoadImage(char *);
static int GL_GuessImageType(char *);
boolean_t GL_LoadTexture(GLuint *, char *, boolean_t);
void GL_DeleteAllTextures(material_t *);
static GLenum GL_ScaleImage(image_t *, int, int);
static GLenum GL_BuildMipmaps(image_t *, int *, char *);
material_t *GL_CreateNULLMaterial(void);
void GL_BindMaterial(material_t *);
material_t *GL_GetMaterial(char *);
void GL_DeleteMaterialPool(void);
static void GL_DeleteMaterial(material_t *);
static void GL_LinkMaterial(material_t *);
static void GL_UnlinkMaterial(material_t *);
void GL_SetViewport(void);
void GL_Perspective(real_t, real_t, real_t, real_t);
void GL_Shutdown(void);
void GL_BuildFonts(void);
void GL_Printf(GLint, GLint, color3_t *, boolean_t, const char *, ...);
void GL_CalcFPS(void);
void GL_CameraLookAt(void);
void GL_UpdateCamera(void);
static void GL_RotateCameraAroundAxis(int, vec3_t *);
boolean_t GL_LoadVertexProgram(submesh_t *, char *);
boolean_t GL_LoadFragmentProgram(submesh_t *, char *);
const char *GL_ErrorString(GLenum);

gl_capabilities_t glcaps;

static GLuint fontlist;
static GLuint fonttex;
static mesh_t *meshpool = NULL;
static material_t *materialpool = NULL;

extern int errno;

/*
=======================================================

                         MESH

=======================================================
*/

/*
GL_LoadMesh()

Tries to load a given mesh file with one of the
mesh loaders.

If meshname is NULL, the mesh will inherit
the first added submesh'es name

Returns the mesh upon success, or NULL on failure
==========================
*/
#define MDL_3DS      1
#define MDL_UNKNOWN  0

mesh_t *GL_LoadMesh(char *meshfile, char *meshname)
{
	mesh_t *newmesh;
	cvar_t *dev;
	boolean_t ok = false;

	if(!meshfile)
		return NULL;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_LoadMesh: loading %s..\n", meshfile);

	newmesh = GL_CreateMesh(NULL);

	switch(GL_GuessMeshType(meshfile))
	{
	case MDL_3DS:
		if(MDL_Load3DS(newmesh, meshfile))
			ok = true;
		break;
	case MDL_UNKNOWN:
		Sys_Warn("GL_LoadMesh: couldn't guess mesh type for %s, trying to load..\n", meshfile);
		// try to load the image with each loader..
		if(MDL_Load3DS(newmesh, meshfile))
		{
			ok = true;
			break;
		}
		// failed, it's not a supported image
		break;
	}
	if(ok)
	{
		if(meshname)
			newmesh->name = Common_CopyString(meshname);
		GL_LinkMesh(newmesh);
		return newmesh;
	}
	else
	{
		GL_DeleteMesh(newmesh);
	}
	return NULL;
}

/*
==========================
GL_CreateMesh()

If meshname is NULL, the mesh will inherit
the first added submesh'es name
==========================
*/
mesh_t *GL_CreateMesh(char *meshname)
{
	mesh_t *newmesh;

	newmesh = (mesh_t *) Z_Malloc(sizeof(*newmesh));

	if(meshname)
		newmesh->name = Common_CopyString(meshname);
	else
		newmesh->name = NULL;
	newmesh->submeshpool = NULL;
	newmesh->next = NULL;

	return newmesh;
}

/*
==========================
GL_GetMesh()
==========================
*/
mesh_t *GL_GetMesh(char *meshname)
{
	mesh_t *mesh;

	if(!meshname)
		return NULL;	
	for(mesh = meshpool; mesh != NULL; mesh = meshpool->next)
		if(!strcmp(meshname, mesh->name))
			return mesh;
	return NULL;
}

/*
==========================
GL_AddSubmesh()
==========================
*/
void GL_AddSubmesh(mesh_t *mesh, submesh_t *submesh)
{
	if(!mesh || !submesh)
		return;
	if(mesh->name == NULL)
		if(submesh->name)
			mesh->name = Common_CopyString(submesh->name);
	GL_LinkSubmesh(mesh, submesh);
}

/*
==========================
GL_GetSubmesh()
==========================
*/
submesh_t *GL_GetSubmesh(mesh_t *mesh, char *submeshname)
{
	submesh_t *submesh;

	if(!mesh || !submeshname)
		return NULL;
	for(submesh = mesh->submeshpool; submesh != NULL; submesh = mesh->submeshpool->next)
		if(!strcmp(submesh->name, submeshname))
			return submesh;
	return NULL;
}

/*
==========================
GL_DeleteSubmesh()
==========================
*/
void GL_DeleteSubmesh(mesh_t *mesh, submesh_t *submesh)
{
	cvar_t *dev;

	if(mesh == NULL || submesh == NULL)
		return;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_DeleteSubmesh: deleting submesh %s from %s..\n",
				   submesh->name, mesh->name);
	GL_UnlinkSubmesh(mesh, submesh);
	if(submesh->name)
		Z_Free(submesh->name);
	if(submesh->vertexdata)
		Z_Free(submesh->vertexdata);
	if(submesh->normaldata)
		Z_Free(submesh->normaldata);
	if(submesh->texcoords)
		Z_Free(submesh->texcoords);
	if(submesh->faces)
		Z_Free(submesh->faces);
	Z_Free(submesh);
	submesh->next = NULL;
	submesh = NULL;
}

/*
==========================
GL_DeleteSubmeshPool()
==========================
*/
void GL_DeleteSubmeshPool(mesh_t *mesh)
{
	submesh_t *submesh, *submesh2;

	if(!mesh)
		return;	
	submesh = mesh->submeshpool;
	while(submesh != NULL)
	{
		submesh2 = submesh->next;
		GL_DeleteSubmesh(mesh, submesh);
		submesh = submesh2;
	}
}

/*
==========================
GL_DeleteMesh()
==========================
*/
void GL_DeleteMesh(mesh_t *mesh)
{
	cvar_t *dev;

	if(!mesh)
		return;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_DeleteMesh: deleting %s..\n", mesh->name ? mesh->name : "(null)");

	GL_UnlinkMesh(mesh);
	GL_DeleteSubmeshPool(mesh);
	if(mesh->name)
	{
		Z_Free(mesh->name);
		mesh->name = NULL;
	}
	Z_Free(mesh);
	mesh->next = NULL;
	mesh = NULL;
}

/*
==========================
GL_DeleteMeshPool()
==========================
*/
void GL_DeleteMeshPool(void)
{
	mesh_t *mesh, *mesh2;

	mesh = meshpool;
	while(mesh != NULL)
	{
		mesh2 = mesh->next;
		GL_DeleteMesh(mesh);
		mesh = mesh2;
	}
}

/*
==========================
GL_GetSubmeshRenderoperation()
==========================
*/
void GL_GetSubmeshRenderoperation(submesh_t *submesh, renderoperation_t *ro)
{
	ro->rendermode = RM_TRIANGLES;
	if(extgl_Extensions.ARB_vertex_buffer_object)
	{
		ro->vertexvboptr = &submesh->vertexvboid;
		ro->normalvboptr = &submesh->normalvboid;
		ro->texcoordvboptr = &submesh->texcoordvboid;
	}
	else
	{
		ro->vertexvboptr = NULL;
		ro->normalvboptr = NULL;
		ro->texcoordvboptr = NULL;
	}
	ro->numvertices = submesh->numvertices * 3;
	ro->vertexdata = (real_t *) submesh->vertexdata;
	ro->normaldata = (real_t *) submesh->normaldata;
	ro->numtexcoords = submesh->numtexcoords;
	ro->texcoorddata = (real_t *) submesh->texcoords;
	ro->usefaceindices = true;
	ro->numfaceindices = submesh->numfaces * 3;
	ro->faceindices = (unsigned int *) submesh->faces;
	ro->hasvp = submesh->hasvertexprogram;
	ro->vpidptr = &submesh->vertexprogramid;
	ro->hasfp = submesh->hasfragmentprogram;
	ro->fpidptr = &submesh->fragmentprogramid;
}

/*
==========================
GL_RenderRenderoperation()
==========================
*/
void GL_RenderRenderoperation(renderoperation_t *ro)
{
	GLenum rm;
	GLenum type;

	if(!ro || !ro->rendermode || !ro->numvertices)
		return;

	if(ro->hasvp)
	{
		glEnable(GL_VERTEX_PROGRAM_ARB);
		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, *ro->vpidptr);
	}
	if(ro->hasfp)
	{
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, *ro->fpidptr);
	}

	type = (PRECISION == PRECISION_SINGLE) ? GL_FLOAT : GL_DOUBLE;

	if(extgl_Extensions.ARB_vertex_buffer_object)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, *ro->normalvboptr);
		glNormalPointer(type, 0, (char *) NULL);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, *ro->texcoordvboptr);
		glTexCoordPointer(2, type, 0, (char *) NULL);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, *ro->vertexvboptr);
		glVertexPointer(3, type, 0, (char *) NULL);
	}
	else
	{
		if(ro->normaldata)
			glNormalPointer(type, 0, ro->normaldata);
		if(ro->numtexcoords)
			glTexCoordPointer(2, type, 0, ro->texcoorddata);
		glVertexPointer(3, type, 0, ro->vertexdata);
	}
	switch(ro->rendermode)
	{
	case RM_TRIANGLES:      rm = GL_TRIANGLES;       break;
	case RM_TRIANGLE_STRIP: rm = GL_TRIANGLE_STRIP;  break;
	case RM_TRIANGLE_FAN:   rm = GL_TRIANGLE_FAN;    break;
	case RM_POLYGON:        rm = GL_POLYGON;         break;
	case RM_QUADS:          rm = GL_QUADS;           break;
	case RM_QUAD_STRIP:     rm = GL_QUAD_STRIP;      break;
	case RM_POINTS:         rm = GL_POINTS;          break;
	case RM_LINES:          rm = GL_LINES;           break;
	case RM_LINE_STRIP:     rm = GL_LINE_STRIP;      break;
	case RM_LINE_LOOP:      rm = GL_LINE_LOOP;       break;
	}
	if(ro->usefaceindices)
		glDrawElements(rm, ro->numfaceindices, GL_UNSIGNED_INT, ro->faceindices);
	else
		glDrawArrays(rm, 0, ro->numvertices);
	if(ro->hasfp)
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
	if(ro->hasvp)
		glDisable(GL_VERTEX_PROGRAM_ARB);
}

/*
==========================
GL_PostProcessMesh()

Currently deletes mesh data from system memory
if VBOs are available.. this should be changed when
mesh animation support is implemented
==========================
*/
void GL_PostProcessMesh(mesh_t *mesh)
{
	submesh_t *submesh;

	if(extgl_Extensions.ARB_vertex_buffer_object)
	{
		for(submesh = mesh->submeshpool; submesh != NULL; submesh = submesh->next)
		{
			glGenBuffersARB(1, &submesh->vertexvboid);
			glGenBuffersARB(1, &submesh->normalvboid);
			glGenBuffersARB(1, &submesh->texcoordvboid);

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, submesh->vertexvboid);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, submesh->numvertices * (3 * sizeof(real_t)),
							submesh->vertexdata, GL_STATIC_DRAW_ARB);

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, submesh->normalvboid);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, submesh->numvertices * (3 * sizeof(real_t)),
							submesh->normaldata, GL_STATIC_DRAW_ARB);

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, submesh->texcoordvboid);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, submesh->numtexcoords * (2 * sizeof(real_t)),
							submesh->texcoords, GL_STATIC_DRAW_ARB);

			Z_Free(submesh->vertexdata);
			submesh->vertexdata = NULL;
			Z_Free(submesh->normaldata);
			submesh->normaldata = NULL;
			Z_Free(submesh->texcoords);
			submesh->texcoords = NULL;
		}
	}
}

/*
==========================
GL_PrintMeshInfo()
==========================
*/
void GL_PrintMeshInfo(mesh_t *mesh)
{
	submesh_t *submesh;
	material_t *material;
	Sys_Printf("mesh: %s\n", mesh->name ? mesh->name : "(null)");
	for(submesh = mesh->submeshpool; submesh != NULL; submesh = submesh->next)
	{
		Sys_Printf(" ***** submesh: %s\n", submesh->name ? submesh->name : "(null)");
		Sys_Printf("   - submesh->numvertices: %d\n", submesh->numvertices);
		Sys_Printf("   - submesh->numfaces: %d\n", submesh->numfaces);
		Sys_Printf("   - submesh->numtexcoords: %d\n", submesh->numtexcoords);
		if(submesh->material)
		{
			material = submesh->material;
			Sys_Printf("   ** submesh->material: %s\n", material->name ? material->name : "(null)");
			Sys_Printf("     - material->hastexmap1: %s\n", material->hastexmap1 ? "true" : "false");
			Sys_Printf("     - material->hastexmap2: %s\n", material->hastexmap2 ? "true" : "false");
			Sys_Printf("     - material->hasbumpmap: %s\n", material->hasbumpmap ? "true" : "false");
			Sys_Printf("     - material->color: %.2f, %.2f, %.2f\n",
					   material->color.r, material->color.g, material->color.b);
			Sys_Printf("     - material->ambient: %.2f, %.2f, %.2f\n",
					   material->ambient.r, material->ambient.g, material->ambient.b);
			Sys_Printf("     - material->diffuse: %.2f, %.2f, %.2f\n",
					   material->diffuse.r, material->diffuse.g, material->diffuse.b);
			Sys_Printf("     - material->specular: %.2f, %.2f, %.2f\n",
					   material->specular.r, material->specular.g, material->specular.b);
			Sys_Printf("     - material->shininess: %.2f\n", material->shininess);
			Sys_Printf("     - material->emission: %.2f, %.2f, %.2f\n",
					   material->emission.r, material->emission.g, material->emission.b);
		}
	}
}

/*
==========================
GL_GuessMeshType()

Tries to guess mesh type from file extension
==========================
*/
static int GL_GuessMeshType(char *meshfile)
{
	char *ptr;

	if(!meshfile)
		return MDL_UNKNOWN;
	// try to guess image type
	if((ptr = strrchr(meshfile, '.')) != NULL) {
		if(!strcasecmp(ptr + 1, "3ds"))
			return MDL_3DS;
	}
	return MDL_UNKNOWN;
}

/*
==========================
GL_LinkSubmesh()
==========================
*/
static void GL_LinkSubmesh(mesh_t *mesh, submesh_t *submesh)
{
	submesh->next = mesh->submeshpool;
	mesh->submeshpool = submesh;
}

/*
==========================
GL_UnlinkSubmesh()
==========================
*/
static void GL_UnlinkSubmesh(mesh_t *mesh, submesh_t *submeshtounlink)
{
	submesh_t *submesh;

	if(!mesh || !submeshtounlink)
		return;

	if(submeshtounlink == mesh->submeshpool) // check if first link
	{
		mesh->submeshpool = mesh->submeshpool->next;
		return;
	}

	submesh = mesh->submeshpool;
	while(submesh != NULL)
	{
		if(submesh->next == submeshtounlink)
		{
			submesh->next = submesh->next->next;
			break;
		}
		submesh = submesh->next;
	}
}

/*
==========================
GL_LinkMesh()
==========================
*/
static void GL_LinkMesh(mesh_t *mesh)
{
	if(!mesh)
		return;
	mesh->next = meshpool;
	meshpool = mesh;
}

/*
==========================
GL_UnlinkMesh()
==========================
*/
static void GL_UnlinkMesh(mesh_t *meshtounlink)
{
	mesh_t *mesh;

	if(!meshtounlink)
		return;

	if(meshtounlink == meshpool) // check if first link
	{
		meshpool = meshpool->next;
		return;
	}

	mesh = meshpool;
	while(mesh != NULL)
	{
		if(mesh->next == meshtounlink)
		{
			mesh->next = mesh->next->next;
			break;
		}
		mesh = mesh->next;
	}
}

/*
=======================================================

                        IMAGE

=======================================================
*/
#define IMG_TGA      1
#define IMG_UNKNOWN  0

/*
==========================
GL_LoadImage()

Tries to load a given image file with one of the
image codecs.

Returns the image upon success, or NULL on failure
==========================
*/
image_t *GL_LoadImage(char *imagefile)
{
	image_t *newimage;
	cvar_t *dev;
	boolean_t ok = false;

	if(!imagefile)
		return NULL;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_LoadImage: loading %s..\n", imagefile);

	newimage = (image_t *) Z_Malloc(sizeof(*newimage));

	switch(GL_GuessImageType(imagefile))
	{
	case IMG_TGA:
		if(IMG_LoadTGA(newimage, imagefile))
			ok = true;
		break;
	case IMG_UNKNOWN:
		Sys_Warn("GL_LoadImage: couldn't guess image type for %s, trying to load..\n", imagefile);
		// try to load the image with each codec..
		if(IMG_LoadTGA(newimage, imagefile))
		{
			ok = true;
			break;
		}
		// failed, it's not a supported image
		break;
	}
	if(ok)
	{
		return newimage;
	}
	else
	{
		if(newimage->data)
			Z_Free(newimage->data);
		if(newimage)
			Z_Free(newimage);
	}
	return NULL;
}

/*
==========================
GL_GuessImageType()

Tries to guess image type from file extension
==========================
*/
static int GL_GuessImageType(char *imagefile)
{
	char *ptr;

	if(!imagefile)
		return IMG_UNKNOWN;
	// try to guess image type
	if((ptr = strrchr(imagefile, '.')) != NULL) {
		if(!strcasecmp(ptr + 1, "tga"))
			return IMG_TGA;
	}
	return IMG_UNKNOWN;
}

/*
==========================
GL_SetViewport()
==========================
*/
void GL_SetViewport(void)
{
	cvar_t *scrwidth, *scrheight, *nearclip, *farclip;
	int width, height;

	width = 640;
	height = 480;

	scrwidth = Cvar_Get("scr_width", 0);
	scrheight = Cvar_Get("scr_height", 0);
	if(scrwidth && scrwidth->value && scrheight && scrheight->value)
	{
		width = (int) scrwidth->value;
		height = (int) scrheight->value;
	}
	if(height == 0)
		height = 1;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	nearclip = Cvar_Get("r_nearclip", 0);
	farclip = Cvar_Get("r_farclip", 0);
	if(nearclip && nearclip->value && farclip && farclip->value)
		GL_Perspective(45.0f, (GLfloat) width / (GLfloat) height, nearclip->value, farclip->value);
	else
		GL_Perspective(45.0f, (GLfloat) width / (GLfloat) height, 0.1f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);
}

/*
==========================
GL_Perspective()

gluPerspective() implementation
from Mesa3D    http://www.mesa3d.org
==========================
*/
void GL_Perspective(real_t fovy, real_t aspect, real_t znear, real_t zfar)
{
	mat4x4_t m;
	real_t sine, cotangent, deltaz;
	real_t radians = fovy / 2 * M_PI / 180;

	deltaz = zfar - znear;
	sine = (real_t) sin(radians);
	if((deltaz == 0) || (sine == 0) || (aspect == 0))
		return;
	cotangent = (real_t) cos(radians) / sine;

	M_MakeIdentity4x4(&m);
	m.m11 = cotangent / aspect;
	m.m22 = cotangent;
	m.m33 = -(zfar + znear) / deltaz;
	m.m34 = -1;
	m.m43 = -2 * znear * zfar / deltaz;
	m.m44 = 0;

#if PRECISION == PRECISION_SINGLE
	glMultMatrixf((const GLfloat *) &m);
#else
	glMultMatrixd((const GLdouble *) &m);
#endif
}

/*
==========================
GL_Shutdown()
==========================
*/
void GL_Shutdown(void)
{
	glDeleteLists(fontlist, 256);

	GL_DeleteMeshPool();
	GL_DeleteMaterialPool();
}

/*
==========================
GL_BuildFonts()

From NeHe lesson 17
==========================
*/
void GL_BuildFonts(void)
{
	cvar_t *dev;
	GLfloat cx, cy;
	int i;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_BuildFonts: building fonts..\n");

	fontlist = glGenLists(256);
	if(!GL_LoadTexture(&fonttex, "data/fontmap.tga", false))
		Sys_Error("GL_BuildFonts: failed to load fontmap\n");
	glBindTexture(GL_TEXTURE_2D, fonttex);

	for(i = 0; i < 256; i++)
	{
		cx = (real_t) (i % 16) / 16.0f;
		cy = (real_t) (i / 16) / 16.0f;
		glNewList(fontlist + i, GL_COMPILE);
		  glEnable(GL_BLEND);
		  glBegin(GL_QUADS);
		    glTexCoord2f(cx, 1 - cy - 0.0625f);
			glVertex2i(0, 0);
			glTexCoord2f(cx + 0.0625f, 1 - cy - 0.0625f);
			glVertex2i(16, 0);
			glTexCoord2f(cx + 0.0625f, 1 - cy);
			glVertex2i(16, 16);
			glTexCoord2f(cx, 1 - cy);
			glVertex2i(0, 16);
		  glEnd();
		  glTranslated(10, 0, 0);
		  glDisable(GL_BLEND);
		glEndList();
	}
}

/*
==========================
GL_Printf()

From NeHe lesson 17
==========================
*/
void GL_Printf(GLint x, GLint y, color3_t *color, boolean_t italics, const char *fmt, ...)
{
	cvar_t *scrwidth, *scrheight;
	va_list argptr;
	char text[BIGSTRINGLEN + 1];

	if(fmt == NULL)
		return;

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	scrwidth = Cvar_Get("scr_width", 0);
	scrheight = Cvar_Get("scr_height", 0);

	glBindTexture(GL_TEXTURE_2D, fonttex);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	  glLoadIdentity();
	  if(scrwidth && scrwidth->value && scrheight && scrheight->value)
		  glOrtho(0.0f, (GLdouble) scrwidth->value, 0.0f, (GLdouble) scrheight->value, -1.0f, 1.0f);
	  else
		  glOrtho(0.0f, 640.0f, 0.0f, 480.0f, -1.0f, 1.0f);
	  glMatrixMode(GL_MODELVIEW);
	  glPushMatrix();
	    glLoadIdentity();
		glTranslated(x, y, 0);
		glListBase(fontlist - 32 + (128 * (italics ? 1 : 0)));
		glColor3f(color->r, color->g, color->b);
		glCallLists(strlen(text), GL_BYTE, text);
		glMatrixMode(GL_PROJECTION);
	  glPopMatrix();
	  glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
}

/*
==========================
GL_LoadTexture()
==========================
*/
boolean_t GL_LoadTexture(GLuint *texid, char *imagefile, boolean_t mipmaps)
{
	GLenum err;
	int num_mipmaps = 0;
	GLenum format;
	image_t *image;
	cvar_t *texanisotropy, *dev;

	if((image = GL_LoadImage(imagefile)) == NULL)
		return false;

	// GL textures must be atleast 64x64
	if(image->width < 64 ||
	   image->height < 64)
	{
		Sys_Warn("GL_LoadTexture: %s is smaller than 64x64 (size: %dx%d), scaling up..\n",
				 imagefile, image->width, image->height);
		// scale up if smaller than 64x64
		if(!(err = GL_ScaleImage(image, 64, 64)))
		{
			Sys_Printf(".. failed: %s\n", GL_ErrorString(err));
			return false;
		}
		else
		{
			Sys_Printf(".. ok\n");
		}
	}

	glGenTextures(1, texid);
	glBindTexture(GL_TEXTURE_2D, *texid);

	// see if we should build mipmaps
	if(mipmaps)
	{		
		// build mipmaps
		if((err = GL_BuildMipmaps(image, &num_mipmaps, imagefile)) != GL_TRUE)
		{
			Sys_Printf("GL_LoadTexture: failed to build mipmaps for %s: %s\n",
					   imagefile, GL_ErrorString(err));
			return false;
		}
	}
	else
	{
		format = (image->format == IMG_RGB) ? GL_RGB : GL_RGBA;
		// upload image without mipmaps
		glTexImage2D(GL_TEXTURE_2D,               // target texture
					 0,                           // lod level
					 format,                      // internalformat
					 image->width, image->height, // width/height
					 0,                           // border
					 format,                      // pixel data format
					 GL_UNSIGNED_BYTE,            // data type
					 image->data);                // image data
	}

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_LoadTexture: loaded texture %s with %d mipmaps..\n",
				   imagefile, num_mipmaps);

	Z_Free(image->data);
	Z_Free(image);

	// texture filtering
	if(mipmaps)     // automatically default to nicest result if mipmaps are enabled
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
		texanisotropy = Cvar_Get("r_texanisotropy", 0);
		if(extgl_Extensions.EXT_texture_filter_anisotropic &&
		   texanisotropy && (texanisotropy->value >= 1.0))
		{   // set texture anisotropy if available and requested
			if(texanisotropy->value > glcaps.maxanisotropy)
				Cvar_SetValue("r_texanisotropy", glcaps.maxanisotropy); // clamp to max first
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLfloat) texanisotropy->value);
		}
	}
	else            // otherwise just default to GL_LINEAR
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	
	}

	return true;
}

/*
==========================
GL_ScaleImage()
==========================
*/
static GLenum GL_ScaleImage(image_t *image, int width, int height)
{
	unsigned char *scaledimagedata;
	GLint scale_err;
	GLenum format;

	scaledimagedata = (unsigned char *) Z_Malloc((64 * 64) * sizeof(unsigned char));
	format = (image->format == IMG_RGB) ? GL_RGB : GL_RGBA;
	if((scale_err = gluScaleImage(format,                      // src image format
								  image->width, image->height, // src image width/height
								  GL_UNSIGNED_BYTE,            // src image data type
								  image->data,                 // src image data
								  width, height,               // dest image width/height
								  GL_UNSIGNED_BYTE,            // dest image data type
								  scaledimagedata)             // pointer to dest image data
		) == 0)
	{
		Z_Free(image->data);
		image->data = scaledimagedata;
		image->width = width;
		image->height = height;
		return GL_TRUE;
	}
	else
	{
		return scale_err;
	}
}

/*
==========================
GL_BuildMipmaps()

Builds mipmaps for a given image.
Returns the number of mipmaps generated in num_mipmaps
==========================
*/
static GLenum GL_BuildMipmaps(image_t *image, int *num_mipmaps, char *imagefile)
{
	GLenum mipmap_err;
	GLenum format;
	unsigned int width, height;
	boolean_t w_poweroftwo, h_poweroftwo;
	boolean_t bothpoweroftwo;
	int mipmap_count;

	width = image->width;
	height = image->height;
	bothpoweroftwo = true;

	/*
	  Check if image width & height are in powers of 2

	  There are probably easier ways to do this.. here I
	  check each bit of the value and see if it's 1, if at
	  the end we have one encounter with a 1, we have a correct
	  value, otherwise the value is not in powers of 2.
	*/
	w_poweroftwo = false;
	// switch bits to right until 0
	mipmap_count = 0;
	for(; width != 0; width >>= 1)
	{
		// check if we haven't encountered a 1-bit yet
		// and if the current (rightmost) bit is 1
		if(!w_poweroftwo && (width & 01))
			w_poweroftwo = true;              // it is 1, set flag to true
		else if(w_poweroftwo && (width & 01)) // if the flag is true and we encounter
			bothpoweroftwo = false;           // another 1, the value is not in powers of 2
		mipmap_count++;                       // count mipmaps
	}
	*num_mipmaps = mipmap_count;
	h_poweroftwo = false;
	// check height
	mipmap_count = 0;
	for(; height != 0; height >>= 1)
	{
		if(!h_poweroftwo && (height & 01))
			h_poweroftwo = true;
		else if(h_poweroftwo && (height & 01))
			bothpoweroftwo = false;
		mipmap_count++;
	}
	// bigger of width<->height denotes
	// actual number of mipmaps
	if(image->width < image->height)
		*num_mipmaps = mipmap_count;

	if(!bothpoweroftwo)
		Sys_Warn("GL_BuildMipmaps: size of image not in powers of two; texture will be scaled by gluBuild2DMipmaps()\n",
				 imagefile);

	format = (image->format == IMG_RGB) ? GL_RGB : GL_RGBA;
	if((mipmap_err = gluBuild2DMipmaps(GL_TEXTURE_2D,               // target texture
									   format,                      // internalformat
									   image->width, image->height, // width/height
									   format,                      // pixel data format
									   GL_UNSIGNED_BYTE,            // data type
									   image->data)                 // image data
		) == 0)
	{
		return GL_TRUE;
	}
	else
	{
		return mipmap_err;
	}
}

/*
==========================
GL_CreateNULLMaterial()

Creates an "empty" material with black
color, properties set to reflect all light,
shininess 40.0, no texture or bumpmap, and NULL
name.
==========================
*/
material_t *GL_CreateNULLMaterial(void)
{
	material_t *newmat;
	color4_t defmat_color;
	color4_t defmat_ambient;
	color4_t defmat_diffuse;
	color4_t defmat_specular;
	color4_t defmat_emission;
	real_t defmat_shininess;
	unsigned char facebits;

	newmat = (material_t *) Z_Malloc(sizeof(*newmat));

	defmat_color.r = 0.0f;
	defmat_color.g = 0.0f;
	defmat_color.b = 0.0f;
	defmat_color.a = 1.0f;
	defmat_ambient.r = 1.0f;
	defmat_ambient.g = 1.0f;
	defmat_ambient.b = 1.0f;
	defmat_ambient.a = 1.0f;
	defmat_diffuse.r = 1.0f;
	defmat_diffuse.g = 1.0f;
	defmat_diffuse.b = 1.0f;
	defmat_diffuse.a = 1.0f;
	defmat_specular.r = 1.0f;
	defmat_specular.g = 1.0f;
	defmat_specular.b = 1.0f;
	defmat_specular.a = 1.0f;
	defmat_shininess = 40.0f;
	defmat_emission.r = 0.0f;
	defmat_emission.g = 0.0f;
	defmat_emission.b = 0.0f;
	defmat_emission.a = 1.0f;
	facebits = FACE_FRONT_AND_BACK;

	newmat->name = NULL;
	newmat->hastexmap1 = false;
	newmat->hastexmap2 = false;
	newmat->texmap1 = 0;
	newmat->texmap2 = 0;
	newmat->color = defmat_color;
	newmat->ambient = defmat_ambient;
	newmat->diffuse = defmat_diffuse;
	newmat->specular = defmat_specular;
	newmat->shininess = defmat_shininess;
	newmat->emission = defmat_emission;
	newmat->facebits = facebits;
	newmat->bumpmap = 0;
	newmat->hasbumpmap = false;

	GL_LinkMaterial(newmat);

	return newmat;
}

/*
==========================
GL_BindMaterial()
==========================
*/
void GL_BindMaterial(material_t *material)
{
	GLenum face;

	face = GL_FRONT_AND_BACK;
	if(material->facebits & FACE_FRONT_AND_BACK)
		face = GL_FRONT_AND_BACK;
	else if(material->facebits & FACE_FRONT)
		face = GL_FRONT;
	else if(material->facebits & FACE_BACK)
		face = GL_BACK;
	glMaterialfv(face, GL_AMBIENT, (GLfloat *) &material->ambient);
	glMaterialfv(face, GL_DIFFUSE, (GLfloat *) &material->diffuse);
	glMaterialfv(face, GL_SPECULAR, (GLfloat *) &material->specular);
	glMaterialf(face, GL_SHININESS, (GLfloat) material->shininess);
	glMaterialfv(face, GL_EMISSION, (GLfloat *) &material->emission);
	glColor4fv((GLfloat *) &material->color);

	if(material->texmap1)
		glBindTexture(GL_TEXTURE_2D, material->texmap1);
}

/*
==========================
GL_GetMaterial()
==========================
*/
material_t *GL_GetMaterial(char *matname)
{
	material_t *mat;

	if(!matname)
		return NULL;
	for(mat = materialpool; mat; mat = mat->next)
		if(!strcmp(matname, mat->name))
			return mat;

	return NULL;
}

/*
==========================
GL_DeleteMaterial()
==========================
*/
static void GL_DeleteMaterial(material_t *material)
{
	cvar_t *dev;

	if(!material)
		return;

	dev = Cvar_Get("developer", 0);
	if(dev && dev->value)
		Sys_Printf("GL_DeleteMaterial: deleting %s..\n", material->name ? material->name : "(null)");

	GL_UnlinkMaterial(material);
	if(material->name)
		Z_Free(material->name);
	GL_DeleteAllTextures(material);
	Z_Free(material);
	material->next = NULL;
	material = NULL;
}

/*
==========================
GL_DeleteMaterialPool()
==========================
*/
void GL_DeleteMaterialPool(void)
{
	material_t *mat, *mat2;

	mat = materialpool;
	while(mat != NULL)
	{
		mat2 = mat->next;
		GL_DeleteMaterial(mat);
		mat = mat2;
	}
}

/*
==========================
GL_LinkMaterial()
==========================
*/
static void GL_LinkMaterial(material_t *mat)
{
	if(!mat)
		return;
	mat->next = materialpool;
	materialpool = mat;
}

/*
==========================
GL_UnlinkMaterial()
==========================
*/
static void GL_UnlinkMaterial(material_t *mattounlink)
{
	material_t *mat;

	if(!mattounlink)
		return;

	if(mattounlink == materialpool) // check if first link
	{
		materialpool = materialpool->next;
		return;
	}

	mat = materialpool;
	while(mat != NULL)
	{
		if(mat->next == mattounlink)
		{
			mat->next = mat->next->next;
			break;
		}
		mat = mat->next;
	}
}

/*
==========================
GL_DeleteAllTextures()
==========================
*/
void GL_DeleteAllTextures(material_t *material)
{
	if(material->hastexmap1 && glIsTexture(material->texmap1))
		glDeleteTextures(1, &material->texmap1);
	if(material->hastexmap2 && glIsTexture(material->texmap2))
		glDeleteTextures(1, &material->texmap2);
	if(material->hasbumpmap && glIsTexture(material->bumpmap))
		glDeleteTextures(1, &material->bumpmap);
}

/*
==========================
GL_CalcFPS()
==========================
*/
static real_t fpscount = 0.0f;

void GL_CalcFPS(void)
{
	static real_t lasttime = 0.0f;
    static real_t frametime = 0.0f;

	real_t currenttime = (real_t) Sys_GetMilliseconds();
	common.frameinterval = currenttime - frametime;
	frametime = currenttime;

	fpscount += 1.0f;
	if((currenttime - lasttime) > 1000.0f)
	{
		lasttime = currenttime;
		common.curfps = fpscount;
		fpscount = 0.0f;
	}
}

/*
==========================
GL_UpdateCamera()
==========================
*/
void GL_UpdateCamera(void)
{
	vec3_t axis, view;
	real_t deltax, deltay, deltaz;
	real_t speed;
	speed = 0.0f;
	if(common.cam_viewanglesdelta[PITCH])
	{
		// get view vector
		M_Vec3Subtract(&common.camlookat, &common.campos, &view);
		// get cross product of view and up vector to get local X axis
		M_Vec3Cross(&view, &common.up, &axis);                    
		M_Vec3Normalize(&axis, &axis);
		// rotate around local X axis
		GL_RotateCameraAroundAxis(PITCH, &axis);
		common.cam_viewanglesdelta[PITCH] = 0.0f;
	}
	if(common.cam_viewanglesdelta[YAW])
	{
		// rotate around Y axis
		axis.x = 0.0f; axis.y = 1.0f; axis.z = 0.0f;
		GL_RotateCameraAroundAxis(YAW, &axis);
		common.cam_viewanglesdelta[YAW] = 0.0f;
	}
	if(common.cam_sidestep != 0)
	{
		// get view vector
		M_Vec3Subtract(&common.camlookat, &common.campos, &view);
		// get cross product of view and up vector to get local X axis
		M_Vec3Cross(&view, &common.up, &axis);                    
		M_Vec3Normalize(&axis, &axis);
		speed = common.camspeed * common.frameinterval;
		deltax = axis.x * speed;
		deltaz = axis.z * speed;
		if(common.cam_sidestep < 0)
		{
			deltax = -deltax;
			deltaz = -deltaz;
		}
		common.campos.x += deltax;
		common.campos.z += deltaz;
		common.camlookat.x += deltax;
		common.camlookat.z += deltaz;
	}
	if(common.cam_moveforward != 0)
	{
		// get view vector
		M_Vec3Subtract(&common.camlookat, &common.campos, &view);
		M_Vec3Normalize(&view, &view);
		if(!speed)
			speed = common.camspeed * common.frameinterval;
		deltax = view.x * speed;
		deltay = view.y * speed;
		deltaz = view.z * speed;
		if(common.cam_moveforward < 0)
		{
			deltax = -deltax;
			deltay = -deltay;
			deltaz = -deltaz;
		}
		common.campos.x += deltax;
		common.campos.y += deltay;
		common.campos.z += deltaz;
		common.camlookat.x += deltax;
		common.camlookat.y += deltay;
		common.camlookat.z += deltaz;
	}
}

/*
==========================
GL_RotateCameraAroundAxis()

From www.gametutorials.com TimeBasedMovement

ANGLE IS AN INDEX TO common.viewangles[] !!!!
==========================
*/
void GL_RotateCameraAroundAxis(int angle, vec3_t *axis)
{
	vec3_t newcamlookat;
	vec3_t camview;
	real_t costheta, sintheta;
	real_t x, y, z;

	x = axis->x;
	y = axis->y;
	z = axis->z;

	M_Vec3Subtract(&common.camlookat, &common.campos, &camview);
	M_Vec3Normalize(&camview, &camview);

	costheta = (real_t) cos(common.cam_viewanglesdelta[angle]);
	sintheta = (real_t) sin(common.cam_viewanglesdelta[angle]);

    newcamlookat.x  = (costheta + (1 - costheta) * x * x)       * camview.x;
    newcamlookat.x += ((1 - costheta) * x * y - z * sintheta)   * camview.y;
    newcamlookat.x += ((1 - costheta) * x * z + y * sintheta)   * camview.z;

    newcamlookat.y  = ((1 - costheta) * x * y + z * sintheta)   * camview.x;
    newcamlookat.y += (costheta + (1 - costheta) * y * y)       * camview.y;
    newcamlookat.y += ((1 - costheta) * y * z - x * sintheta)   * camview.z;

    newcamlookat.z  = ((1 - costheta) * x * z - y * sintheta)   * camview.x;
    newcamlookat.z += ((1 - costheta) * y * z + x * sintheta)   * camview.y;
    newcamlookat.z += (costheta + (1 - costheta) * z * z)       * camview.z;

	M_Vec3Normalize(&newcamlookat, &newcamlookat);
	M_Vec3Add(&common.campos, &newcamlookat, &common.camlookat);
}

/*
==========================
GL_CameraLookAt()

gluLookAt() implementation
from Mesa3D   http://www.mesa3d.org

modified to use my nicer math routines
==========================
*/
void GL_CameraLookAt(void)
{
	mat4x4_t m;
	vec3_t xvec, yvec, zvec;

	// make rotation matrix

	// Z vector
	zvec.x = common.campos.x - common.camlookat.x;
	zvec.y = common.campos.y - common.camlookat.y;
	zvec.z = common.campos.z - common.camlookat.z;
	M_Vec3Normalize(&zvec, &zvec);

	// Y vector
	yvec.x = common.up.x;
	yvec.y = common.up.y;
	yvec.z = common.up.z;

	// X vector = Y cross Z
	M_Vec3Cross(&yvec, &zvec, &xvec);

	// Recompute Y = Z cross X
	M_Vec3Cross(&zvec, &xvec, &yvec);

	M_Vec3Normalize(&xvec, &xvec);
	M_Vec3Normalize(&yvec, &yvec);

	m.m11 = xvec.x; m.m21 = xvec.y; m.m31 = xvec.z; m.m41 = 0.0f;
	m.m12 = yvec.x; m.m22 = yvec.y; m.m32 = yvec.z; m.m42 = 0.0f;
	m.m13 = zvec.x; m.m23 = zvec.y; m.m33 = zvec.z; m.m43 = 0.0f;
	m.m14 = 0.0f;   m.m24 = 0.0f;   m.m34 = 0.0f;   m.m44 = 1.0f;

#if PRECISION == PRECISION_SINGLE
	glMultMatrixf((const GLfloat *) &m);
#else
	glMultMatrixd((const GLdouble *) &m);
#endif
	// translate
#if PRECISION == PRECISION_SINGLE
	glTranslatef(-common.campos.x, -common.campos.y, -common.campos.z);
#else
	glTranslated(-common.campos.x, -common.campos.y, -common.campos.z);
#endif
}

/*
==========================
GL_LoadVertexProgram()

existing vertex program will be deleted (if any)
==========================
*/
boolean_t GL_LoadVertexProgram(submesh_t *submesh, char *vpfile)
{
	FILE *fp;
	char buffer[BIGBUFFERLEN];
	char *str;
	int filelen, readlen;
	int errorpos, isnative;
	int value, value2;
	cvar_t *developer;

	if(!submesh || !vpfile)
		return false;

	developer = Cvar_Get("developer", 0);

	// check that we have the extension
	if(!extgl_Extensions.ARB_vertex_program)
	{
		if(developer && developer->value)
			Sys_Warn("GL_LoadVertexProgram: tried to load a vertex program without ARB_vertex_program!\n");
		return false;
	}

	if((filelen = FS_FOpenFile(vpfile, &fp, "r")) < 0)
	{
		Sys_Printf("GL_LoadVertexProgram: unable to open vpfile %s: %s\n", vpfile, strerror(errno));
		return false;
	}

	readlen = 0;
	if((readlen = fread(buffer, 1, filelen, fp)) < filelen)
	{
		Sys_Printf("GL_LoadVertexProgram: ERROR: read less than filelength!\n");
		return false;
	}

	// if there is a vp already, delete it
	if(submesh->hasvertexprogram && glIsProgramARB(submesh->vertexprogramid))
	{
		glDeleteProgramsARB(1, &submesh->vertexprogramid);
		submesh->hasvertexprogram = false;
	}

	glGenProgramsARB(1, &submesh->vertexprogramid);
	glBindProgramARB(GL_VERTEX_PROGRAM_ARB, submesh->vertexprogramid);
	glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, readlen, buffer);

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
	glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isnative);
	if(errorpos != -1)
	{
		Sys_Printf("GL_LoadVertexProgram: %s failed to load:\n", vpfile);
		str = (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		Sys_Printf("%s\n", str);
		if(developer && developer->value)
		{
			Sys_Printf("Program dump follows (starting at error):\n");
			Sys_Printf("-----------------------------------------\n");
			Sys_Printf("%s", buffer + errorpos);
			Sys_Printf("-----------------------------------------\n");
		}
		return false;
	}
	else if(errorpos == -1 && isnative == 0)
	{
		Sys_Warn("GL_LoadVertexProgram: %s loaded but exceeds native resource limits!\n");
		Sys_Warn("GL_LoadVertexProgram: %s MAY EXECUTE SUBOPTIMALLY!!!\n");
		if(developer && developer->value)
		{
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_LENGTH_ARB, &value);
			Sys_Printf("GL_LoadVertexProgram:    length: %d\n", value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_INSTRUCTIONS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    instructions: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_TEMPORARIES_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    temporaries: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_PARAMETERS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    parameters: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_ATTRIBS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_ATTRIBS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    attribs: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_ADDRESS_REGISTERS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    address registers: %d (%d native)\n", value, value2);
		}
	}
	else if(errorpos == -1 && isnative == 1)
	{
		if(developer && developer->value)
		{
			Sys_Printf("GL_LoadVertexProgram: %s for submesh %s (%s) loaded succesfully\n",
					   vpfile, submesh->name ? submesh->name : "(null)",
					   submesh->parent ? (submesh->parent->name ? submesh->parent->name : "(null)") : "(null)");
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_LENGTH_ARB, &value);
			Sys_Printf("GL_LoadVertexProgram:    length: %d\n", value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_INSTRUCTIONS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    instructions: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_TEMPORARIES_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    temporaries: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_PARAMETERS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    parameters: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_ATTRIBS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_ATTRIBS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    attribs: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_ADDRESS_REGISTERS_ARB, &value);
			glGetProgramivARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB, &value2);
			Sys_Printf("GL_LoadVertexProgram:    address registers: %d (%d native)\n", value, value2);
		}
	}
	submesh->hasvertexprogram = true;
	return true;
}

/*
==========================
GL_LoadFragmentProgram()
==========================
*/
boolean_t GL_LoadFragmentProgram(submesh_t *submesh, char *fpfile)
{
	FILE *fp;
	char buffer[BIGBUFFERLEN];
	char *str;
	int filelen, readlen;
	int errorpos, isnative;
	int value, value2;
	cvar_t *developer;

	if(!submesh || !fpfile)
		return false;

	developer = Cvar_Get("developer", 0);

	// check that we have the extension
	if(!extgl_Extensions.ARB_fragment_program)
	{
		if(developer && developer->value)
			Sys_Warn("GL_LoadFragmentProgram: tried to load a fragment program without ARB_fragment_program!\n");
		return false;
	}

	if((filelen = FS_FOpenFile(fpfile, &fp, "r")) < 0)
	{
		Sys_Printf("GL_LoadFragmentProgram: unable to open fpfile %s: %s\n", fpfile, strerror(errno));
		return false;
	}

	readlen = 0;
	if((readlen = fread(buffer, 1, filelen, fp)) < filelen)
	{
		Sys_Printf("GL_LoadFragmentProgram: ERROR: read less than filelength!\n");
		return false;
	}

	// if there is a fp already, delete it
	if(submesh->hasfragmentprogram && glIsProgramARB(submesh->fragmentprogramid))
	{
		glDeleteProgramsARB(1, &submesh->fragmentprogramid);
		submesh->hasfragmentprogram = false;
	}

	glGenProgramsARB(1, &submesh->fragmentprogramid);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, submesh->fragmentprogramid);
	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, readlen, buffer);

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorpos);
	glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isnative);
	if(errorpos != -1)
	{
		Sys_Printf("GL_LoadFragmentProgram: %s failed to load:\n", fpfile);
		str = (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		Sys_Printf("%s\n", str);
		if(developer && developer->value)
		{
			Sys_Printf("Program dump follows (starting at error):\n");
			Sys_Printf("-----------------------------------------\n");
			Sys_Printf("%s", buffer + errorpos);
			Sys_Printf("-----------------------------------------\n");
		}
		return false;
	}
	else if(errorpos == -1 && isnative == 0)
	{
		Sys_Warn("GL_LoadFragmentProgram: %s loaded but exceeds native resource limits!\n");
		Sys_Warn("GL_LoadFragmentProgram: %s MAY EXECUTE SUBOPTIMALLY!!!\n");
		if(developer && developer->value)
		{
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_LENGTH_ARB, &value);
			Sys_Printf("GL_LoadFragmentProgram:    length: %d\n", value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_INSTRUCTIONS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    instructions: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEMPORARIES_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    temporaries: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_PARAMETERS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    parameters: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ATTRIBS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ATTRIBS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    attribs: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ADDRESS_REGISTERS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    address registers: %d (%d native)\n", value, value2);
		}
	}
	else if(errorpos == -1 && isnative == 1)
	{
		if(developer && developer->value)
		{
			Sys_Printf("GL_LoadFragmentProgram: %s for submesh %s (%s) loaded succesfully\n",
					   fpfile, submesh->name ? submesh->name : "(null)",
					   submesh->parent ? (submesh->parent->name ? submesh->parent->name : "(null)") : "(null)");
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_LENGTH_ARB, &value);
			Sys_Printf("GL_LoadFragmentProgram:    length: %d\n", value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_INSTRUCTIONS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    instructions: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEMPORARIES_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEMPORARIES_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    temporaries: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_PARAMETERS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    parameters: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ATTRIBS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ATTRIBS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    attribs: %d (%d native)\n", value, value2);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ADDRESS_REGISTERS_ARB, &value);
			glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB, &value2);
			Sys_Printf("GL_LoadFragmentProgram:    address registers: %d (%d native)\n", value, value2);
		}
	}
	submesh->hasfragmentprogram = true;
	return true;
}

/*
==========================
GL_ErrorString()
==========================
*/
const char *GL_ErrorString(GLenum errorcode)
{
   if(errorcode == GL_NO_ERROR)
      return (const char *) "no error";
   else if(errorcode == GL_INVALID_VALUE)
      return (const char *) "invalid value";
   else if(errorcode == GL_INVALID_ENUM)
      return (const char *) "invalid enum";
   else if(errorcode == GL_INVALID_OPERATION)
      return (const char *) "invalid operation";
   else if(errorcode == GL_STACK_OVERFLOW)
      return (const char *) "stack overflow";
   else if(errorcode == GL_STACK_UNDERFLOW)
      return (const char *) "stack underflow";
   else if(errorcode == GL_OUT_OF_MEMORY)
      return (const char *) "out of memory";
   else
      return NULL;
}
