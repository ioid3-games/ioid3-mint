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
//
#include "g_local.h"

#ifndef Q3_VM
static intptr_t(QDECL *syscall)(intptr_t arg, ...) = (intptr_t(QDECL *)(intptr_t, ...)) - 1;
/*
=======================================================================================================================================
dllEntry
=======================================================================================================================================
*/
Q_EXPORT void dllEntry(intptr_t(QDECL *syscallptr)(intptr_t arg, ...)) {
	syscall = syscallptr;
}
#endif
/*
=======================================================================================================================================
PASSFLOAT
=======================================================================================================================================
*/
int PASSFLOAT(float x) {
	floatint_t fi;

	fi.f = x;
	return fi.i;
}

/*
=======================================================================================================================================
trap_Print
=======================================================================================================================================
*/
void trap_Print(const char *text) {
	syscall(G_PRINT, text);
}

/*
=======================================================================================================================================
trap_Error
=======================================================================================================================================
*/
void trap_Error(const char *text) {
	syscall(G_ERROR, text);
#ifndef Q3_VM
	// shut up GCC warning about returning functions, because we know better
	exit(1);
#endif
}

/*
=======================================================================================================================================
trap_Milliseconds
=======================================================================================================================================
*/
int trap_Milliseconds(void) {
	return syscall(G_MILLISECONDS);
}

/*
=======================================================================================================================================
trap_Argc
=======================================================================================================================================
*/
int trap_Argc(void) {
	return syscall(G_ARGC);
}

/*
=======================================================================================================================================
trap_Argv
=======================================================================================================================================
*/
void trap_Argv(int n, char *buffer, int bufferLength) {
	syscall(G_ARGV, n, buffer, bufferLength);
}

/*
=======================================================================================================================================
trap_Args
=======================================================================================================================================
*/
void trap_Args(char *buffer, int bufferLength) {
	syscall(G_ARGS, buffer, bufferLength);
}

/*
=======================================================================================================================================
trap_LiteralArgs
=======================================================================================================================================
*/
void trap_LiteralArgs(char *buffer, int bufferLength) {
	syscall(G_LITERAL_ARGS, buffer, bufferLength);
}

/*
=======================================================================================================================================
trap_FS_FOpenFile
=======================================================================================================================================
*/
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
	return syscall(G_FS_FOPEN_FILE, qpath, f, mode);
}

/*
=======================================================================================================================================
trap_FS_Read
=======================================================================================================================================
*/
int trap_FS_Read(void *buffer, int len, fileHandle_t f) {
	return syscall(G_FS_READ, buffer, len, f);
}

/*
=======================================================================================================================================
trap_FS_Write
=======================================================================================================================================
*/
int trap_FS_Write(const void *buffer, int len, fileHandle_t f) {
	return syscall(G_FS_WRITE, buffer, len, f);
}

/*
=======================================================================================================================================
trap_FS_Seek
=======================================================================================================================================
*/
int trap_FS_Seek(fileHandle_t f, long offset, int origin) {
	return syscall(G_FS_SEEK, f, offset, origin);
}

/*
=======================================================================================================================================
trap_FS_Tell
=======================================================================================================================================
*/
int trap_FS_Tell(fileHandle_t f) {
	return syscall(G_FS_TELL, f);
}

/*
=======================================================================================================================================
trap_FS_FCloseFile
=======================================================================================================================================
*/
void trap_FS_FCloseFile(fileHandle_t f) {
	syscall(G_FS_FCLOSE_FILE, f);
}

/*
=======================================================================================================================================
trap_FS_GetFileList
=======================================================================================================================================
*/
int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize) {
	return syscall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

/*
=======================================================================================================================================
trap_FS_Delete
=======================================================================================================================================
*/
int trap_FS_Delete(const char *path) {
	return syscall(G_FS_DELETE, path);
}

/*
=======================================================================================================================================
trap_FS_Rename
=======================================================================================================================================
*/
int trap_FS_Rename(const char *from, const char *to) {
	return syscall(G_FS_RENAME, from, to);
}

/*
=======================================================================================================================================
trap_Cmd_ExecuteText
=======================================================================================================================================
*/
void trap_Cmd_ExecuteText(int exec_when, const char *text) {
	syscall(G_CMD_EXECUTETEXT, exec_when, text);
}

/*
=======================================================================================================================================
trap_Cvar_Register
=======================================================================================================================================
*/
void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags) {
	syscall(G_CVAR_REGISTER, cvar, var_name, value, flags);
}

