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

#include "g_local.h"

static int g_numBots;
static char *g_botInfos[MAX_BOTS];

static int g_numTeamBots;
static char *g_teamBotInfos[MAX_BOTS];

int g_numArenas;
static char *g_arenaInfos[MAX_ARENAS];

#define BOT_BEGIN_DELAY_BASE 2000
#define BOT_BEGIN_DELAY_INCREMENT 1500
#define BOT_SPAWN_QUEUE_DEPTH 16

typedef struct {
	int clientNum;
	int spawnTime;
} botSpawnQueue_t;

static botSpawnQueue_t botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern gentity_t *podium1;
extern gentity_t *podium2;
extern gentity_t *podium3;

/*
=======================================================================================================================================
G_ParseInfos
=======================================================================================================================================
*/
int G_ParseInfos(char *buf, int max, char *infos[]) {
	char *token;
	int count;
	char key[MAX_TOKEN_CHARS];
	char info[MAX_INFO_STRING];

	count = 0;

	while (1) {
		token = COM_Parse(&buf);

		if (!token[0]) {
			break;
		}

		if (strcmp(token, "{")) {
			Com_Printf("Missing { in info file\n");
			break;
		}

		if (count == max) {
			Com_Printf("Max infos exceeded\n");
			break;
		}

		info[0] = '\0';

		while (1) {
			token = COM_ParseExt(&buf, qtrue);

			if (!token[0]) {
				Com_Printf("Unexpected end of info file\n");
				break;
			}

			if (!strcmp(token, "}")) {
				break;
			}

			Q_strncpyz(key, token, sizeof(key));

			token = COM_ParseExt(&buf, qfalse);

			if (!token[0]) {
				strcpy(token, "<NULL>");
			}

			Info_SetValueForKey(info, key, token);
		}
		// NOTE: extra space for arena number
		infos[count] = trap_HeapMalloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);

		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}

	return count;
}

/*
=======================================================================================================================================
G_LoadArenasFromFile
=======================================================================================================================================
*/
static void G_LoadArenasFromFile(char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	if (len >= MAX_ARENAS_TEXT) {
		trap_FS_FCloseFile(f);
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_ARENAS_TEXT));
		return;
	}

	trap_FS_Read(buf, len, f);

	buf[len] = 0;

	trap_FS_FCloseFile(f);

	g_numArenas += G_ParseInfos(buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas]);
}

/*
=======================================================================================================================================
G_LoadArenas
=======================================================================================================================================
*/
static void G_LoadArenas(void) {
	int numdirs;
	vmCvar_t arenasFile;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int i, n;
	int dirlen;

	g_numArenas = 0;

	trap_Cvar_Register(&arenasFile, "g_arenasFile", "", CVAR_INIT|CVAR_ROM);

	if (*arenasFile.string) {
		G_LoadArenasFromFile(arenasFile.string);
	} else {
		G_LoadArenasFromFile("scripts/arenas.txt");
	}
	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr = dirlist;

	for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadArenasFromFile(filename);
	}

	G_DPrintf("%i arenas parsed\n", g_numArenas);

	for (n = 0; n < g_numArenas; n++) {
		Info_SetValueForKey(g_arenaInfos[n], "num", va("%i", n));
	}
}

/*
=======================================================================================================================================
G_GetArenaInfoByMap
=======================================================================================================================================
*/
const char *G_GetArenaInfoByMap(const char *map) {
	int n;

	for (n = 0; n < g_numArenas; n++) {
		if (Q_stricmp(Info_ValueForKey(g_arenaInfos[n], "map"), map) == 0) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

/*
=======================================================================================================================================
PlayerIntroSound
=======================================================================================================================================
*/
static void PlayerIntroSound(const char *modelAndSkin) {
	char model[MAX_QPATH];
	char *skin;

	Q_strncpyz(model, modelAndSkin, sizeof(model));

	skin = strrchr(model, '/');

	if (skin) {
		*skin++ = '\0';
	} else {
		skin = model;
	}

	if (Q_stricmp(skin, "default") == 0) {
		skin = model;
	}

	trap_Cmd_ExecuteText(EXEC_APPEND, va("play snd/v/%s.wav\n", skin));
}

/*
=======================================================================================================================================
G_CountBotPlayersByName

Check connected and connecting (delay join) bots. Returns number of bots with name on specified team or whole server if team is -1.
=======================================================================================================================================
*/
int G_CountBotPlayersByName(const char *name, int team) {
	int i, num;
	gclient_t *cl;

	num = 0;

	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;

		if (cl->pers.connected == CON_DISCONNECTED) {
			continue;
		}

		if (!(g_entities[i].r.svFlags & SVF_BOT)) {
			continue;
		}

		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}

		if (name && Q_stricmp(name, cl->pers.netname)) {
			continue;
		}

		num++;
	}

	return num;
}

