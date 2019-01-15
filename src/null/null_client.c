/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

cvar_t *cl_shownet;

void CL_Shutdown(char *finalmsg, qboolean disconnect, qboolean quit)
{
}

void CL_Init( void ) {
	cl_shownet = Cvar_Get ("cl_shownet", "0", CVAR_TEMP );
}

void CL_MouseEvent( int dx, int dy, int time ) {
}

void Key_WriteBindings( fileHandle_t f ) {
}

void CL_Frame ( int msec ) {
}

void CL_PacketEvent( netadr_t from, msg_t *msg ) {
}

void CL_CharEvent( int key ) {
}

void CL_Disconnect( qboolean showMainMenu ) {
}

void CL_MapLoading( void ) {
}

qboolean CL_GameCommand( void ) {
  return qfalse;
}

void CL_KeyEvent (int key, qboolean down, unsigned time) {
}

qboolean UI_GameCommand( void ) {
	return qfalse;
}

void CL_ForwardCommandToServer( const char *string ) {
}

void CL_ConsolePrint( char *txt ) {
}

void CL_JoystickEvent( int axis, int value, int time ) {
}

void CL_InitKeyCommands( void ) {
}

void CL_CDDialog( void ) {
}

void CL_FlushMemory(void)
{
}

void CL_ShutdownAll(qboolean shutdownRef)
{
}

void CL_StartHunkUsers( qboolean rendererOnly ) {
}

void CL_InitRef(void)
{
}

void CL_Snd_Shutdown(void)
{
}
