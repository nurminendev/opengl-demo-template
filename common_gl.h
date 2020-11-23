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

#ifndef __COMMON_GL_H__
#define __COMMON_GL_H__

typedef struct
{
	int texunits;
	real_t maxanisotropy;
} gl_capabilities_t;

extern gl_capabilities_t glcaps;

typedef enum
{
	RM_POINTS = 1,
	RM_LINES,
	RM_LINE_STRIP,
	RM_LINE_LOOP,
	RM_TRIANGLES,
	RM_TRIANGLE_STRIP,
	RM_TRIANGLE_FAN,
	RM_QUADS,
	RM_QUAD_STRIP,
	RM_POLYGON
} rendermode_t;

typedef struct
{
	rendermode_t rendermode;
	unsigned int *vertexvboptr;
	unsigned int *normalvboptr;
	unsigned int *texcoordvboptr;
	int numvertices;
	real_t *vertexdata;
	real_t *normaldata;
	int numtexcoords;
	real_t *texcoorddata;
	boolean_t usefaceindices;
	int numfaceindices;
	unsigned int *faceindices;
	boolean_t hasvp;
	unsigned int *vpidptr;
	boolean_t hasfp;
	unsigned int *fpidptr;
} renderoperation_t;

extern mesh_t *GL_LoadMesh(char *, char *);
extern mesh_t *GL_CreateMesh(char *);
extern mesh_t *GL_GetMesh(char *);
extern void GL_AddSubmesh(mesh_t *, submesh_t *);
extern submesh_t *GL_GetSubmesh(mesh_t *, char *);
extern void GL_DeleteSubmesh(mesh_t *, submesh_t *);
extern void GL_DeleteSubmeshPool(mesh_t *);
extern void GL_DeleteMesh(mesh_t *);
extern void GL_DeleteMeshPool(void);
extern void GL_GetSubmeshRenderoperation(submesh_t *, renderoperation_t *);
extern void GL_PrintMeshInfo(mesh_t *);
extern void GL_PostProcessMesh(mesh_t *);
extern void GL_RenderRenderoperation(renderoperation_t *);
extern boolean_t GL_LoadTexture(GLuint *, char *, boolean_t);
extern void GL_DeleteAllTextures(material_t *);
extern image_t *GL_LoadImage(char *);
extern material_t *GL_CreateNULLMaterial(void);
extern void GL_BindMaterial(material_t *);
extern material_t *GL_GetMaterial(char *);
extern void GL_DeleteMaterialPool(void);
extern void GL_SetViewport(void);
extern void GL_Perspective(real_t, real_t, real_t, real_t);
extern void GL_Shutdown(void);
extern void GL_BuildFonts(void);
extern void GL_Printf(GLint, GLint, color3_t *, boolean_t, const char *, ...);
extern void GL_CalcFPS(void);
extern void GL_CameraLookAt(void);
extern void GL_UpdateCamera(void);
extern boolean_t GL_LoadVertexProgram(submesh_t *, char *);
extern boolean_t GL_LoadFragmentProgram(submesh_t *, char *);
extern const char *GL_ErrorString(GLenum);

#endif // __COMMON_GL_H__