/*
=======================================================================================================================================
G_SelectRandomBotInfo

Get random least used bot info on team or whole server if team is -1.
=======================================================================================================================================
*/
char *G_SelectRandomBotInfo(int team) {
	int selection[MAX_BOTS];
	int n, num;
	int count, bestCount;
	char *value;
	int numBots;
	char **botInfos;

	if (g_gametype.integer > GT_TOURNAMENT) {
		numBots = g_numTeamBots;
		botInfos = g_teamBotInfos;
	} else {
		numBots = g_numBots;
		botInfos = g_botInfos;
	}
	// don't add duplicate bots to the server if there are less bots than bot types
	if (team != -1 && G_CountBotPlayersByName(NULL, -1) < numBots) {
		team = -1;
	}

	num = 0;
	bestCount = MAX_CLIENTS;

	for (n = 0; n < numBots; n++) {
		value = Info_ValueForKey(botInfos[n], "funname");

		if (!value[0]) {
			value = Info_ValueForKey(botInfos[n], "name");
		}

		count = G_CountBotPlayersByName(value, team);

		if (count < bestCount) {
			bestCount = count;
			num = 0;
		}

		if (count == bestCount) {
			selection[num++] = n;

			if (num == MAX_BOTS) {
				break;
			}
		}
	}

	if (num > 0) {
		num = random() * (num - 1);
		return botInfos[selection[num]];
	}

	return NULL;
}

/*
=======================================================================================================================================
G_AddRandomBot
=======================================================================================================================================
*/
void G_AddRandomBot(int team) {
	char *teamstr;
	float skill;

	skill = trap_Cvar_VariableValue("g_spSkill");

	if (team == TEAM_RED) {
		teamstr = "red";
	} else if (team == TEAM_BLUE) {
		teamstr = "blue";
	} else {
		teamstr = "free";
	}

	trap_Cmd_ExecuteText(EXEC_INSERT, va("addbot random %f %s %i\n", skill, teamstr, 0));
}

/*
=======================================================================================================================================
G_RemoveRandomBot
=======================================================================================================================================
*/
int G_RemoveRandomBot(int team) {
	int i;
	gclient_t *cl;

	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;

		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}

		if (!(g_entities[i].r.svFlags & SVF_BOT)) {
			continue;
		}

		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}

		trap_Cmd_ExecuteText(EXEC_INSERT, va("kicknum %d\n", i));
		return qtrue;
	}

	return qfalse;
}

/*
=======================================================================================================================================
G_CountHumanPlayers
=======================================================================================================================================
*/
int G_CountHumanPlayers(int team) {
	int i, num;
	gclient_t *cl;

	num = 0;

	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;

		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}

		if (g_entities[i].r.svFlags & SVF_BOT) {
			continue;
		}

		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}

		num++;
	}

	return num;
}

/*
=======================================================================================================================================
G_CountBotPlayers

Check connected and connecting (delay join) bots.
=======================================================================================================================================
*/
int G_CountBotPlayers(int team) {
	int i, num;
	gclient_t *cl;

	num = 0;

	for (i = 0; i < g_maxclients.integer; i++) {
		cl = level.clients + i;

		if (cl->pers.connected == CON_DISCONNECTED) {
			continue;
		}

		if (!(g_entities[i].r.svFlags & SVF_BOT)) {
			continue;
		}

		if (team >= 0 && cl->sess.sessionTeam != team) {
			continue;
		}

		num++;
	}

	return num;
}