/*
=======================================================================================================================================
trap_Cvar_Update
=======================================================================================================================================
*/
void trap_Cvar_Update(vmCvar_t *cvar) {
	syscall(G_CVAR_UPDATE, cvar);
}

/*
=======================================================================================================================================
trap_Cvar_Set
=======================================================================================================================================
*/
void trap_Cvar_Set(const char *var_name, const char *value) {
	syscall(G_CVAR_SET, var_name, value);
}

/*
=======================================================================================================================================
trap_Cvar_SetValue
=======================================================================================================================================
*/
void trap_Cvar_SetValue(const char *var_name, float value) {
	syscall(G_CVAR_SET_VALUE, var_name, PASSFLOAT(value));
}

/*
=======================================================================================================================================
trap_Cvar_VariableValue
=======================================================================================================================================
*/
float trap_Cvar_VariableValue(const char *var_name) {
	floatint_t fi;

	fi.i = syscall(G_CVAR_VARIABLE_VALUE, var_name);
	return fi.f;
}

/*
=======================================================================================================================================
trap_Cvar_VariableIntegerValue
=======================================================================================================================================
*/
int trap_Cvar_VariableIntegerValue(const char *var_name) {
	return syscall(G_CVAR_VARIABLE_INTEGER_VALUE, var_name);
}

