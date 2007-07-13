
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

static int size = 0;
static Window *clients = NULL;

 /**
	* Create a new client
	* @param win Window of the client
	* @return Returns either a #SubWin on success or otherwise NULL
	**/

SubWin *
subClientNew(Window win)
{
	int i, n = 0;
	Window unnused;
	XWMHints *hints = NULL;
	XWindowAttributes attr;
	Atom *protos = NULL;
	SubWin *w = subWinNew(win);
	w->client = (SubClient *)calloc(1, sizeof(SubClient));
	if(!w->client) subLogError("Can't alloc memory. Exhausted?\n");

	XGrabServer(d->dpy);
	XAddToSaveSet(d->dpy, w->win);
	XGetWindowAttributes(d->dpy, w->win, &attr);
	w->client->cmap	= attr.colormap;
	w->flags				= SUB_WIN_TYPE_CLIENT; 

	/* EWMH: Client list and client list stacking */
	clients = (Window *)realloc(clients, sizeof(Window) * (size + 1));
	if(!clients) subLogError("Can't alloc memory. Exhausted?\n");
	clients[size] = w->win;
	size++;
	subEwmhSetWindows(DefaultRootWindow(d->dpy), SUB_EWMH_NET_CLIENT_LIST, clients, size);
	subEwmhSetWindows(DefaultRootWindow(d->dpy), SUB_EWMH_NET_CLIENT_LIST_STACKING, clients, size);

	/* Window caption */
	XFetchName(d->dpy, win, &w->client->name);
	if(!w->client->name) w->client->name = strdup("subtle");
	w->client->caption = XCreateSimpleWindow(d->dpy, w->frame, d->th, 0, 
		(strlen(w->client->name) + 1) * d->fx, d->th, 0, d->colors.border, d->colors.norm);

	/* Window manager hints */
	hints = XGetWMHints(d->dpy, win);
	if(hints)
		{
			if(hints->flags & StateHint) subClientSetWMState(w, hints->initial_state);
			else subClientSetWMState(w, NormalState);
			if(hints->initial_state == IconicState) subLogDebug("Iconic: win=%#lx\n", win);			
			if(hints->input) w->flags |= SUB_WIN_PREF_INPUT;
			if(hints->flags & XUrgencyHint) w->flags |= SUB_WIN_OPT_TRANS;
			XFree(hints);
		}
	
	/* Protocols */
	if(XGetWMProtocols(d->dpy, w->win, &protos, &n))
		{
			for(i = 0; i < n; i++)
				{
					if(protos[i] == subEwmhGetAtom(SUB_EWMH_WM_TAKE_FOCUS)) 		w->flags |= SUB_WIN_PREF_FOCUS;
					if(protos[i] == subEwmhGetAtom(SUB_EWMH_WM_DELETE_WINDOW))	w->flags |= SUB_WIN_PREF_CLOSE;
				}
			XFree(protos);
		}

	subWinMap(w);

	/* Check for dialog windows etc. */
	XGetTransientForHint(d->dpy, win, &unnused);
	if(unnused && !(w->flags & SUB_WIN_OPT_TRANS)) w->flags |= SUB_WIN_OPT_TRANS;

	XSync(d->dpy, False);
	XUngrabServer(d->dpy);
	return(w);
}

 /**
	* Delete a client
	* @param w A #SubWin
	**/

