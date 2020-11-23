# Old/archived codebase

**This codebase is old/deprecated and probably would need updating to run today**. It is here for archival/nostalgia purposes.

# OpenGL Demo Template

This is an OpenGL template code, written in C for
speed and simplicity. It uses native X and Win32 calls for
window management, thus requiring only OpenGL for operation
(no GLUT, SDL, etc needed) and only the OpenGL headers and
libs and GLU for compiling.

# Features

- Mesh loading from 3dsmax 3DS format. Also loads:
  - Object material ambient, diffuse, and specular
    parameters as the corresponding OpenGL material params 
  - Object material diffuse map as primary texture
- ARB_vertex_program and ARB_fragment_program support
- Mesh data automatically stored in hardware buffers
  (ARB_vertex_buffer_object) if possible 
- Works in GNU/Linux (gcc 2.95 and 3.3 tested) and Windows (MSVC 6)
  and uses native X and Win32 calls for window management (no GLUT,
  SDL, etc needed). 

# Requirements

- Video card with working OpenGL drivers

If you want to compile the source yourself, you'll also need:

- The OpenGL headers and libraries
- The OpenGL Utility Library (GLU)

# Installing & running

**Binary:**

If you've downloaded a binary version, you can just run the executable
in the package. The following command line options can be used when
starting:

```
  -nostdout             Don't output anything to console
  -nolog                Don't write demorun.log
```

**Source:**

The source comes with a Makefile for GNU/Linux and a MSVC workspace
(.dsw) for Windows that you can use to compile the project.

# Key bindings

Keys used to interact are:

```
      ESC       Quit
      e         Move camera forward
      d         Move camera backward
      s         Sidestep camera left
      f         Sidestep camera right
```

Use the mouse to look around. If the mouse up/down movement
feels wrong to you, try inverting it with m_pitch (see below).
The default value for m_pitch is -0.022 which means the mouse
Y-axis is inverted.

# Config variables & autoexec.cfg

The autoexec.cfg is executed automatically from the data/
dir. You can change the default data directory in
demo.c (DEMO_DATADIR).

Any variable in the cfg will overwrite the corresponding
default value. The available config variables are listed
below along with their default values.

Lines that start with // are ignored (interpreted as a
comment). Any other unknown line will be printed to the
console (unless in win32 or started with -nostdout) and
logged to the logfile (unless started with -nolog).

Available variables:

```
scr_width <value> (default: 640)

  Sets the screen width.

scr_height <value> (default: 480)

  Sets the screen height.

scr_fullscreen <0|1> (default: 0)

  Sets fullscreen on (1) / off (0).

scr_xpos <value> (default: 100)

  Sets the X position of the upper left corner of
  the window when running in windowed mode.

scr_ypos <value> (default: 100)

  Sets the Y position of the upper left corner of
  the window when running in windowed mode.

r_nearclip <value> (default: 0.1)

  Sets the distance to the GL near clip plane.

r_farclip <value> (default: 4000)

  Sets the distance to the GL far clip plane.

r_texanisotropy

  Sets the degree of anisotropy to use for texture filtering.
  This variable is only effective if the EXT_texture_filter_anisotropy
  extension is available.

  For anisotropic filtering to take place, this variable must be set to 1.0
  or higher. If it is set higher than the maximum supported degree of anisotropy,
  it will be clamped to the maximum value.

writecfg <0|1> (default: 1)

  Sets whether to overwrite autoexec.cfg each time
  the app quits. If this is 0, the autoexec.cfg will
  never be touched. If it is 1, the cfg will be re-written
  at each app quit with the variable values that are set
  at that time.

in_mouse <0|1> (default: 1)

  Sets whether to receive mouse input.

sensitivity <value> (default: 0.5)

  Sets the mouse sensitivity. The higher the value, the faster
  the mouse will move.

in_dgamouse <0|1> (default: 1) [GNU/Linux ONLY]

  Sets whether to try to use the XFree86-DGA extension for
  mouse input in GNU/Linux. Using the DGA extension results in
  smoother motion. Not all X systems have the DGA extension.

m_filter <0|1> (default: 1)

  Sets whether to filter mouse movement. If enabled, the engine
  will take the average value between two consecutive numbers
  produced by the mouse input and use that value as the movement
  value. Enabling this variable will cause the mouse movement to be
  smoother but there will be slightly increased latency between the
  actual mouse movement and movement on the screen.

m_yaw <value> (default: 0.022)

  Sets the sensitivity for looking left and right with the mouse.

m_pitch <value> (default: -0.022)

  Sets the sensitivity for looking up and down with the mouse. Inverse
  value will naturally invert the mouse Y-axis looking.

developer <0|1> (default: 0)

  Sets "developer" mode on/off. Running in developer mode mode causes
  the engine to output a lot more extra information than normally.

  Note that this cvar is independant of the mode the app was
  compiled in (release or debug).
```

# Thanks & Acknowledgements:

- NeHe for providing the greatest OpenGL site and tutorials on the planet. Keep up the great work! http://nehe.gamedev.net
- id Software for releasing the Quake 2 source code under the GPL. http://www.idsoftware.com

# DISCLAIMER

```
############################################################################
#
# This demo is a product of Riku "Rakkis" Nurminen <riku@nurminen.dev> and
# released under the GNU General Public License (GPL). Please respect the
# terms and conditions in this license when copying/modifying/distributing
# this demo.
#
# A copy of the GPL is included within this package in the file COPYING.txt
# and on the Internet at http://www.gnu.org/licenses/gpl.txt
#
############################################################################
```