/*
=======================================================================================================================================
trap_Cvar_VariableStringBuffer
=======================================================================================================================================
*/
void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	syscall(G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

/*
=======================================================================================================================================
trap_Cvar_LatchedVariableStringBuffer
=======================================================================================================================================
*/
void trap_Cvar_LatchedVariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	syscall(G_CVAR_LATCHED_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

/*
=======================================================================================================================================
trap_Cvar_DefaultVariableStringBuffer
=======================================================================================================================================
*/
void trap_Cvar_DefaultVariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	syscall(G_CVAR_DEFAULT_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

/*
=======================================================================================================================================
trap_Cvar_InfoStringBuffer
=======================================================================================================================================
*/
void trap_Cvar_InfoStringBuffer(int bit, char *buffer, int bufsize) {
	syscall(G_CVAR_INFO_STRING_BUFFER, bit, buffer, bufsize);
}

/*
=======================================================================================================================================
trap_Cvar_CheckRange
=======================================================================================================================================
*/
void trap_Cvar_CheckRange(const char *var_name, float min, float max, qboolean integral) {
	syscall(G_CVAR_CHECK_RANGE, var_name, PASSFLOAT(min), PASSFLOAT(max), integral);
}

/*
=======================================================================================================================================
trap_LocateGameData
=======================================================================================================================================
*/
void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *players, int sizeofGamePlayer) {
	syscall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, players, sizeofGamePlayer);
}

/*
=======================================================================================================================================
trap_SetNetFields
=======================================================================================================================================
*/
void trap_SetNetFields(int entityStateSize, int entityNetworkSize, vmNetField_t *entityStateFields, int numEntityStateFields, int playerStateSize, int playerNetworkSize, vmNetField_t *playerStateFields, int numPlayerStateFields) {
	syscall(G_SET_NET_FIELDS, entityStateSize, entityNetworkSize, entityStateFields, numEntityStateFields, playerStateSize, playerNetworkSize, playerStateFields, numPlayerStateFields);
}

/*
=======================================================================================================================================
trap_DropPlayer
=======================================================================================================================================
*/
void trap_DropPlayer(int playerNum, const char *reason) {
	syscall(G_DROP_PLAYER, playerNum, reason);
}

/*
=======================================================================================================================================
trap_SendServerCommandEx
=======================================================================================================================================
*/
void trap_SendServerCommandEx(int clientNum, int localPlayerNum, const char *text) {
	syscall(G_SEND_SERVER_COMMAND, clientNum, localPlayerNum, text);
}

/*
=======================================================================================================================================
trap_SetConfigstring
=======================================================================================================================================
*/
void trap_SetConfigstring(int num, const char *string) {
	syscall(G_SET_CONFIGSTRING, num, string);
}

/*
=======================================================================================================================================
trap_GetConfigstring
=======================================================================================================================================
*/
void trap_GetConfigstring(int num, char *buffer, int bufferSize) {
	syscall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}

/*
=======================================================================================================================================
trap_SetConfigstringRestrictions
=======================================================================================================================================
*/
void trap_SetConfigstringRestrictions(int num, const clientList_t *clientList) {
	syscall(G_SET_CONFIGSTRING_RESTRICTIONS, num, clientList);
}

/*
=======================================================================================================================================
trap_GetUserinfo
=======================================================================================================================================
*/
void trap_GetUserinfo(int num, char *buffer, int bufferSize) {
	syscall(G_GET_USERINFO, num, buffer, bufferSize);
}

/*
=======================================================================================================================================
trap_SetUserinfo
=======================================================================================================================================
*/
void trap_SetUserinfo(int num, const char *buffer) {
	syscall(G_SET_USERINFO, num, buffer);
}

/*
=======================================================================================================================================
trap_GetServerinfo
=======================================================================================================================================
*/
void trap_GetServerinfo(char *buffer, int bufferSize) {
	syscall(G_GET_SERVERINFO, buffer, bufferSize);
}

/*
=======================================================================================================================================
trap_GetBrushBounds
=======================================================================================================================================
*/
void trap_GetBrushBounds(int modelindex, vec3_t mins, vec3_t maxs) {
	syscall(G_GET_BRUSH_BOUNDS, modelindex, mins, maxs);
}

/*
=======================================================================================================================================
trap_Trace
=======================================================================================================================================
*/
void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	syscall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/*
=======================================================================================================================================
trap_TraceCapsule
=======================================================================================================================================
*/
void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/*
=======================================================================================================================================
trap_ClipToEntities
=======================================================================================================================================
*/
void trap_ClipToEntities(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	syscall(G_CLIPTOENTITIES, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/*
=======================================================================================================================================
trap_ClipToEntitiesCapsule
=======================================================================================================================================
*/
void trap_ClipToEntitiesCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	syscall(G_CLIPTOENTITIESCAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

/*
=======================================================================================================================================
trap_PointContents
=======================================================================================================================================
*/
int trap_PointContents(const vec3_t point, int passEntityNum) {
	return syscall(G_POINT_CONTENTS, point, passEntityNum);
}

/*
=======================================================================================================================================
trap_InPVS
=======================================================================================================================================
*/
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2) {
	return syscall(G_IN_PVS, p1, p2);
}

/*
=======================================================================================================================================
trap_InPVSIgnorePortals
=======================================================================================================================================
*/
qboolean trap_InPVSIgnorePortals(const vec3_t p1, const vec3_t p2) {
	return syscall(G_IN_PVS_IGNORE_PORTALS, p1, p2);
}

/*
=======================================================================================================================================
trap_AdjustAreaPortalState
=======================================================================================================================================
*/
void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open) {
	syscall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}

/*
=======================================================================================================================================
trap_AreasConnected
=======================================================================================================================================
*/
qboolean trap_AreasConnected(int area1, int area2) {
	return syscall(G_AREAS_CONNECTED, area1, area2);
}

/*
=======================================================================================================================================
trap_LinkEntity
=======================================================================================================================================
*/
void trap_LinkEntity(gentity_t *ent) {
	syscall(G_LINKENTITY, ent);
}

/*
=======================================================================================================================================
trap_UnlinkEntity
=======================================================================================================================================
*/
void trap_UnlinkEntity(gentity_t *ent) {
	syscall(G_UNLINKENTITY, ent);
}

/*
=======================================================================================================================================
trap_EntitiesInBox
=======================================================================================================================================
*/
int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount) {
	return syscall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount);
}

/*
=======================================================================================================================================
trap_EntityContact
=======================================================================================================================================
*/
qboolean trap_EntityContact(const vec3_t mins, const vec3_t maxs, const gentity_t *ent) {
	return syscall(G_ENTITY_CONTACT, mins, maxs, ent);
}

/*
=======================================================================================================================================
trap_EntityContactCapsule
=======================================================================================================================================
*/
qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent) {
	return syscall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent);
}

/*
=======================================================================================================================================
trap_BotAllocateClient
=======================================================================================================================================
*/
int trap_BotAllocateClient(void) {
	return syscall(G_BOT_ALLOCATE_CLIENT);
}

/*
=======================================================================================================================================
trap_BotFreeClient
=======================================================================================================================================
*/
void trap_BotFreeClient(int playerNum) {
	syscall(G_BOT_FREE_CLIENT, playerNum);
}

/*
=======================================================================================================================================
trap_GetUsercmd
=======================================================================================================================================
*/
void trap_GetUsercmd(int playerNum, usercmd_t *cmd) {
	syscall(G_GET_USERCMD, playerNum, cmd);
}

