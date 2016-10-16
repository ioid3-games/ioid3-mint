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

/**************************************************************************************************************************************
 Mumble link interface.
**************************************************************************************************************************************/

#ifdef WIN32
#include <windows.h>
#define uint32_t UINT32
#else
#include <unistd.h>
#ifdef __sun
#define _POSIX_C_SOURCE 199309L
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "libmumblelink.h"
#ifndef MIN
#define MIN (a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct {
	uint32_t uiVersion;
	uint32_t uiTick;
	float fAvatarPosition[3];
	float fAvatarFront[3];
	float fAvatarTop[3];
	wchar_t name[256];
	// new in mumble 1.2
	float fCameraPosition[3];
	float fCameraFront[3];
	float fCameraTop[3];
	wchar_t identity[256];
	uint32_t context_len;
	unsigned char context[256];
	wchar_t description[2048];
} LinkedMem;

static LinkedMem *lm = NULL;
#ifdef WIN32
static HANDLE hMapObject = NULL;
#else
/*
=======================================================================================================================================
GetTickCount
=======================================================================================================================================
*/
static int32_t GetTickCount(void) {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_usec / 1000 + tv.tv_sec * 1000;
}
#endif
/*
=======================================================================================================================================
mumble_link
=======================================================================================================================================
*/
int mumble_link(const char *name) {
#ifdef WIN32
	if (lm) {
		return 0;
	}

	hMapObject = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");

	if (hMapObject == NULL) {
		return -1;
	}

	lm = (LinkedMem *)MapViewOfFile(hMapObject, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));

	if (lm == NULL) {
		CloseHandle(hMapObject);
		hMapObject = NULL;
		return -1;
	}
#else
	char file[256];
	int shmfd;

	if (lm) {
		return 0;
	}

	snprintf(file, sizeof(file), "/MumbleLink.%d", getuid());
	shmfd = shm_open(file, O_RDWR, S_IRUSR|S_IWUSR);

	if (shmfd < 0) {
		return -1;
	}

	lm = (LinkedMem *)(mmap(NULL, sizeof(LinkedMem), PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0));

	if (lm == (void *)(-1)) {
		lm = NULL;
		close(shmfd);
		return -1;
	}

	close(shmfd);
#endif
	memset(lm, 0, sizeof(LinkedMem));
	mbstowcs(lm->name, name, sizeof(lm->name) / sizeof(wchar_t));

	return 0;
}

/*
=======================================================================================================================================
mumble_update_coordinates
=======================================================================================================================================
*/
void mumble_update_coordinates(float fPosition[3], float fFront[3], float fTop[3]) {
	mumble_update_coordinates2(fPosition, fFront, fTop, fPosition, fFront, fTop);
}

/*
=======================================================================================================================================
mumble_update_coordinates2
=======================================================================================================================================
*/
void mumble_update_coordinates2(float fAvatarPosition[3], float fAvatarFront[3], float fAvatarTop[3], float fCameraPosition[3], float fCameraFront[3], float fCameraTop[3]) {

	if (!lm) {
		return;
	}

	memcpy(lm->fAvatarPosition, fAvatarPosition, sizeof(lm->fAvatarPosition));
	memcpy(lm->fAvatarFront, fAvatarFront, sizeof(lm->fAvatarFront));
	memcpy(lm->fAvatarTop, fAvatarTop, sizeof(lm->fAvatarTop));
	memcpy(lm->fCameraPosition, fCameraPosition, sizeof(lm->fCameraPosition));
	memcpy(lm->fCameraFront, fCameraFront, sizeof(lm->fCameraFront));
	memcpy(lm->fCameraTop, fCameraTop, sizeof(lm->fCameraTop));

	lm->uiVersion = 2;
	lm->uiTick = GetTickCount();
}

/*
=======================================================================================================================================
mumble_set_identity
=======================================================================================================================================
*/
void mumble_set_identity(const char *identity) {
	size_t len;

	if (!lm) {
		return;
	}

	len = MIN(sizeof(lm->identity) / sizeof(wchar_t), strlen(identity) + 1);
	mbstowcs(lm->identity, identity, len);
}

/*
=======================================================================================================================================
mumble_set_context
=======================================================================================================================================
*/
void mumble_set_context(const unsigned char *context, size_t len) {

	if (!lm) {
		return;
	}

	len = MIN(sizeof(lm->context), len);
	lm->context_len = len;

	memcpy(lm->context, context, len);
}

/*
=======================================================================================================================================
mumble_set_description
=======================================================================================================================================
*/
void mumble_set_description(const char *description) {
	size_t len;

	if (!lm) {
		return;
	}

	len = MIN(sizeof(lm->description) / sizeof(wchar_t), strlen(description) + 1);
	mbstowcs(lm->description, description, len);
}

/*
=======================================================================================================================================
mumble_unlink
=======================================================================================================================================
*/
void mumble_unlink() {

	if (!lm) {
		return;
	}
#ifdef WIN32
	UnmapViewOfFile(lm);
	CloseHandle(hMapObject);
	hMapObject = NULL;
#else
	munmap(lm, sizeof(LinkedMem));
#endif
	lm = NULL;
}

int mumble_islinked(void) {
	return lm != NULL;
}