/*
=======================================================================================================================================
G_CheckMinimumPlayers
=======================================================================================================================================
*/
void G_CheckMinimumPlayers(void) {
	int minplayers;
	int humanplayers, botplayers;
	static int checkminimumplayers_time;

	if (level.intermissiontime) {
		return;
	}
	// only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}

	checkminimumplayers_time = level.time;

	trap_Cvar_Update(&bot_minplayers);

	minplayers = bot_minplayers.integer;

	if (minplayers <= 0) {
		return;
	}

	if (g_gametype.integer == GT_FFA) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer - 1;
		}

		humanplayers = G_CountHumanPlayers(TEAM_FREE);
		botplayers = G_CountBotPlayers(TEAM_FREE);

		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_FREE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_FREE);
		}
	} else if (g_gametype.integer == GT_TOURNAMENT) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer - 1;
		}

		humanplayers = G_CountHumanPlayers(-1);
		botplayers = G_CountBotPlayers(-1);

		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_FREE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot(TEAM_SPECTATOR)) {
				// just remove the bot that is playing
				G_RemoveRandomBot(-1);
			}
		}
	} else if (g_gametype.integer > GT_TOURNAMENT) {
		if (minplayers >= g_maxclients.integer / 2) {
			minplayers = (g_maxclients.integer / 2) - 1;
		}

		humanplayers = G_CountHumanPlayers(TEAM_RED);
		botplayers = G_CountBotPlayers(TEAM_RED);

		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_RED);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_RED);
		}

		humanplayers = G_CountHumanPlayers(TEAM_BLUE);
		botplayers = G_CountBotPlayers(TEAM_BLUE);

		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot(TEAM_BLUE);
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot(TEAM_BLUE);
		}
	}
}

/*
=======================================================================================================================================
G_CheckBotSpawn
=======================================================================================================================================
*/
void G_CheckBotSpawn(void) {
	int n;
	char userinfo[MAX_INFO_VALUE];

	G_CheckMinimumPlayers();

	for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
		if (!botSpawnQueue[n].spawnTime) {
			continue;
		}

		if (botSpawnQueue[n].spawnTime > level.time) {
			continue;
		}

		ClientBegin(botSpawnQueue[n].clientNum);

		botSpawnQueue[n].spawnTime = 0;

		if (g_gametype.integer == GT_SINGLE_PLAYER) {
			trap_GetUserinfo(botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo));
			PlayerIntroSound(Info_ValueForKey(userinfo, "model"));
		}
	}
}

/*
=======================================================================================================================================
AddBotToSpawnQueue
=======================================================================================================================================
*/
static void AddBotToSpawnQueue(int clientNum, int delay) {
	int n;

	for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
		if (!botSpawnQueue[n].spawnTime) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf(S_COLOR_YELLOW "Unable to delay spawn\n");
	ClientBegin(clientNum);
}