/*
=======================================================================================================================================
trap_GetEntityToken
=======================================================================================================================================
*/
qboolean trap_GetEntityToken(int *parseOffset, char *buffer, int bufferSize) {
	return syscall(G_GET_ENTITY_TOKEN, parseOffset, buffer, bufferSize);
}

/*
=======================================================================================================================================
trap_DebugPolygonCreate
=======================================================================================================================================
*/
int trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points) {
	return syscall(G_DEBUG_POLYGON_CREATE, color, numPoints, points);
}

/*
=======================================================================================================================================
trap_DebugPolygonShow
=======================================================================================================================================
*/
void trap_DebugPolygonShow(int id, int color, int numPoints, vec3_t *points) {
	syscall(G_DEBUG_POLYGON_SHOW, id, color, numPoints, points);
}

/*
=======================================================================================================================================
trap_DebugPolygonDelete
=======================================================================================================================================
*/
void trap_DebugPolygonDelete(int id) {
	syscall(G_DEBUG_POLYGON_DELETE, id);
}

/*
=======================================================================================================================================
trap_RealTime
=======================================================================================================================================
*/
int trap_RealTime(qtime_t *qtime) {
	return syscall(G_REAL_TIME, qtime);
}

/*
=======================================================================================================================================
trap_SnapVector
=======================================================================================================================================
*/
void trap_SnapVector(float *v) {
	syscall(G_SNAPVECTOR, v);
}

/*
=======================================================================================================================================
trap_AddCommand
=======================================================================================================================================
*/
void trap_AddCommand(const char *cmdName) {
	syscall(G_ADDCOMMAND, cmdName);
}

/*
=======================================================================================================================================
trap_RemoveCommand
=======================================================================================================================================
*/
void trap_RemoveCommand(const char *cmdName) {
	syscall(G_REMOVECOMMAND, cmdName);
}

/*
=======================================================================================================================================
trap_R_RegisterModel
=======================================================================================================================================
*/
qhandle_t trap_R_RegisterModel(const char *name) {
	return syscall(G_R_REGISTERMODEL, name);
}

/*
=======================================================================================================================================
trap_R_LerpTag
=======================================================================================================================================
*/
int trap_R_LerpTag(orientation_t *tag, clipHandle_t handle, int startFrame, int endFrame, float frac, const char *tagName) {
	return syscall(G_R_LERPTAG, tag, handle, startFrame, endFrame, PASSFLOAT(frac), tagName);
}

/*
=======================================================================================================================================
trap_R_LerpTagFrameModel
=======================================================================================================================================
*/
int trap_R_LerpTagFrameModel(orientation_t *tag, clipHandle_t mod, clipHandle_t frameModel, int startFrame, clipHandle_t endFrameModel, int endFrame, float frac, const char *tagName, int *tagIndex) {
	return syscall(G_R_LERPTAG_FRAMEMODEL, tag, mod, frameModel, startFrame, endFrameModel, endFrame, PASSFLOAT(frac), tagName, tagIndex);
}

/*
=======================================================================================================================================
trap_R_LerpTagTorso
=======================================================================================================================================
*/
int trap_R_LerpTagTorso(orientation_t *tag, clipHandle_t mod, clipHandle_t frameModel, int startFrame, clipHandle_t endFrameModel, int endFrame, float frac, const char *tagName, int *tagIndex, const vec3_t *torsoAxis, qhandle_t torsoFrameModel, int torsoFrame, qhandle_t oldTorsoFrameModel, int oldTorsoFrame, float torsoFrac) {
	return syscall(G_R_LERPTAG_TORSO, tag, mod, frameModel, startFrame, endFrameModel, endFrame, PASSFLOAT(frac), tagName, tagIndex, torsoAxis, torsoFrameModel, torsoFrame, oldTorsoFrameModel, oldTorsoFrame, PASSFLOAT(torsoFrac));
}

/*
=======================================================================================================================================
trap_R_ModelBounds
=======================================================================================================================================
*/
int trap_R_ModelBounds(clipHandle_t handle, vec3_t mins, vec3_t maxs, int startFrame, int endFrame, float frac) {
	return syscall(G_R_MODELBOUNDS, handle, mins, maxs, startFrame, endFrame, PASSFLOAT(frac));
}

/*
=======================================================================================================================================
trap_ClientCommand
=======================================================================================================================================
*/
void trap_ClientCommand(int playerNum, const char *command) {
	syscall(G_CLIENT_COMMAND, playerNum, command);
}

