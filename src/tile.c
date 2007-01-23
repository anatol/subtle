#include "subtle.h"

 /**
	* Create a new tile.
	* @param mode Tile mode
	* @return Return either a #SubWin on success or otherwise NULL.
	**/

SubWin *
subTileNew(short mode)
{
	SubWin *w = subWinNew(0);

	w->prop	= mode ? SUB_WIN_TILEV : SUB_WIN_TILEH;
	w->tile	= (SubTile *)calloc(1, sizeof(SubTile));
	if(!w->tile) subLogError("Can't alloc memory. Exhausted?\n");

	w->tile->btnew	= XCreateSimpleWindow(d->dpy, w->frame, d->th, 0, 7 * d->fx, d->th, 0, d->colors.border, d->colors.norm);
	w->tile->btdel	= XCreateSimpleWindow(d->dpy, w->frame, d->th + 7 * d->fx, 0, 7 * d->fx, d->th, 0, d->colors.border, d->colors.norm);
	w->tile->mw			= w->width;
	w->tile->mh			= w->height;

	screen->active = w;

	subWinMap(w);
	subLogDebug("Adding %s-tile: x=%d, y=%d, width=%d, height=%d\n", (w->prop & SUB_WIN_TILEH) ? "h" : "v", w->x, w->y, w->width, w->height);

	return(w);
}

 /**
	* Add a window to a tile.
	* @param t A #SubWin
	* @param w A #SubWin
	**/

void
subTileAdd(SubWin *t,
	SubWin *w)
{
	XReparentWindow(d->dpy, w->frame, t->win, 0, 0);
	subWinRestack(w);
	subTileConfigure(t);
	w->parent = t;

	subLogDebug("Adding window: x=%d, y=%d, width=%d, height=%d\n", w->x, w->y, w->width, w->height);
}

 /**
	* Delete a tile window and all children.
	* @param w A #SubWin
	**/

void
subTileDelete(SubWin *w)
{
	unsigned int n, i;
	Window nil, *wins = NULL;

	if(w->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV))
		{
			SubWin *parent = NULL;
			XGrabServer(d->dpy);
			subWinUnmap(w);

			if(screen->active == w) screen->active = w->parent;

			XQueryTree(d->dpy, w->win, &nil, &nil, &wins, &n);	
			for(i = 0; i < n; i++)
				{
					SubWin *c = subWinFind(wins[i]);
					if(c && c->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV)) subTileDelete(c);
					else if(c && c->prop & SUB_WIN_CLIENT) subClientSendDelete(c);
				}

			subLogDebug("Deleting %s-tile with %d children\n", (w->prop & SUB_WIN_TILEH) ? "h" : "v", n);

			if(w->parent) parent = w->parent; 
			free(w->tile); 
			subWinDelete(w);
			subTileConfigure(parent); 
			XFree(wins);

			XSync(d->dpy, False);
			XUngrabServer(d->dpy);
		}
}

 /**
	* Render a tile window.
	* @param mode Render mode
	* @param w A #SubWin
	**/

void
subTileRender(short mode,
	SubWin *w)
{
	unsigned long col = mode ? d->colors.norm : d->colors.act;

	if(w->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV))
		{
			XSetWindowBackground(d->dpy, w->tile->btnew, col);
			XSetWindowBackground(d->dpy, w->tile->btdel, col);
			XClearWindow(d->dpy, w->tile->btnew);
			XClearWindow(d->dpy, w->tile->btdel);

			/* Descriptive buttons */
			if(w->prop & SUB_WIN_TILEV) XDrawString(d->dpy, w->tile->btnew, d->gcs.font, 3, d->fy - 1, "Newrow", 6);
			else XDrawString(d->dpy, w->tile->btnew, d->gcs.font, 3, d->fy - 1, "Newcol", 6);
			if(w->parent && w->parent->prop & SUB_WIN_TILEV) XDrawString(d->dpy, w->tile->btdel, d->gcs.font, 3, d->fy - 1, "Delrow", 6);
			else XDrawString(d->dpy, w->tile->btdel, d->gcs.font, 3, d->fy - 1, "Delcol", 6);
		}
}

 /**
	* Configure a tile and each child.
	* @param w A #SubWin
	**/

void
subTileConfigure(SubWin *w)
{
	unsigned int n = 0, i, y = 0, fix = 0;
	Window nil, *wins = NULL;
	XWindowAttributes attr;

	if(w && w->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV))
		{
			XGetWindowAttributes(d->dpy, w->win, &attr);
			XQueryTree(d->dpy, w->win, &nil, &nil, &wins, &n);
			if(w->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV) && n > 0)
				{
					n						= n - w->tile->shaded >= 0 ? n - w->tile->shaded : 1; /* Prevent division by zero */
					w->tile->mw = (w->prop & SUB_WIN_TILEH) ? attr.width / n	: attr.width;
					w->tile->mh = (w->prop & SUB_WIN_TILEH) ? attr.height : (attr.height - w->tile->shaded * d->th) / n;

					/* Fix rounding */
					if(w->prop & SUB_WIN_TILEH) fix = abs(attr.width - n * w->tile->mw - w->tile->shaded * d->th);
					else fix = abs(attr.height - n * w->tile->mh - w->tile->shaded * d->th);

					for(i = 0; i < n + w->tile->shaded; i++)
						{
							SubWin *c = subWinFind(wins[i]);
							if(c && !(c->prop & (SUB_WIN_FLOAT|SUB_WIN_TRANS)))
								{
									c->height = (c->prop & SUB_WIN_SHADED) ? d->th : w->tile->mh;
									c->width	= w->tile->mw;
									c->x			= (w->prop & SUB_WIN_TILEH) ? i * w->tile->mw : 0;
									c->y			= (w->prop & SUB_WIN_TILEH) ? 0 : y;

									if(i == n + w->tile->shaded - 1) 
										if(w->prop & SUB_WIN_TILEH) c->width += fix;
										else c->height += fix;

									y += c->height;

									subLogDebug("Configuring %s-window: x=%d, y=%d, width=%d, height=%d\n", (c->prop & SUB_WIN_SHADED) ? "s" : "n", c->x, c->y, c->width, c->height);

									subWinResize(c);

									if(c->prop & SUB_WIN_CLIENT) subClientSendConfigure(c);
									else if(c->prop & (SUB_WIN_TILEH|SUB_WIN_TILEV)) subTileConfigure(c);
								}
						}
					XFree(wins);
					subLogDebug("Configuring %s-tile: n=%d, mw=%d, mh=%d\n", (w->prop & SUB_WIN_TILEH) ? "h" : "v", n, w->tile->mw, w->tile->mh);
				}
		}
}