/*
=======================================================================================================================================
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn doesn't happen on a freed index.
=======================================================================================================================================
*/
void G_RemoveQueuedBotBegin(int clientNum) {
	int n;

	for (n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++) {
		if (botSpawnQueue[n].clientNum == clientNum) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}

/*
=======================================================================================================================================
G_BotConnect
=======================================================================================================================================
*/
qboolean G_BotConnect(int clientNum, qboolean restart) {
	bot_settings_t settings;
	char userinfo[MAX_INFO_STRING];

	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	Q_strncpyz(settings.characterfile, Info_ValueForKey(userinfo, "characterfile"), sizeof(settings.characterfile));

	settings.skill = atof(Info_ValueForKey(userinfo, "skill"));

	if (!BotAISetupClient(clientNum, &settings, restart)) {
		trap_DropClient(clientNum, "BotAISetupClient failed");
		return qfalse;
	}

	return qtrue;
}

/*
=======================================================================================================================================
G_DefaultColorForName
=======================================================================================================================================
*/
static int G_DefaultColorForName(const char *name) {
	int val;
	const char *p;

	p = name;
	val = 0;

	while (*p) {
		val += *p;
		p++;
	}
	// choose value in range 1-13
	return val % 13 + 1;
}

/*
=======================================================================================================================================
G_AddBot
=======================================================================================================================================
*/
static void G_AddBot(const char *name, float skill, const char *team, int delay, char *altname) {
	int value;
	int connectionNum;
	int clientNum;
	int teamNum;
	char *botinfo;
	char *key;
	char *s;
	char *botname;
	char *model;
	char *headmodel;
	char userinfo[MAX_INFO_STRING];
	qboolean modelSet;

	// have the server allocate a client slot
	value = trap_BotAllocateClient();
	if (value == -1) {
		G_Printf(S_COLOR_RED "Unable to add bot. All player slots are in use.\n");
		G_Printf(S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n");
		return;
	}
	// get connection and player numbers
	connectionNum = value & 0xFFFF;
	clientNum = value >> 16;
	// set default team
	if (!team || !*team) {
		if (g_gametype.integer > GT_TOURNAMENT) {
			if (PickTeam(clientNum) == TEAM_RED) {
				team = "red";
			} else {
				team = "blue";
			}
		} else {
			team = "free";
		}
	}
	// get the botinfo from bots.txt
	if (Q_stricmp(name, "random") == 0) {
		if (Q_stricmp(team, "red") == 0 || Q_stricmp(team, "r") == 0) {
			teamNum = TEAM_RED;
		} else if (Q_stricmp(team, "blue") == 0 || Q_stricmp(team, "b") == 0) {
			teamNum = TEAM_BLUE;
		} else if (!Q_stricmp(team, "spectator") || !Q_stricmp(team, "s")) {
			teamNum = TEAM_SPECTATOR;
		} else {
			teamNum = TEAM_FREE;
		}

		botinfo = G_SelectRandomBotInfo(teamNum);

		if (!botinfo) {
			G_Printf(S_COLOR_RED "Error: Cannot add random bot, no bot info available.\n");
			trap_BotFreeClient(clientNum);
			return;
		}
	} else {
		// get info of the bot
		botinfo = G_GetBotInfoByName(name);
	}

	if (!botinfo) {
		G_Printf(S_COLOR_RED "Error: Bot '%s' not defined\n", name);
		trap_BotFreeClient(clientNum);
		return;
	}
	// create the bot's userinfo
	userinfo[0] = '\0';
	// botname
	botname = Info_ValueForKey(botinfo, "funname");

	if (!botname[0]) {
		botname = Info_ValueForKey(botinfo, "name");
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}

	Info_SetValueForKey(userinfo, "name", botname);
	Info_SetValueForKey(userinfo, "rate", "25000");
	Info_SetValueForKey(userinfo, "snaps", "60");
	Info_SetValueForKey(userinfo, "skill", va("%.2g", skill));
	Info_SetValueForKey(userinfo, "teampref", team);
	// model
	key = "model";
	model = Info_ValueForKey(botinfo, key);
	modelSet = (*model);

	if (!modelSet) {
		model = DEFAULT_MODEL;
	}

	Info_SetValueForKey(userinfo, key, model);
	// team model
	key = "team_model";
	Info_SetValueForKey(userinfo, key, model);
	// head model
	key = "headmodel";
	headmodel = Info_ValueForKey(botinfo, key);

	if (!*headmodel) {
		if (!modelSet) {
			headmodel = DEFAULT_HEAD;
		} else {
			headmodel = model;
		}
	}

	Info_SetValueForKey(userinfo, key, headmodel);
	// team head model
	key = "team_headmodel";
	Info_SetValueForKey(userinfo, key, headmodel);

	// color1
	key = "color1";
	s = Info_ValueForKey(botinfo, key);

	if (!*s) {
		s = va("%d", G_DefaultColorForName(botname));
	}

	Info_SetValueForKey(userinfo, key, s);
	// color2
	key = "color2";
	s = Info_ValueForKey(botinfo, key);

	if (!*s) {
		s = va("%d", G_DefaultColorForName(botname + strlen(botname) / 2));
	}

	Info_SetValueForKey(userinfo, key, s);
	// ai character
	s = Info_ValueForKey(botinfo, "aifile");

	if (!*s) {
		trap_Print(S_COLOR_RED "Error: bot has no aifile specified\n");
		trap_BotFreeClient(clientNum);
		return;
	}

	Info_SetValueForKey(userinfo, "characterfile", s);
	// don't send tinfo to bots, they don't parse it
	Info_SetValueForKey(userinfo, "teamoverlay", "0");
	// register the userinfo
	trap_SetUserinfo(clientNum, userinfo);
	// have it connect to the game as a normal client
	if (ClientConnect(clientNum, qtrue, qtrue, connectionNum, 0)) {
		return;
	}

	if (delay == 0) {
		ClientBegin(clientNum);
		return;
	}

	AddBotToSpawnQueue(clientNum, delay);
}

/*
=======================================================================================================================================
Svcmd_AddBot_f
=======================================================================================================================================
*/
void Svcmd_AddBot_f(void) {
	float skill;
	int delay;
	char name[MAX_TOKEN_CHARS];
	char altname[MAX_TOKEN_CHARS];
	char string[MAX_TOKEN_CHARS];
	char team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if (!trap_Cvar_VariableIntegerValue("bot_enable")) {
		return;
	}
	// name
	trap_Argv(1, name, sizeof(name));

	if (!name[0]) {
		trap_Print("Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n");
		return;
	}
	// skill
	trap_Argv(2, string, sizeof(string));

	if (!string[0]) {
		skill = 3;
	} else {
		skill = Com_Clamp(1, 5, atof(string));
	}
	// team
	trap_Argv(3, team, sizeof(team));
	// delay
	trap_Argv(4, string, sizeof(string));

	if (!string[0]) {
		delay = 0;
	} else {
		delay = atoi(string);
	}
	// alternative name
	trap_Argv(5, altname, sizeof(altname));

	G_AddBot(name, skill, team, delay, altname);
	// if this was issued during gameplay and we are playing locally, go ahead and load the bot's media immediately
	if (level.time - level.startTime > 1000 && trap_Cvar_VariableIntegerValue("cl_running")) {
		trap_SendServerCommand(-1, "loaddeferred\n");
	}
}

/*
=======================================================================================================================================
Svcmd_AddBotComplete
=======================================================================================================================================
*/
void Svcmd_AddBotComplete(char *args, int argNum) {

	if (argNum == 2) {
		int i;
		char name[MAX_NAME_LENGTH];
		char list[32000]; // [MAX_BOTS * MAX_NAME_LENGTH] is too big to fit in QVM locals(max 32k)
		int listTotalLength;

		// ZTM: FIXME: have to clear whole list because BG_AddStringToList doesn't properly terminate list
		memset(list, 0, sizeof(list));

		listTotalLength = 0;

		for (i = 0; i < g_numBots; i++) {
			Q_strncpyz(name, Info_ValueForKey(g_botInfos[i], "name"), sizeof(name));
			Q_CleanStr(name);
			// Use quotes if there is a space in the name
			if (strchr(name, ' ')!= NULL) {
				BG_AddStringToList(list, sizeof(list), &listTotalLength, va("\"%s\"", name));
			} else {
				BG_AddStringToList(list, sizeof(list), &listTotalLength, name);
			}
		}

		if (listTotalLength > 0) {
			list[listTotalLength++] = 0;
			trap_Field_CompleteList(list);
		}
	} else if (argNum == 4) {
		trap_Field_CompleteList("blue\0follow1\0follow2\0free\0red\0scoreboard\0spectator\0");
	}
}

/*
=======================================================================================================================================

=======================================================================================================================================
*/
void Svcmd_BotList_f(void) {
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char aifile[MAX_TOKEN_CHARS];

	trap_Print("^1name             model            aifile              funname\n");

	for (i = 0; i < g_numBots; i++) {
		Q_strncpyz(name, Info_ValueForKey(g_botInfos[i], "name"), sizeof(name));

		if (!*name) {
			strcpy(name, DEFAULT_PLAYER_NAME);
		}

		Q_strncpyz(funname, Info_ValueForKey(g_botInfos[i], "funname"), sizeof(funname));

		if (!*funname) {
			strcpy(funname, "");
		}

		Q_strncpyz(model, Info_ValueForKey(g_botInfos[i], "model"), sizeof(model));

		if (!*model) {
			strcpy(model, DEFAULT_MODEL);
		}

		Q_strncpyz(aifile, Info_ValueForKey(g_botInfos[i], "aifile"), sizeof(aifile));

		if (!*aifile) {
			strcpy(aifile, "bots/default_c.c");
		}

		trap_Print(va("%-16s %-16s %-20s %-20s\n", name, model, aifile, funname));
	}
}

/*
=======================================================================================================================================
G_SpawnBots
=======================================================================================================================================
*/
static void G_SpawnBots(char *botList, int baseDelay) {
	char *bot;
	char *p;
	float skill;
	int delay;
	char bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = trap_Cvar_VariableValue("g_spSkill");

	if (skill < 1) {
		skill = 1;
		trap_Cvar_SetValue("g_spSkill", skill);
	} else if (skill > 5) {
		skill = 5;
		trap_Cvar_SetValue("g_spSkill", skill);
	}

	Q_strncpyz(bots, botList, sizeof(bots));

	p = &bots[0];
	delay = baseDelay;

	while (*p) {
		// skip spaces
		while (*p && *p == ' ') {
			p++;
		}

		if (!*p) {
			break;
		}
		// mark start of bot name
		bot = p;
		// skip until space of null
		while (*p && *p != ' ') {
			p++;
		}

		if (*p) {
			*p++ = 0;
		}
		// we must add the bot this way, calling G_AddBot directly at this stage does "Bad Things"
		trap_Cmd_ExecuteText(EXEC_INSERT, va("addbot %s %f free %i\n", bot, skill, delay));

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}

/*
=======================================================================================================================================
G_LoadBotsFromFile
=======================================================================================================================================
*/
static void G_LoadBotsFromFile(char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (!f) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	if (len >= MAX_BOTS_TEXT) {
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);

	buf[len] = 0;

	trap_FS_FCloseFile(f);

	g_numBots += G_ParseInfos(buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots]);
}

/*
=======================================================================================================================================
G_LoadBots
=======================================================================================================================================
*/
static void G_LoadBots(void) {
	vmCvar_t botsFile;
	int numdirs;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int i;
	int dirlen;

	if (!trap_Cvar_VariableIntegerValue("bot_enable")) {
		return;
	}

	g_numBots = 0;

	trap_Cvar_Register(&botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM);

	if (*botsFile.string) {
		G_LoadBotsFromFile(botsFile.string);
	} else {
		G_LoadBotsFromFile("scripts/bots.txt");
	}
	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024);
	dirptr = dirlist;

	for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);

		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadBotsFromFile(filename);
	}

	G_DPrintf("%i bots parsed\n", g_numBots);
}