void
subClientDelete(SubWin *w)
{
	if(w && w->flags & SUB_WIN_TYPE_CLIENT)
		{
			int i, j;

			if(w->flags & SUB_WIN_PREF_CLOSE)
				{
					XEvent ev;

					ev.type									= ClientMessage;
					ev.xclient.window				= w->win;
					ev.xclient.message_type = subEwmhGetAtom(SUB_EWMH_WM_PROTOCOLS);
					ev.xclient.format				= 32;
					ev.xclient.data.l[0]		= subEwmhGetAtom(SUB_EWMH_WM_DELETE_WINDOW);
					ev.xclient.data.l[1]		= CurrentTime;

					XSendEvent(d->dpy, w->win, False, NoEventMask, &ev);

					printf("Delete: send %#lx delete event\n", w->win);
				}
			else XDestroyWindow(d->dpy, w->win);

			if(w->client->name) XFree(w->client->name);
			free(w->client);

			/* Update client list */
			for(i = 0; i < size; i++) 	
				if(clients[i] == w->win)
					{
						for(j = i; j < size - 1; j++) clients[j] = clients[j + 1];
						break; /* Leave loop */
					}

			clients = (Window *)realloc(clients, sizeof(Window) * size);
			if(!clients)  subLogError("Can't alloc memory. Exhausted?\n");
			size--;

			/* EWMH: Client list and client list stacking */			
			subEwmhSetWindows(DefaultRootWindow(d->dpy), SUB_EWMH_NET_CLIENT_LIST, clients, size);
			subEwmhSetWindows(DefaultRootWindow(d->dpy), SUB_EWMH_NET_CLIENT_LIST_STACKING, clients, size);					
		}
}

 /**
	* Render the client window
	* @param w A #SubWin
	**/

void
subClientRender(SubWin *w)
{
	if(w && w->flags & SUB_WIN_TYPE_CLIENT)
		{
			unsigned long col = d->focus && d->focus == w ? d->colors.focus : 
				(w->flags & SUB_WIN_OPT_COLLAPSE ? d->colors.cover : d->colors.norm);

			XSetWindowBackground(d->dpy, w->client->caption, col);
			XClearWindow(d->dpy, w->client->caption);
			XDrawString(d->dpy, w->client->caption, d->gcs.font, 3, d->fy - 1, w->client->name, 
				strlen(w->client->name));
		}
}

 /**
	* Send a configure request to the client
	* @param w A #SubWin
	**/

void
subClientConfigure(SubWin *w)
{
	XConfigureEvent ev;

	ev.type								= ConfigureNotify;
	ev.event							= w->win;
	ev.window							= w->win;
	ev.x									= w->x;
	ev.y									= w->y;
	ev.width							= SUBWINWIDTH(w);
	ev.height							= SUBWINHEIGHT(w);
	ev.above							= None;
	ev.border_width 			= 0;
	ev.override_redirect	= 0;

	XSendEvent(d->dpy, w->win, False, StructureNotifyMask, (XEvent *)&ev);
}

	/**
	 * Fetch client name
	 * @param w A #SubWin
	 **/

void
subClientFetchName(SubWin *w)
{
	if(w && w->flags & SUB_WIN_TYPE_CLIENT)
		{
			int width, mode;
			if(w->client->name) XFree(w->client->name);
			XFetchName(d->dpy, w->win, &w->client->name);

			/* Check max length of the caption */
			width = (strlen(w->client->name) + 1) * d->fx;
			if(width > w->width - d->th - 4) width = w->width - d->th - 14;
			XMoveResizeWindow(d->dpy, w->client->caption, d->th, 0, width, d->th);

			mode = (d->focus && d->focus->frame == w->frame) ? 0 : 1;
			subClientRender(w);
		}
}

 /**
	* Set the WM state for the client
	* @param w A #SubWin
	* @param state New state for the client
	**/

void
subClientSetWMState(SubWin *w,
	long state)
{
	CARD32 data[2];
	data[0] = state;
	data[1] = None; /* No icons */

	XChangeProperty(d->dpy, w->win, subEwmhGetAtom(SUB_EWMH_WM_STATE), subEwmhGetAtom(SUB_EWMH_WM_STATE),
		32, PropModeReplace, (unsigned char *)data, 2);
}

 /**
	* Get the WM state of the client
	* @param w A #SubWin
	* @return Returns the state of the client
	**/

long
subClientGetWMState(SubWin *w)
{
	Atom type;
	int format;
	unsigned long nil, bytes;
	long *data = NULL, state = WithdrawnState;

	if(XGetWindowProperty(d->dpy, w->win, subEwmhGetAtom(SUB_EWMH_WM_STATE), 0L, 2L, False, 
			subEwmhGetAtom(SUB_EWMH_WM_STATE), &type, &format, &bytes, &nil,
			(unsigned char **) &data) == Success && bytes)
		{
			state = *data;
			XFree(data);
		}
	return(state);
}
