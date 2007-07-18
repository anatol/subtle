
 /**
	* subtle - window manager
	* Copyright (c) 2005-2007 Christoph Kappel
	*
	* This program is free software; you can redistribute it and/or modify
	* it under the terms of the GNU General Public License as published by
	* the Free Software Foundation; either version 2 of the License, or
	* (at your option) any later version.
	*
	* This program is distributed in the hope that it will be useful,
	* but WITHOUT ANY WARRANTY; without even the implied warranty of
	* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	* GNU General Public License for more details.
	*
	* You should have received a copy of the GNU General Public License along
	* with this program; if not, write to the Free Software Foundation, Inc.,
	* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
	**/

#include "subtle.h"

int size = 0;
SubKey **keys = NULL;

 /**
	* Find a key
	* @param keycode A keycode
	* @return Returns a #SubKey or NULL
	**/

SubKey *
subKeyFind(int keycode,
	int mod)
{
	int i;

	for(i = 0; i < size; i++)
		if(keys[i]->code == keycode && keys[i]->mod == mod) return(keys[i]);
	return(NULL);
}

 /**
	* Parse key chains
	* @key Key name
	* @value Key action
	**/

void
subKeyParseChain(const char *key,
	const char *value)
{
	int mod;
	KeySym sym;
	char *tok = strtok((char *)value, "-");
	SubKey *k = (SubKey *)calloc(1, sizeof(SubKey));
	if(!k) subLogError("Can't alloc memory. Exhausted?\n");

	/* FIXME: strncmp() is really slow.. */	
	if(!strncmp(key, "AddVertTile", 11)) 					k->flags = SUB_KEY_ACTION_ADD_VTILE;
	else if(!strncmp(key, "AddHorzTile", 11)) 		k->flags = SUB_KEY_ACTION_ADD_HTILE;
	else if(!strncmp(key, "DeleteWindow", 12))		k->flags = SUB_KEY_ACTION_DELETE_WIN;
	else if(!strncmp(key, "ToggleCollapse", 14))	k->flags = SUB_KEY_ACTION_TOGGLE_COLLAPSE;
	else if(!strncmp(key, "ToggleRaise", 11))			k->flags = SUB_KEY_ACTION_TOGGLE_RAISE;
	else if(!strncmp(key, "ToggleFull", 10))			k->flags = SUB_KEY_ACTION_TOGGLE_FULL;
	else if(!strncmp(key, "ToggleWeight", 12))		k->flags = SUB_KEY_ACTION_TOGGLE_WEIGHT;
	else if(!strncmp(key, "TogglePile", 10))			k->flags = SUB_KEY_ACTION_TOGGLE_PILE;
	else if(!strncmp(key, "NextDesktop", 11))			k->flags = SUB_KEY_ACTION_DESKTOP_NEXT;
	else if(!strncmp(key, "PreviousDesktop", 11))	k->flags = SUB_KEY_ACTION_DESKTOP_PREV;
	else if(!strncmp(key, "MoveToDesktop", 13))
		{
			char *desktop = (char *)key + 13;
			if(desktop) 
				{
					k->number = atoi(desktop);
					k->flags = SUB_KEY_ACTION_DESKTOP_MOVE;
				}
			else 
				{
					subLogWarn("Can't assign keychain `%s'.\n", key);
					return;
				}
		}
	else
		{
			k->flags	= SUB_KEY_ACTION_EXEC;
			k->string	= strdup(key);
		}

	while(tok)
		{ 
			/* Get key sym and modifier */
			switch(*tok)
				{
					case 'S':	sym = XK_Shift_L;		mod = ShiftMask;		break;
					case 'C':	sym = XK_Control_L;	mod = ControlMask;	break;
					case 'A':	sym = XK_Alt_L;			mod = Mod1Mask;			break;
					case 'W':	sym = XK_Super_L;		mod = Mod4Mask;			break;
					case 'M':	sym = XK_Meta_L;		mod = Mod3Mask;			break;
					default:
						sym = XStringToKeysym(tok);
						if(sym == NoSymbol) 
							{
								subLogWarn("Can't assign keychain `%s'.\n", key);
								if(k->string) free(k->string);
								free(k);
								return;
							}
				}
			if(IsModifierKey(sym)) k->mod |= mod;
			else k->code = XKeysymToKeycode(d->dpy, sym);

			tok = strtok(NULL, "-");
		}
	
	if(k->code && k->mod)
		{
			keys = (SubKey **)realloc(keys, sizeof(SubKey *) * (size + 1));
			if(!keys) subLogError("Can't alloc memory. Exhausted?\n");

			keys[size] = k;
			size++;

			subLogDebug("Keychain: name=%s, code=%d, mod=%d\n", key, k->code, k->mod);
		}
}

 /**
	* Kill keys
	**/

void
subKeyKill(void)
{
	int i;

	if(keys)
		{
			for(i = 0; i < size; i++) 
				{
					if(keys[i])
						{
							if(keys[i]->flags == SUB_KEY_ACTION_EXEC && keys[i]->string) free(keys[i]->string);
							free(keys[i]);
						}
				}

			free(keys);
		}
}

 /**
	* Grab keys for a window
	* @param  w A #SubWin
	**/

void
subKeyGrab(SubWin *w)
{
	if(w && keys)
		{
			int i;

			for(i = 0; i < size; i++) 
				XGrabKey(d->dpy, keys[i]->code, keys[i]->mod, w->frame, True, GrabModeAsync, GrabModeAsync);

			/* Focus */
			if(w->flags & SUB_WIN_TYPE_CLIENT && !(w->flags & SUB_WIN_STATE_COLLAPSE))
				{
					if(w->flags & SUB_WIN_PREF_FOCUS)
						{
							XEvent ev;

							ev.type									= ClientMessage;
							ev.xclient.window				= w->win;
							ev.xclient.message_type = subEwmhGetAtom(SUB_EWMH_WM_PROTOCOLS);
							ev.xclient.format				= 32;
							ev.xclient.data.l[0]		= subEwmhGetAtom(SUB_EWMH_WM_TAKE_FOCUS);
							ev.xclient.data.l[1]		= CurrentTime;

							XSendEvent(d->dpy, w->win, False, NoEventMask, &ev);
						}
					else if(w->flags & SUB_WIN_PREF_INPUT) XSetInputFocus(d->dpy, w->win, RevertToNone, CurrentTime);

					subLogDebug("Grab: Focus on win=%#lx, input=%d, send=%d\n", w->win, 
						w->flags & SUB_WIN_PREF_INPUT, w->flags & SUB_WIN_PREF_FOCUS);
				}
			else if(!(w->flags & SUB_WIN_STATE_COLLAPSE)) XSetInputFocus(d->dpy, w->frame, RevertToNone, CurrentTime);				
		}
}

 /** 
	* Ungrab keys for a window
	**/

void
subKeyUngrab(SubWin *w)
{
	XUngrabKey(d->dpy, AnyKey, AnyModifier, w->frame);
}