/*
=======================================================================================================================================
trap_BotGetSnapshotEntity
=======================================================================================================================================
*/
int trap_BotGetSnapshotEntity(int playerNum, int sequence) {
	return syscall(G_BOT_GET_SNAPSHOT_ENTITY, playerNum, sequence);
}

/*
=======================================================================================================================================
trap_BotGetServerCommand
=======================================================================================================================================
*/
int trap_BotGetServerCommand(int playerNum, char *command, int size) {
	return syscall(G_BOT_GET_SERVER_COMMAND, playerNum, command, size);
}

/*
=======================================================================================================================================
trap_BotUserCommand
=======================================================================================================================================
*/
void trap_BotUserCommand(int playerNum, usercmd_t *ucmd) {
	syscall(G_BOT_USER_COMMAND, playerNum, ucmd);
}

/*
=======================================================================================================================================
trap_PC_AddGlobalDefine
=======================================================================================================================================
*/
int trap_PC_AddGlobalDefine(const char *define) {
	return syscall(G_PC_ADD_GLOBAL_DEFINE, define);
}

/*
=======================================================================================================================================
trap_PC_RemoveGlobalDefine
=======================================================================================================================================
*/
int trap_PC_RemoveGlobalDefine(const char *define) {
	return syscall(G_PC_REMOVE_GLOBAL_DEFINE, define);
}

/*
=======================================================================================================================================
trap_PC_RemoveAllGlobalDefines
=======================================================================================================================================
*/
void trap_PC_RemoveAllGlobalDefines(void) {
	syscall(G_PC_REMOVE_ALL_GLOBAL_DEFINES);
}

/*
=======================================================================================================================================
trap_PC_LoadSource
=======================================================================================================================================
*/
int trap_PC_LoadSource(const char *filename, const char *basepath) {
	return syscall(G_PC_LOAD_SOURCE, filename, basepath);
}

/*
=======================================================================================================================================
trap_PC_FreeSource
=======================================================================================================================================
*/
int trap_PC_FreeSource(int handle) {
	return syscall(G_PC_FREE_SOURCE, handle);
}

/*
=======================================================================================================================================
trap_PC_AddDefine
=======================================================================================================================================
*/
int trap_PC_AddDefine(int handle, const char *define) {
	return syscall(G_PC_ADD_DEFINE, handle, define);
}

/*
=======================================================================================================================================
trap_PC_ReadToken
=======================================================================================================================================
*/
int trap_PC_ReadToken(int handle, pc_token_t *pc_token) {
	return syscall(G_PC_READ_TOKEN, handle, pc_token);
}

/*
=======================================================================================================================================
trap_PC_UnreadToken
=======================================================================================================================================
*/
void trap_PC_UnreadToken(int handle) {
	syscall(G_PC_UNREAD_TOKEN, handle);
}

/*
=======================================================================================================================================
trap_PC_SourceFileAndLine
=======================================================================================================================================
*/
int trap_PC_SourceFileAndLine(int handle, char *filename, int *line) {
	return syscall(G_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

/*
=======================================================================================================================================
trap_HeapMalloc
=======================================================================================================================================
*/
void *trap_HeapMalloc(int size) {
	return(void *)syscall(G_HEAP_MALLOC, size);
}

/*
=======================================================================================================================================
trap_HeapAvailable
=======================================================================================================================================
*/
int trap_HeapAvailable(void) {
	return syscall(G_HEAP_AVAILABLE);
}

/*
=======================================================================================================================================
trap_HeapFree
=======================================================================================================================================
*/
void trap_HeapFree(void *data) {
	syscall(G_HEAP_FREE, data);
}

/*
=======================================================================================================================================
trap_Field_CompleteFilename
=======================================================================================================================================
*/
void trap_Field_CompleteFilename(const char *dir, const char *ext, qboolean stripExt, qboolean allowNonPureFilesOnDisk) {
	syscall(G_FIELD_COMPLETEFILENAME, dir, ext, stripExt, allowNonPureFilesOnDisk);
}

/*
=======================================================================================================================================
trap_Field_CompleteCommand
=======================================================================================================================================
*/
void trap_Field_CompleteCommand(const char *cmd, qboolean doCommands, qboolean doCvars) {
	syscall(G_FIELD_COMPLETECOMMAND, cmd, doCommands, doCvars);
}

/*
=======================================================================================================================================
trap_Field_CompleteList
=======================================================================================================================================
*/
void trap_Field_CompleteList(const char *list) {
	syscall(G_FIELD_COMPLETELIST, list);
}
