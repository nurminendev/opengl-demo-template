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

#include <stdio.h>
#include "extgl.h"
#include "common.h"
#include "common_gl.h"
#if PLATFORM == PLATFORM_LINUX
  #include <unistd.h>
#else
  #include <process.h>
#endif

static void GL_BeginFrame(void);
static void GL_RenderFrame(void);
static void GL_EndFrame(void);
static void DM_MainLoop(void);
static void DM_Shutdown(void);
void DM_KeyInput(int, boolean_t);
void DM_MouseButtonInput(int, boolean_t, int, int);
void DM_MouseMotionInput(int, int);
static void GL_Init(void);
static void GL_CheckExtensions(void);

#define DEMO_NAME       "Demo Template"
#define DEMO_VERSION    "1.0"
#define DEMO_DATADIR    "data"
#define DEMO_LOGFILE    "../demorun.log"
#define DEMO_COPYRIGHT1 "Copyright (C) 2003 Riku \"Rakkis\" Nurminen <rakkis@rakkis.net>"
#define DEMO_COPYRIGHT2 "This program comes with ABSOLUTELY NO WARRANTY; for details see the\n\
GNU General Public License (COPYING) provided with this distribution.\n\
This is free software, and you are welcome to redistribute it under\n\
certain conditions; again see COPYING for details."

/*
** Default lighting properties
*/
static const GLfloat def_global_ambient[] = {
	1.0f, 1.0f, 1.0f, 1.0f
};
#define IS_LOCAL_VIEWER  false
#define COLOR_CONTROL    GL_SINGLE_COLOR
// material
static const GLfloat def_mat_ambient[] = {
	1.0f, 1.0f, 1.0f, 1.0f
};
static const GLfloat def_mat_diffuse[] = {
	1.0f, 1.0f, 1.0f, 1.0f
};
static const GLfloat def_mat_specular[] = {
	1.0f, 1.0f, 1.0f, 1.0f
};
static const GLfloat def_mat_shininess = 40.0f;
static const GLfloat def_mat_emission[] = {
	0.0f, 0.0f, 0.0f, 1.0f
};

static boolean_t quitrequested;

#define WHITE         0
#define RED           1
#define GREEN         2
#define BLUE          3
#define BLACK         4
static color3_t color[5];

#define BIGROOM       0
#define NUM_MESHES    1
static mesh_t *meshes[NUM_MESHES];

/*
==========================
GL_BeginFrame()
==========================
*/
static void GL_BeginFrame(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

	GL_CameraLookAt();
}

/*
==========================
GL_RenderFrame()
==========================
*/
static void GL_RenderFrame(void)
{
	renderoperation_t ro;
	submesh_t *submesh;

	glPushMatrix();
	glTranslatef(0.0f, -80.0f, -340.0f);
	submesh = meshes[BIGROOM]->submeshpool;
	while(submesh != NULL)
	{
		GL_GetSubmeshRenderoperation(submesh, &ro);
		if(submesh->material)
			GL_BindMaterial(submesh->material);
		GL_RenderRenderoperation(&ro);
		submesh = submesh->next;
	}
	glPopMatrix();
}

/*
==========================
GL_PostFrame()
==========================
*/
static void GL_EndFrame(void)
{
#if DEBUG_MODE == 1
	int		err;
	err = glGetError();
	if(err != GL_NO_ERROR)
		Sys_Error("GL_PostFrame() failed: %s\n", GL_ErrorString(err));
#endif
	GL_Printf(10, 10, &color[WHITE], false, "fps: %.2f", common.curfps);
	glFlush();
}

/*
==========================
DM_MainLoop()
==========================
*/
static void DM_MainLoop(void)
{
	quitrequested = false;
	while(1)
	{
		IN_Frame();
		IN_HandleEvents();
		Common_PreFrame();
		GL_BeginFrame();
		GL_RenderFrame();
		GL_EndFrame();
		GLw_SwapBuffers();
		Common_PostFrame();
		if(quitrequested)
			DM_Shutdown();
	}
}

/*
==========================
DM_Shutdown()
==========================
*/
static void DM_Shutdown(void)
{
	GL_Shutdown();
	GLw_Shutdown();
	Common_Shutdown();
	_exit(0);
}

/*
==========================
DM_KeyInput()
==========================
*/
void DM_KeyInput(int key, boolean_t pressed)
{
	if(key == K_ESCAPE)
		quitrequested = true;

	else if(key == 'e' && pressed)
		common.cam_moveforward = 1;
	else if(key == 'e' && !pressed)
		common.cam_moveforward = 0;

	else if(key == 'd' && pressed)
		common.cam_moveforward = -1;
	else if(key == 'd' && !pressed)
		common.cam_moveforward = 0;

	else if(key == 's' && pressed)
		common.cam_sidestep = -1;
	else if(key == 's' && !pressed)
		common.cam_sidestep = 0;

	else if(key == 'f' && pressed)
		common.cam_sidestep = 1;
	else if(key == 'f' && !pressed)
		common.cam_sidestep = 0;
}

