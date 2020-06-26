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

/*****************************************************************************
 * name:		ai_ea.c
 *
 * desc:		elementary actions
 *
 * $Archive: /MissionPack/code/game/ai_ea.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "ai_char.h"
#include "ai_chat_sys.h"
#include "ai_ea.h"
#include "ai_gen.h"
#include "ai_goal.h"
#include "ai_move.h"
#include "ai_weap.h"
#include "ai_weight.h"
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_vcmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
#include "chars.h" // characteristics
#include "inv.h" // indexes into the inventory
#include "syn.h" // synonyms
#include "match.h" // string matching types and vars

#define MAX_USERMOVE 400
#define MAX_COMMANDARGUMENTS 10

bot_input_t botinputs[MAX_CLIENTS];

/*
=======================================================================================================================================
EA_Say
=======================================================================================================================================
*/
void EA_Say(int playerNum, char *str) {
	trap_ClientCommand(playerNum, va("say %s", str));
}

/*
=======================================================================================================================================
EA_SayTeam
=======================================================================================================================================
*/
void EA_SayTeam(int playerNum, char *str) {
	trap_ClientCommand(playerNum, va("say_team %s", str));
}

/*
=======================================================================================================================================
EA_Tell
=======================================================================================================================================
*/
void EA_Tell(int playerNum, int playerto, char *str) {
	trap_ClientCommand(playerNum, va("tell %d, %s", playerto, str));
}

/*
=======================================================================================================================================
EA_UseItem
=======================================================================================================================================
*/
void EA_UseItem(int playerNum, char *it) {
	trap_ClientCommand(playerNum, va("use %s", it));
}

/*
=======================================================================================================================================
EA_DropItem
=======================================================================================================================================
*/
void EA_DropItem(int playerNum, char *it) {
	trap_ClientCommand(playerNum, va("drop %s", it));
}

/*
=======================================================================================================================================
EA_UseInv
=======================================================================================================================================
*/
void EA_UseInv(int playerNum, char *inv) {
	trap_ClientCommand(playerNum, va("invuse %s", inv));
}

/*
=======================================================================================================================================
EA_DropInv
=======================================================================================================================================
*/
void EA_DropInv(int playerNum, char *inv) {
	trap_ClientCommand(playerNum, va("invdrop %s", inv));
}

/*
=======================================================================================================================================
EA_Gesture
=======================================================================================================================================
*/
void EA_Gesture(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_GESTURE;
}

/*
=======================================================================================================================================
EA_Command
=======================================================================================================================================
*/
void EA_Command(int playerNum, char *command) {
	trap_ClientCommand(playerNum, command);
}

/*
=======================================================================================================================================
EA_SelectWeapon
=======================================================================================================================================
*/
void EA_SelectWeapon(int playerNum, int weapon) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->weapon = weapon;
}

/*
=======================================================================================================================================
EA_Attack
=======================================================================================================================================
*/
void EA_Attack(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_ATTACK;
}

/*
=======================================================================================================================================
EA_Talk
=======================================================================================================================================
*/
void EA_Talk(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_TALK;
}

/*
=======================================================================================================================================
EA_Use
=======================================================================================================================================
*/
void EA_Use(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_USE;
}

/*
=======================================================================================================================================
EA_Respawn
=======================================================================================================================================
*/
void EA_Respawn(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_RESPAWN;
}

/*
=======================================================================================================================================
EA_Jump
=======================================================================================================================================
*/
void EA_Jump(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME) {
		bi->actionflags & = ~ACTION_JUMP;
	} else {
		bi->actionflags |= ACTION_JUMP;
	}
}

/*
=======================================================================================================================================
EA_DelayedJump
=======================================================================================================================================
*/
void EA_DelayedJump(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	if (bi->actionflags & ACTION_JUMPEDLASTFRAME) {
		bi->actionflags & = ~ACTION_DELAYEDJUMP;
	} else {
		bi->actionflags |= ACTION_DELAYEDJUMP;
	}
}

/*
=======================================================================================================================================
EA_Crouch
=======================================================================================================================================
*/
void EA_Crouch(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_CROUCH;
}

/*
=======================================================================================================================================
EA_Action
=======================================================================================================================================
*/
void EA_Action(int playerNum, int action) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= action;
}

/*
=======================================================================================================================================
EA_MoveUp
=======================================================================================================================================
*/
void EA_MoveUp(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVEUP;
}

/*
=======================================================================================================================================
EA_MoveDown
=======================================================================================================================================
*/
void EA_MoveDown(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVEDOWN;
}

/*
=======================================================================================================================================
EA_MoveForward
=======================================================================================================================================
*/
void EA_MoveForward(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVEFORWARD;
}

/*
=======================================================================================================================================
EA_MoveBack
=======================================================================================================================================
*/
void EA_MoveBack(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVEBACK;
}

/*
=======================================================================================================================================
EA_MoveLeft
=======================================================================================================================================
*/
void EA_MoveLeft(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVELEFT;
}

/*
=======================================================================================================================================
EA_MoveRight
=======================================================================================================================================
*/
void EA_MoveRight(int playerNum) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	bi->actionflags |= ACTION_MOVERIGHT;
}

/*
=======================================================================================================================================
EA_Move
=======================================================================================================================================
*/
void EA_Move(int playerNum, vec3_t dir, float speed) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	VectorCopy(dir, bi->dir);
	// cap speed
	if (speed > MAX_USERMOVE) {
		speed = MAX_USERMOVE;
	} else if (speed < -MAX_USERMOVE) {
		speed = -MAX_USERMOVE;
	}

	bi->speed = speed;

	if (speed <= 200) {
		bi->actionflags |= ACTION_WALK;
	}
}

/*
=======================================================================================================================================
EA_View
=======================================================================================================================================
*/
void EA_View(int playerNum, vec3_t viewangles) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];

	VectorCopy(viewangles, bi->viewangles);
}

/*
=======================================================================================================================================
EA_EndRegular
=======================================================================================================================================
*/
void EA_EndRegular(int playerNum, float thinktime) {

}

/*
=======================================================================================================================================
EA_GetInput
=======================================================================================================================================
*/
void EA_GetInput(int playerNum, float thinktime, bot_input_t *input) {
	bot_input_t *bi;

	bi = &botinputs[playerNum];
	bi->thinktime = thinktime;

	Com_Memcpy(input, bi, sizeof(bot_input_t));
}

/*
=======================================================================================================================================
EA_ResetInput
=======================================================================================================================================
*/
void EA_ResetInput(int playerNum) {
	bot_input_t *bi;
	int jumped = qfalse;

	bi = &botinputs[playerNum];

	bi->thinktime = 0;

	VectorClear(bi->dir);

	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;

	if (jumped) {
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}
}

/*
=======================================================================================================================================
EA_Setup
=======================================================================================================================================
*/
int EA_Setup(void) {

	// initialize the bot inputs
	Com_Memset(botinputs, 0, sizeof(botinputs));
	return BLERR_NOERROR;
}

/*
=======================================================================================================================================
EA_Shutdown
=======================================================================================================================================
*/
void EA_Shutdown(void) {

}