/*
=======================================================================================================================================
G_GetBotInfoByNumber
=======================================================================================================================================
*/
char *G_GetBotInfoByNumber(int num) {

	if (num < 0 || num >= g_numBots) {
		trap_Print(va(S_COLOR_RED "Invalid bot number: %i\n", num));
		return NULL;
	}

	return g_botInfos[num];
}

/*
=======================================================================================================================================
G_GetBotInfoByName
=======================================================================================================================================
*/
char *G_GetBotInfoByName(const char *name) {
	int n;
	char *value;

	for (n = 0; n < g_numBots; n++) {
		value = Info_ValueForKey(g_botInfos[n], "name");

		if (!Q_stricmp(value, name)) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

/*
=======================================================================================================================================
Character_Parse
=======================================================================================================================================
*/
static qboolean Character_Parse(char **p, const char *filename) {
	char *token;
	char name[MAX_NAME_LENGTH];

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while (1) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if (!token[0]) {
			return qfalse;
		}

		if (token[0] == '{') {
			// two tokens per line, character name and sex
			token = COM_ParseExt(p, qtrue);

			if (!token[0]) {
				return qfalse;
			}

			Q_strncpyz(name, token, sizeof(name));

			token = COM_ParseExt(p, qtrue);

			if (!token[0]) {
				return qfalse;
			}

			if (g_numTeamBots < ARRAY_LEN(g_teamBotInfos)) {
				g_teamBotInfos[g_numTeamBots] = G_GetBotInfoByName(name);

				if (g_teamBotInfos[g_numTeamBots]) {
					g_numTeamBots++;
				} else {
					G_Printf(S_COLOR_YELLOW "WARNING: No bot defined in bots.txt for character '%s' in %s.\n", name, filename);
				}
			}

			token = COM_ParseExt(p, qtrue);

			if (token[0] != '}') {
				return qfalse;
			}
		}
	}

	return qfalse;
}

/*
=======================================================================================================================================
G_ParseTeamInfo

Team Arena's addbot menu only allows adding characters from teaminfo.txt. In g_gametypes > GT_TOURNAMENT, so use them for random bot
selection too.
=======================================================================================================================================
*/
static void G_ParseTeamInfo(const char *filename) {
	int len;
	fileHandle_t f;
	char buf[MAX_BOTS_TEXT];
	char *p, *token;

	g_numTeamBots = 0;

	if (g_gametype.integer < GT_TEAM) {
		return;
	}

	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (!f) {
		trap_Print(va(S_COLOR_RED "File not found: %s\n", filename));
		return;
	}

	if (len >= MAX_BOTS_TEXT) {
		trap_Print(va(S_COLOR_RED "File too large: %s is %i, max allowed is %i\n", filename, len, MAX_BOTS_TEXT));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);

	buf[len] = 0;

	trap_FS_FCloseFile(f);

	p = buf;

	while (1) {
		token = COM_ParseExt(&p, qtrue);

		if (!token[0] || token[0] == '}') {
			break;
		}

		if (Q_stricmp(token, "}") == 0) {
			break;
		}

		if (Q_stricmp(token, "teams") == 0) {
			if (SkipBracedSection(&p, 0)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "characters") == 0) {
			if (Character_Parse(&p, filename)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "aliases") == 0) {
			if (SkipBracedSection(&p, 0)) {
				continue;
			} else {
				break;
			}
		}
	}
}

/*
=======================================================================================================================================
G_InitBots
=======================================================================================================================================
*/
void G_InitBots(qboolean restart) {
	int fragLimit;
	int timeLimit;
	const char *arenainfo;
	char *strValue;
	int basedelay;
	char map[MAX_QPATH];
	char serverinfo[MAX_INFO_STRING];

	G_LoadBots();
	G_LoadArenas();
	G_ParseTeamInfo("teaminfo.txt");

	trap_Cvar_Register(&bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO);

	if (g_gametype.integer == GT_SINGLE_PLAYER) {
		trap_GetServerinfo(serverinfo, sizeof(serverinfo));
		Q_strncpyz(map, Info_ValueForKey(serverinfo, "mapname"), sizeof(map));

		arenainfo = G_GetArenaInfoByMap(map);

		if (!arenainfo) {
			return;
		}

		fragLimit = atoi(Info_ValueForKey(arenainfo, "fraglimit"));
		timeLimit = atoi(Info_ValueForKey(arenainfo, "timelimit"));

		if (!fragLimit && !timeLimit) {
			trap_Cvar_SetValue("fraglimit", 10);
			trap_Cvar_SetValue("timelimit", 0);
		} else {
			trap_Cvar_SetValue("fraglimit", fragLimit);
			trap_Cvar_SetValue("timelimit", timeLimit);
		}

		basedelay = BOT_BEGIN_DELAY_BASE;
		strValue = Info_ValueForKey(arenainfo, "special");

		if (Q_stricmp(strValue, "training") == 0) {
			basedelay += 10000;
		}

		if (!restart) {
			G_SpawnBots(Info_ValueForKey(arenainfo, "bots"), basedelay);
		}
	}
}