/*
==========================
DM_MouseButtonInput()
==========================
*/
void DM_MouseButtonInput(int button, boolean_t pressed, int x, int y)
{
	if(button == K_MOUSE1 && !pressed)
		printf("mouse1 released, x %d, y %d\n", x, y);
	if(button == K_MOUSE2 && !pressed)
		printf("mouse2 released, x %d, y %d\n", x, y);
	if(button == K_MOUSE3 && !pressed)
		printf("mouse3 released, x %d, y %d\n", x, y);
}

/*
==========================
DM_MouseMotionInput()
==========================
*/
void DM_MouseMotionInput(int x, int y)
{
}

/*
==========================
GL_Init()
==========================
*/
static void GL_Init(void)
{
	cvar_t *dev;

	Sys_Printf("---------- glinfo ----------\n");
	Sys_Printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
	Sys_Printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
	Sys_Printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glcaps.texunits);
	Sys_Printf("texunits: %d\n\n", glcaps.texunits);

	extgl_Initialize();

	GL_CheckExtensions();

	glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_TEXTURE_2D);

	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_LIGHTING);

	// default lighting params
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, def_global_ambient);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, IS_LOCAL_VIEWER ? GL_TRUE : GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, COLOR_CONTROL);

	// default material params
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, def_mat_ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, def_mat_diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, def_mat_specular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, def_mat_shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, def_mat_emission);

	GL_SetViewport();

	glFlush();

	// colortable for some common colors
	color[WHITE].r  =  1.0; color[WHITE].g  =  1.0; color[WHITE].b  =  1.0;
	color[RED].r    =  1.0; color[RED].g    =  0.0; color[RED].b    =  0.0;
	color[GREEN].r  =  0.0; color[GREEN].g  =  1.0; color[GREEN].b  =  0.0;
	color[BLUE].r   =  0.0; color[BLUE].g   =  0.0; color[BLUE].b   =  1.0;
	color[BLACK].r  =  0.0; color[BLACK].g  =  0.0; color[BLACK].b  =  0.0;

	GL_BuildFonts();

	// init camera position
	common.campos.x = 0.0f;
	common.campos.y = 0.0f;
	common.campos.z = 0.0f;
	common.camlookat.x = 1.0f;
	common.camlookat.y = 0.0f;
	common.camlookat.z = 0.0f;

	// camera speed
	common.camspeed = 1.0f;

	meshes[BIGROOM] = GL_LoadMesh("data/bigroom.3DS", "bigroom_mesh");
	GL_LoadVertexProgram(meshes[BIGROOM]->submeshpool, "testvp.arbvp");
	GL_LoadFragmentProgram(meshes[BIGROOM]->submeshpool, "testfp.arbfp");

	dev = Cvar_Get("developer", 0);
	// this generates a lot of output!
	//if(dev && dev->value)
	//	GL_PrintMeshInfo(meshes[BIGROOM]);
}

/*
==========================
GL_CheckExtensions()
==========================
*/
static void GL_CheckExtensions(void)
{
	Sys_Printf("-- Checking for GL extensions..\n");
	if(extgl_Extensions.ARB_multitexture)
		Sys_Printf("      - ARB_multitexture found\n");
	else
		Sys_Printf("      - ARB_multitexture not found\n");
	if(extgl_Extensions.ARB_vertex_buffer_object)
		Sys_Printf("      - ARB_vertex_buffer_object found\n");
	else
		Sys_Printf("      - ARB_vertex_buffer_object not found\n");
	if(extgl_Extensions.EXT_texture_filter_anisotropic)
	{
		Sys_Printf("      - EXT_texture_filter_anisotropic found");
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glcaps.maxanisotropy);
		Sys_Printf(", maximum degree of anisotropy: %.1f\n", glcaps.maxanisotropy);
	}
	else
	{
		Sys_Printf("      - EXT_texture_filter_anisotropic not found, r_texanisotropy ineffective\n");
	}
	if(extgl_Extensions.ARB_vertex_program)
		Sys_Printf("      - ARB_vertex_program found\n");
	else
		Sys_Printf("      - ARB_vertex_program not found\n");
	if(extgl_Extensions.ARB_fragment_program)
		Sys_Printf("      - ARB_fragment_program found\n");
	else
		Sys_Printf("      - ARB_fragment_program not found\n");
	Sys_Printf("\n");
}

/*
==========================
main()
==========================
*/
#if PLATFORM == PLATFORM_WIN32
extern void ParseWin32CommandLine(LPSTR lpCmdLine);
int argc;
char *argv[MAX_NUM_ARGVS];
HINSTANCE global_hInstance;
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
#if PLATFORM == PLATFORM_WIN32
	global_hInstance = hInstance;
	ParseWin32CommandLine(lpCmdLine);
#endif
	Common_Init(argc, argv, DEMO_LOGFILE, DEMO_NAME,
				DEMO_VERSION, DEMO_COPYRIGHT1, DEMO_COPYRIGHT2,
				DEMO_DATADIR);

	Cvar_ExecAutoexec();
	GLw_Init();
	GL_Init();
	// never returns
	DM_MainLoop();

	return 0;
}
