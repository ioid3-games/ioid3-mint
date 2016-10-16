/*
=======================================================================================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Spearmint Source Code.
If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms. You should have received a copy of these additional
terms immediately following the terms and conditions of the GNU General Public License. If not, please request a copy in writing from
id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
=======================================================================================================================================
*/

#ifdef USE_LOCAL_HEADERS
#include "SDL.h"
#else
#include <SDL.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../renderercommon/tr_common.h"
#include "../sys/sys_local.h"
#include "sdl_icon.h"

typedef enum {
	RSERR_OK,
	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,
	RSERR_UNKNOWN
} rserr_t;

SDL_Window *SDL_window = NULL;
static SDL_GLContext SDL_glContext = NULL;

cvar_t *r_allowSoftwareGL; // Don't abort out if a hardware visual can't be obtained
cvar_t *r_allowResize; // make window resizable
cvar_t *r_centerWindow;
cvar_t *r_sdlDriver;
cvar_t *r_forceWindowIcon32;
// GL_ARB_multisample
void (APIENTRYP qglActiveTextureARB)(GLenum texture);
void (APIENTRYP qglClientActiveTextureARB)(GLenum texture);
void (APIENTRYP qglMultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);
// GL_EXT_compiled_vertex_array
void (APIENTRYP qglLockArraysEXT)(GLint first, GLsizei count);
void (APIENTRYP qglUnlockArraysEXT)(void);
// GL_ARB_texture_compression
void (APIENTRYP qglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

/*
=======================================================================================================================================
GLimp_Shutdown
=======================================================================================================================================
*/
void GLimp_Shutdown(void) {

	ri.IN_Shutdown();

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/*
=======================================================================================================================================
GLimp_Minimize

Minimize the game so that user is back at the desktop.
=======================================================================================================================================
*/
void GLimp_Minimize(void) {
	SDL_MinimizeWindow(SDL_window);
}

/*
=======================================================================================================================================
GLimp_LogComment
=======================================================================================================================================
*/
void GLimp_LogComment(char *comment) {

}

/*
=======================================================================================================================================
GLimp_CompareModes
=======================================================================================================================================
*/
static int GLimp_CompareModes(const void *a, const void *b) {
	const float ASPECT_EPSILON = 0.001f;
	SDL_Rect *modeA = (SDL_Rect *)a;
	SDL_Rect *modeB = (SDL_Rect *)b;
	float aspectA = (float)modeA->w / (float)modeA->h;
	float aspectB = (float)modeB->w / (float)modeB->h;
	int areaA = modeA->w * modeA->h;
	int areaB = modeB->w * modeB->h;
	float aspectDiffA = fabs(aspectA - glConfig.displayAspect);
	float aspectDiffB = fabs(aspectB - glConfig.displayAspect);
	float aspectDiffsDiff = aspectDiffA - aspectDiffB;

	if (aspectDiffsDiff > ASPECT_EPSILON) {
		return 1;
	} else if (aspectDiffsDiff < -ASPECT_EPSILON) {
		return -1;
	} else {
		return areaA - areaB;
	}
}

/*
=======================================================================================================================================
GLimp_DetectAvailableModes
=======================================================================================================================================
*/
static void GLimp_DetectAvailableModes(void) {
	int i, j;
	char buf[MAX_STRING_CHARS] = {0};
	int numSDLModes;
	SDL_Rect *modes;
	int numModes = 0;

	SDL_DisplayMode windowMode;

	int display = SDL_GetWindowDisplayIndex(SDL_window);

	if (display < 0) {
		ri.Printf(PRINT_WARNING, "Couldn't get window display index, no resolutions detected: %s\n", SDL_GetError());
		return;
	}

	numSDLModes = SDL_GetNumDisplayModes(display);

	if (SDL_GetWindowDisplayMode(SDL_window, &windowMode) < 0 || numSDLModes <= 0) {
		ri.Printf(PRINT_WARNING, "Couldn't get window display mode, no resolutions detected: %s\n", SDL_GetError());
		return;
	}

	modes = SDL_calloc((size_t)numSDLModes, sizeof(SDL_Rect));

	if (!modes) {
		ri.Error(ERR_FATAL, "Out of memory");
	}

	for (i = 0; i < numSDLModes; i++) {
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(display, i, &mode) < 0) {
			continue;
		}

		if (!mode.w || !mode.h) {
			ri.Printf(PRINT_ALL, "Display supports any resolution\n");
			SDL_free(modes);
			return;
		}

		if (windowMode.format != mode.format) {
			continue;
		}
		// SDL can give the same resolution with different refresh rates.
		// Only list resolution once.
		for (j = 0; j < numModes; j++) {
			if (mode.w == modes[j].w && mode.h == modes[j].h) {
				break;
			}
		}

		if (j != numModes) {
			continue;
		}

		modes[numModes].w = mode.w;
		modes[numModes].h = mode.h;
		numModes++;
	}

	if (numModes > 1) {
		qsort(modes, numModes, sizeof(SDL_Rect), GLimp_CompareModes);
	}

	for (i = 0; i < numModes; i++) {
		const char *newModeString = va("%ux%u ", modes[i].w, modes[i].h);

		if (strlen(newModeString) < (int)sizeof(buf) - strlen(buf)) {
			Q_strcat(buf, sizeof(buf), newModeString);
		} else {
			ri.Printf(PRINT_WARNING, "Skipping mode %ux%u, buffer too small\n", modes[i].w, modes[i].h);
		}
	}

	if (*buf) {
		buf[strlen(buf) - 1] = 0;
		ri.Printf(PRINT_ALL, "Available modes: '%s'\n", buf);
		ri.Cvar_Set("r_availableModes", buf);
	}

	SDL_free(modes);
}

/*
=======================================================================================================================================
GLimp_SetMode
=======================================================================================================================================
*/
static int GLimp_SetMode(int mode, qboolean fullscreen, qboolean noborder) {
	const char *glstring;
	int perChannelColorBits;
	int colorBits, depthBits, stencilBits;
	int samples;
	int i = 0;
	SDL_Surface *icon = NULL;
	Uint32 flags = SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL;
	SDL_DisplayMode desktopMode;
	int display = 0;
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
	char windowTitle[128];
	textureLevel_t *iconPic;
	byte *pixelData;
	int bytesPerPixel, width, height;

	ri.Printf(PRINT_ALL, "Initializing OpenGL display\n");
	ri.Cvar_VariableStringBuffer("com_productName", windowTitle, sizeof(windowTitle));

	if (r_allowResize->integer) {
		flags |= SDL_WINDOW_RESIZABLE;
	}

	iconPic = NULL;
	pixelData = NULL;
	bytesPerPixel = 4;
#ifdef USE_ICON
	// try to load 32 x 32 icon
	if (r_forceWindowIcon32->integer) {
		int numLevels;

		R_LoadImage("windowicon32", &numLevels, &iconPic);

		if (iconPic && (iconPic[0].width != 32 || iconPic[0].height != 32)) {
			ri.Free(iconPic);
			iconPic = NULL;
			ri.Printf(PRINT_WARNING, "Ignoring windowicon32: Image must be 32 x 32!\n");
		}
	} else {
		int numLevels;

		// try to load high resolution icon
		R_LoadImage("windowicon", &numLevels, &iconPic);
	}

	if (iconPic) {
		pixelData = iconPic[0].data;
		width = iconPic[0].width;
		height = iconPic[0].height;
	} else {
		// fallback to default icon
		pixelData = (byte *)CLIENT_WINDOW_ICON.pixel_data;
		bytesPerPixel = CLIENT_WINDOW_ICON.bytes_per_pixel;
		width = CLIENT_WINDOW_ICON.width;
		height = CLIENT_WINDOW_ICON.height;
	}

	icon = SDL_CreateRGBSurfaceFrom((void *)pixelData, width, height, bytesPerPixel * 8, bytesPerPixel * width,
#ifdef Q3_LITTLE_ENDIAN
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000
#else
			0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF
#endif
			);
#endif
	// If a window exists, note its display index
	if (SDL_window != NULL) {
		display = SDL_GetWindowDisplayIndex(SDL_window);

		if (display < 0) {
			ri.Printf(PRINT_DEVELOPER, "SDL_GetWindowDisplayIndex() failed: %s\n", SDL_GetError());
		}
	}

	if (display >= 0 && SDL_GetDesktopDisplayMode(display, &desktopMode) == 0) {
		glConfig.displayWidth = desktopMode.w;
		glConfig.displayHeight = desktopMode.h;
		glConfig.displayAspect = (float)desktopMode.w / (float)desktopMode.h;

		ri.Printf(PRINT_ALL, "Display aspect: %.3f\n", glConfig.displayAspect);
	} else {
		Com_Memset(&desktopMode, 0, sizeof(SDL_DisplayMode));

		glConfig.displayWidth = 640;
		glConfig.displayHeight = 480;
		glConfig.displayAspect = 1.333f;

		ri.Printf(PRINT_ALL, "Cannot determine display aspect, assuming 1.333\n");
	}

	ri.Printf(PRINT_ALL, "...setting mode %d:", mode);

	if (mode == -2) {
		// use desktop video resolution
		if (desktopMode.h > 0) {
			glConfig.vidWidth = desktopMode.w;
			glConfig.vidHeight = desktopMode.h;
		} else {
			glConfig.vidWidth = 640;
			glConfig.vidHeight = 480;
			ri.Printf(PRINT_ALL, "Cannot determine display resolution, assuming 640x480\n");
		}

		glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;
	} else if (!R_GetModeInfo(&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode)) {
		ri.Printf(PRINT_ALL, " invalid mode\n");
		return RSERR_INVALID_MODE;
	}

	ri.Printf(PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);
	// Center window
	if (r_centerWindow->integer && !fullscreen) {
		x = (desktopMode.w / 2) - (glConfig.vidWidth / 2);
		y = (desktopMode.h / 2) - (glConfig.vidHeight / 2);
	}
	// Destroy existing state if it exists
	if (SDL_glContext != NULL) {
		SDL_GL_DeleteContext(SDL_glContext);
		SDL_glContext = NULL;
	}

	if (SDL_window != NULL) {
		SDL_GetWindowPosition(SDL_window, &x, &y);
		ri.Printf(PRINT_DEVELOPER, "Existing window at %dx%d before being destroyed\n", x, y);
		SDL_DestroyWindow(SDL_window);
		SDL_window = NULL;
	}

	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
		glConfig.isFullscreen = qtrue;
	} else {
		if (noborder) {
			flags |= SDL_WINDOW_BORDERLESS;
		}

		glConfig.isFullscreen = qfalse;
	}

	colorBits = r_colorbits->value;

	if ((!colorBits) || (colorBits >= 32)) {
		colorBits = 24;
	}

	if (!r_depthbits->value) {
		depthBits = 24;
	} else {
		depthBits = r_depthbits->value;
	}

	stencilBits = r_stencilbits->value;
	samples = r_ext_multisample->value;

	for (i = 0; i < 16; i++) {
		int testColorBits, testDepthBits, testStencilBits;

		// 0 - default
		// 1 - minus colorBits
		// 2 - minus depthBits
		// 3 - minus stencil
		if ((i % 4) == 0 && i) {
			// one pass, reduce
			switch (i / 4) {
				case 2 :
					if (colorBits == 24) {
						colorBits = 16;
					}

					break;
				case 1 :
					if (depthBits == 24) {
						depthBits = 16;
					} else if (depthBits == 16) {
						depthBits = 8;
					}

				case 3 :
					if (stencilBits == 24) {
						stencilBits = 16;
					} else if (stencilBits == 16) {
						stencilBits = 8;
					}
			}
		}

		testColorBits = colorBits;
		testDepthBits = depthBits;
		testStencilBits = stencilBits;

		if ((i % 4) == 3) { // reduce colorBits
			if (testColorBits == 24) {
				testColorBits = 16;
			}
		}

		if ((i % 4) == 2) { // reduce depthBits
			if (testDepthBits == 24) {
				testDepthBits = 16;
			} else if (testDepthBits == 16) {
				testDepthBits = 8;
			}
		}

		if ((i % 4) == 1) { // reduce stencilBits
			if (testStencilBits == 24) {
				testStencilBits = 16;
			} else if (testStencilBits == 16) {
				testStencilBits = 8;
			} else {
				testStencilBits = 0;
			}
		}

		if (testColorBits == 24) {
			perChannelColorBits = 8;
		} else {
			perChannelColorBits = 4;
		}
#ifdef __sgi // Fix for SGIs grabbing too many bits of color
		if (perChannelColorBits == 4) {
			perChannelColorBits = 0; // Use minimum size for 16-bit color
		}
		// Need alpha or else SGIs choose 36+ bit RGB mode
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
#endif
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, perChannelColorBits);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, testDepthBits);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, testStencilBits);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples ? 1 : 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

		if (r_stereoEnabled->integer) {
			glConfig.stereoEnabled = qtrue;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 1);
		} else {
			glConfig.stereoEnabled = qfalse;
			SDL_GL_SetAttribute(SDL_GL_STEREO, 0);
		}

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if 0 // if multisampling is enabled on X11, this causes create window to fail.
		// If not allowing software GL, demand accelerated
		if (!r_allowSoftwareGL->integer) {
			SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		}
#endif
		if ((SDL_window = SDL_CreateWindow(windowTitle, x, y, glConfig.vidWidth, glConfig.vidHeight, flags)) == NULL) {
			ri.Printf(PRINT_DEVELOPER, "SDL_CreateWindow failed: %s\n", SDL_GetError());
			continue;
		}

		if (fullscreen) {
			SDL_DisplayMode mode;

			switch (testColorBits) {
				case 16:
					mode.format = SDL_PIXELFORMAT_RGB565;
					break;
				case 24:
					mode.format = SDL_PIXELFORMAT_RGB24;
					break;
				default:
					ri.Printf(PRINT_DEVELOPER, "testColorBits is %d, can't fullscreen\n", testColorBits);
					continue;
			}

			mode.w = glConfig.vidWidth;
			mode.h = glConfig.vidHeight;
			mode.refresh_rate = glConfig.displayFrequency = ri.Cvar_VariableIntegerValue("r_displayRefresh");
			mode.driverdata = NULL;

			if (SDL_SetWindowDisplayMode(SDL_window, &mode) < 0) {
				ri.Printf(PRINT_DEVELOPER, "SDL_SetWindowDisplayMode failed: %s\n", SDL_GetError());
				continue;
			}
		}

		SDL_SetWindowIcon(SDL_window, icon);
		// limit window minimum size to 320x200 unless a smaller size was specified
		SDL_SetWindowMinimumSize(SDL_window, MIN(320, glConfig.vidWidth), MIN(200, glConfig.vidHeight));

		if ((SDL_glContext = SDL_GL_CreateContext(SDL_window)) == NULL) {
			ri.Printf(PRINT_DEVELOPER, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
			continue;
		}

		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(SDL_window);

		if (SDL_GL_SetSwapInterval(r_swapInterval->integer) == -1) {
			ri.Printf(PRINT_DEVELOPER, "SDL_GL_SetSwapInterval failed: %s\n", SDL_GetError());
		}

		glConfig.colorBits = testColorBits;
		glConfig.depthBits = testDepthBits;
		glConfig.stencilBits = testStencilBits;

		ri.Printf(PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits);
		break;
	}

	SDL_FreeSurface(icon);

	if (iconPic) {
		ri.Free(iconPic);
		iconPic = NULL;
	}

	if (!SDL_window) {
		ri.Printf(PRINT_ALL, "Couldn't get a visual\n");
		return RSERR_INVALID_MODE;
	}

	GLimp_DetectAvailableModes();

	glstring = (char *)qglGetString(GL_RENDERER);
	ri.Printf(PRINT_ALL, "GL_RENDERER: %s\n", glstring);

	return RSERR_OK;
}

/*
=======================================================================================================================================
GLimp_StartDriverAndSetMode
=======================================================================================================================================
*/
static qboolean GLimp_StartDriverAndSetMode(int mode, qboolean fullscreen, qboolean noborder) {
	rserr_t err;

	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		const char *driverName;

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			ri.Printf(PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO) FAILED (%s)\n", SDL_GetError());
			return qfalse;
		}

		driverName = SDL_GetCurrentVideoDriver();
		ri.Printf(PRINT_ALL, "SDL using driver \"%s\"\n", driverName);
		ri.Cvar_Set("r_sdlDriver", driverName);
	}

	if (fullscreen && ri.Cvar_VariableIntegerValue("in_nograb")) {
		ri.Printf(PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
		ri.Cvar_Set("r_fullscreen", "0");
		r_fullscreen->modified = qfalse;
		fullscreen = qfalse;
	}

	err = GLimp_SetMode(mode, fullscreen, noborder);

	switch (err) {
		case RSERR_INVALID_FULLSCREEN:
			ri.Printf(PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n");
			return qfalse;
		case RSERR_INVALID_MODE:
			ri.Printf(PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode);
			return qfalse;
		default:
			break;
	}

	return qtrue;
}

/*
=======================================================================================================================================
GLimp_ResizeWindow

Window has been resized, update glconfig
=======================================================================================================================================
*/
qboolean GLimp_ResizeWindow(int width, int height) {

	glConfig.vidWidth = width;
	glConfig.vidHeight = height;
	glConfig.windowAspect = (float)glConfig.vidWidth / (float)glConfig.vidHeight;

	ri.CL_GlconfigChanged(&glConfig);
	return qtrue;
}

/*
=======================================================================================================================================
GLimp_HaveExtension
=======================================================================================================================================
*/
static qboolean GLimp_HaveExtension(const char *ext) {
	const char *ptr = Q_stristr(glConfig.extensions_string, ext);

	if (ptr == NULL) {
		return qfalse;
	}

	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0')); // verify it's complete string.
}

/*
=======================================================================================================================================
GLimp_InitExtensions
=======================================================================================================================================
*/
static void GLimp_InitExtensions(void) {

	if (!r_allowExtensions->integer) {
		ri.Printf(PRINT_ALL, "* IGNORING OPENGL EXTENSIONS *\n");
		return;
	}

	ri.Printf(PRINT_ALL, "Initializing OpenGL extensions\n");

	glConfig.textureCompression = TC_NONE;
	qglCompressedTexImage2DARB = NULL;
	// GL_EXT_texture_compression_s3tc
	if (GLimp_HaveExtension("GL_ARB_texture_compression") && GLimp_HaveExtension("GL_EXT_texture_compression_s3tc")) {
		// Compressed DDS image uploading requires this
		qglCompressedTexImage2DARB = SDL_GL_GetProcAddress("glCompressedTexImage2DARB");

		if (r_ext_compressed_textures->value) {
			glConfig.textureCompression = TC_S3TC_ARB;
			ri.Printf(PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n");
		} else {
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n");
		}
	} else {
		ri.Printf(PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n");
	}
	// GL_S3_s3tc ... legacy extension before GL_EXT_texture_compression_s3tc.
	if (glConfig.textureCompression == TC_NONE) {
		if (GLimp_HaveExtension("GL_S3_s3tc")) {
			if (r_ext_compressed_textures->value) {
				glConfig.textureCompression = TC_S3TC;
				ri.Printf(PRINT_ALL, "...using GL_S3_s3tc\n");
			} else {
				ri.Printf(PRINT_ALL, "...ignoring GL_S3_s3tc\n");
			}
		} else {
			ri.Printf(PRINT_ALL, "...GL_S3_s3tc not found\n");
		}
	}
	// GL_EXT_texture_env_add
	glConfig.textureEnvAddAvailable = qfalse;

	if (GLimp_HaveExtension("EXT_texture_env_add")) {
		if (r_ext_texture_env_add->integer) {
			glConfig.textureEnvAddAvailable = qtrue;
			ri.Printf(PRINT_ALL, "...using GL_EXT_texture_env_add\n");
		} else {
			glConfig.textureEnvAddAvailable = qfalse;
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n");
		}
	} else {
		ri.Printf(PRINT_ALL, "...GL_EXT_texture_env_add not found\n");
	}
	// GL_ARB_multitexture
	qglMultiTexCoord2fARB = NULL;
	qglActiveTextureARB = NULL;
	qglClientActiveTextureARB = NULL;

	if (GLimp_HaveExtension("GL_ARB_multitexture")) {
		if (r_ext_multitexture->value) {
			qglMultiTexCoord2fARB = SDL_GL_GetProcAddress("glMultiTexCoord2fARB");
			qglActiveTextureARB = SDL_GL_GetProcAddress("glActiveTextureARB");
			qglClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");

			if (qglActiveTextureARB) {
				GLint glint = 0;
				qglGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &glint);
				glConfig.numTextureUnits = (int)glint;

				if (glConfig.numTextureUnits > 1) {
					ri.Printf(PRINT_ALL, "...using GL_ARB_multitexture\n");
				} else {
					qglMultiTexCoord2fARB = NULL;
					qglActiveTextureARB = NULL;
					qglClientActiveTextureARB = NULL;
					ri.Printf(PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n");
				}
			}
		} else {
			ri.Printf(PRINT_ALL, "...ignoring GL_ARB_multitexture\n");
		}
	} else {
		ri.Printf(PRINT_ALL, "...GL_ARB_multitexture not found\n");
	}
	// GL_EXT_compiled_vertex_array
	if (GLimp_HaveExtension("GL_EXT_compiled_vertex_array")) {
		if (r_ext_compiled_vertex_array->value) {
			ri.Printf(PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n");
			qglLockArraysEXT = (void (APIENTRY *)(GLint, GLint))SDL_GL_GetProcAddress("glLockArraysEXT");
			qglUnlockArraysEXT = (void (APIENTRY *)(void))SDL_GL_GetProcAddress("glUnlockArraysEXT");

			if (!qglLockArraysEXT || !qglUnlockArraysEXT) {
				ri.Error(ERR_FATAL, "bad getprocaddress");
			}
		} else {
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n");
		}
	} else {
		ri.Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n");
	}

	glConfig.textureFilterAnisotropic = qfalse;

	if (GLimp_HaveExtension("GL_EXT_texture_filter_anisotropic")) {
		if (r_ext_texture_filter_anisotropic->integer) {
			qglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, (GLint *)&glConfig.maxAnisotropy);

			if (glConfig.maxAnisotropy <= 0) {
				ri.Printf(PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not properly supported!\n");
				glConfig.maxAnisotropy = 0;
			} else {
				ri.Printf(PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic(max: %i)\n", glConfig.maxAnisotropy);
				glConfig.textureFilterAnisotropic = qtrue;
			}
		} else {
			ri.Printf(PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n");
		}
	} else {
		ri.Printf(PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n");
	}
}

#define R_MODE_FALLBACK 3 // 640 * 480

/*
=======================================================================================================================================
GLimp_Init

This routine is responsible for initializing the OS specific portions of OpenGL.
=======================================================================================================================================
*/
void GLimp_Init(void) {

	ri.Printf(PRINT_DEVELOPER, "Glimp_Init()\n");

	r_allowSoftwareGL = ri.Cvar_Get("r_allowSoftwareGL", "0", CVAR_LATCH);
	r_sdlDriver = ri.Cvar_Get("r_sdlDriver", "", CVAR_ROM);
	r_allowResize = ri.Cvar_Get("r_allowResize", "1", CVAR_ARCHIVE|CVAR_LATCH);
	r_centerWindow = ri.Cvar_Get("r_centerWindow", "0", CVAR_ARCHIVE|CVAR_LATCH);
#ifdef _WIN32
	r_forceWindowIcon32 = ri.Cvar_Get("r_forceWindowIcon32", "1", CVAR_LATCH);
#else
	r_forceWindowIcon32 = ri.Cvar_Get("r_forceWindowIcon32", "0", CVAR_LATCH);
#endif
	if (ri.Cvar_VariableIntegerValue("com_abnormalExit")) {
		ri.Cvar_Set("r_mode", va("%d", R_MODE_FALLBACK));
		ri.Cvar_Set("r_fullscreen", "0");
		ri.Cvar_Set("r_centerWindow", "0");
		ri.Cvar_Set("com_abnormalExit", "0");
	}

	ri.Sys_GLimpInit();
	// Create the window and set up the context
	if (GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, r_noborder->integer)) {
		goto success;
	}
	// Try again, this time in a platform specific "safe mode"
	ri.Sys_GLimpSafeInit();

	if (GLimp_StartDriverAndSetMode(r_mode->integer, r_fullscreen->integer, qfalse)) {
		goto success;
	}
	// Finally, try the default screen resolution
	if (r_mode->integer != R_MODE_FALLBACK) {
		ri.Printf(PRINT_ALL, "Setting r_mode %d failed, falling back on r_mode %d\n", r_mode->integer, R_MODE_FALLBACK);

		if (GLimp_StartDriverAndSetMode(R_MODE_FALLBACK, qfalse, qfalse)) {
			goto success;
		}
	}
	// Nothing worked, give up
	ri.Error(ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem");
success:
	// Only using SDL_SetWindowBrightness to determine if hardware gamma is supported
	glConfig.deviceSupportsGamma = !r_ignorehwgamma->integer && SDL_SetWindowBrightness(SDL_window, 1.0f) >= 0;
	// get our config strings
	Q_strncpyz(glConfig.vendor_string, (char *)qglGetString(GL_VENDOR), sizeof(glConfig.vendor_string));
	Q_strncpyz(glConfig.renderer_string, (char *)qglGetString(GL_RENDERER), sizeof(glConfig.renderer_string));

	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n') {
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	}

	Q_strncpyz(glConfig.version_string, (char *)qglGetString(GL_VERSION), sizeof(glConfig.version_string));
	Q_strncpyz(glConfig.extensions_string, (char *)qglGetString(GL_EXTENSIONS), sizeof(glConfig.extensions_string));
	// initialize extensions
	GLimp_InitExtensions();

	ri.Cvar_Get("r_availableModes", "", CVAR_ROM);
	// This depends on SDL_INIT_VIDEO, hence having it here
	ri.IN_Init(SDL_window);
}

/*
=======================================================================================================================================
GLimp_EndFrame

Responsible for doing a swapbuffers.
=======================================================================================================================================
*/
void GLimp_EndFrame(void) {

	// don't flip if drawing to front buffer
	if (Q_stricmp(r_drawBuffer->string, "GL_FRONT") != 0) {
		SDL_GL_SwapWindow(SDL_window);
	}

	if (r_fullscreen->modified) {
		int fullscreen;
		qboolean needToToggle;
		qboolean sdlToggled = qfalse;

		// Find out the current state
		fullscreen = !!(SDL_GetWindowFlags(SDL_window) & SDL_WINDOW_FULLSCREEN);

		if (r_fullscreen->integer && ri.Cvar_VariableIntegerValue("in_nograb")) {
			ri.Printf(PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
			ri.Cvar_Set("r_fullscreen", "0");
			r_fullscreen->modified = qfalse;
		}
		// Is the state we want different from the current state?
		needToToggle = !!r_fullscreen->integer != fullscreen;

		if (needToToggle) {
			sdlToggled = SDL_SetWindowFullscreen(SDL_window, r_fullscreen->integer) >= 0;
			// SDL_WM_ToggleFullScreen didn't work, so do it the slow way
			if (!sdlToggled) {
				ri.Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
			} else {
				glConfig.isFullscreen = !!r_fullscreen->integer;
				ri.CL_GlconfigChanged(&glConfig);
			}

			ri.IN_Restart();
		}

		r_fullscreen->modified = qfalse;
	}
}
