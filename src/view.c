
 /**
	* subtle - window manager
	* Copyright (c) 2005-2007 Christoph Kappel
	*
	* See the COPYING file for the license in the latest tarball.
	**/

#include "subtle.h"

static int nviews = 0;
static SubView *last = NULL;

static void
TagView(SubView *v)
{
	assert(v);
	v->w = subTileNew(SUB_WIN_TILE_HORZ);
	v->w->flags |= SUB_WIN_TYPE_VIEW;
	v->button = XCreateSimpleWindow(d->disp, d->bar.views, 0, 0, 1, d->th, 0, d->colors.border, d->colors.norm);

	XMapWindow(d->disp, v->button);
	XSaveContext(d->disp, v->button, 1, (void *)v);
}

static void
UntagView(SubView *v)
{
	assert(v->w);
	XDestroySubwindows(d->disp, v->w->frame);
	XDeleteContext(d->disp, v->button, 1);
	XDestroyWindow(d->disp, v->button);
	subTileDelete(v->w);
	free(v->w);
	v->w = NULL;
}

 /**
	* Create a view
	* @param name Name of the view
	* @param tags Tag list
	* @return Returns a new #SubWin
	**/

SubView *
subViewNew(char *name,
	char *tags)
{
	SubView *v = NULL;

	assert(name);
	
	v	= (SubView *)subUtilAlloc(1, sizeof(SubView));
	v->name		= strdup(name);
	v->width	=	strlen(v->name) * d->fx + 8;
	v->xid		= ++nviews;

	if(tags)
		{
			int errcode;
			v->regex = (regex_t *)subUtilAlloc(1, sizeof(regex_t));

			/* Thread safe error handling.. */
			if((errcode = regcomp(v->regex, tags, REG_EXTENDED|REG_NOSUB|REG_ICASE)))
				{
					size_t errsize = regerror(errcode, v->regex, NULL, 0);
					char *errbuf = (char *)subUtilAlloc(1, errsize);

					regerror(errcode, v->regex, errbuf, errsize);

					subUtilLogWarn("Can't compile regex `%s'\n", tags);
					subUtilLogDebug("%s\n", errbuf);

					free(errbuf);
					regfree(v->regex);
					free(v->regex);
					free(v);
					return(NULL);
				}
		}
	if(!d->rv) 
		{
			d->rv = v;
			last	= v;
		}
	else
		{
			last->next = v;
			v->prev = last;
			last = v;
		}

	printf("Adding view %s (%s)\n", v->name, tags ? tags : "default");
	return(v);
}

 /**
	* Delete a view
	* @param w A #SubWin
	**/

void
subViewDelete(SubView *v)
{
	if(v && d->cv != v)
		{
			printf("Removing view (%s)\n", v->name);

			subWinUnmap(v->w);
			XUnmapWindow(d->disp, v->button);
			nviews--;

			subViewConfigure();
		}
}

 /**
	* Sift window
	* @param w A #SubWin
	**/

void
subViewSift(Window win)
{
	int sifted = 0;
	SubView *v = NULL;
	char *class = NULL;

	assert(win && last);

	class = subEwmhGetProperty(win, XA_STRING, SUB_EWMH_NET_WM_CLASS, NULL);

	/* Loop backwards because root view has no regex */
	v = last;
	while(v)
		{
			if((v == d->rv && !sifted) || (v->regex && !regexec(v->regex, class, 0, NULL, 0)))
				{
					SubWin *w = subClientNew(win);

					if(w->flags & SUB_WIN_STATE_TRANS) 
						{
							w->parent = d->cv->w;
							subClientToggle(SUB_WIN_STATE_FLOAT, w);
						}
					else w->views |= v->xid;

					if(!v->w) TagView(v);
					subTileAdd(v->w, w);
					subTileConfigure(v->w);
					sifted++;

					XSaveContext(d->disp, w->frame, 1, (void *)w);

					/* Map only if the desired view is the current view */
					if(d->cv == v) subWinMap(w);
					else XMapSubwindows(d->disp, w->frame);

					printf("Adding client %s (%s)\n", w->client->name, v->name);
				}
			v = v->prev;
		}
	
	XFree(class);
	
	subViewConfigure();
	subViewRender();
}

 /**
	* Render views
	* @param v A #SubView
	**/

void
subViewRender(void)
{
	SubView *v = NULL;
	
	assert(d->rv);

	XClearWindow(d->disp, d->bar.win);
	XFillRectangle(d->disp, d->bar.win, d->gcs.border, 0, 2, DisplayWidth(d->disp, DefaultScreen(d->disp)), d->th - 4);	

	v = d->rv;
	while(v)
		{
			if(v->w)
				{
					XSetWindowBackground(d->disp, v->button, (d->cv == v) ? d->colors.focus : d->colors.norm);
					XClearWindow(d->disp, v->button);
					XDrawString(d->disp, v->button, d->gcs.font, 3, d->fy - 1, v->name, strlen(v->name));
				}
			v = v->next;
		}
}

 /**
	* Configure views
	**/

void
subViewConfigure(void)
{
	int xid = 0, i = 0, width = 0;
	char **names = NULL;
	Window *wins = NULL;
	SubView *v = NULL;

	assert(d->rv);

	wins = (Window *)subUtilAlloc(nviews, sizeof(Window));
	names = (char **)subUtilAlloc(nviews, sizeof(char *));

	v = d->rv;
	while(v)
		{
			if(v->w)
				{
					XMoveResizeWindow(d->disp, v->button, width, 0, v->width, d->th);
					width += v->width;
					wins[i] = v->w->frame;
					names[i++]	= v->name;
				}
			v = v->next;
		}
	XMoveResizeWindow(d->disp, d->bar.views, 0, 0, width, d->th);

	/* EWMH: Virtual roots */
	subEwmhSetWindows(DefaultRootWindow(d->disp), SUB_EWMH_NET_VIRTUAL_ROOTS, wins, nviews);
	free(wins);

	/* EWMH: Desktops */
	subEwmhSetCardinals(DefaultRootWindow(d->disp), SUB_EWMH_NET_NUMBER_OF_DESKTOPS, (long *)&i, 1);
	subEwmhSetStrings(DefaultRootWindow(d->disp), SUB_EWMH_NET_DESKTOP_NAMES, names, i);
	free(names);
}

 /**
	* Kill views 
	**/

void
subViewKill(void)
{
	SubView *v = NULL;

	assert(d->rv);

	printf("Removing all views\n");
	
	v = d->rv;
	while(v)
		{
			SubView *next = v->next;

			XUnmapWindow(d->disp, v->button);
			XDeleteContext(d->disp, v->button, 1);
			XDestroyWindow(d->disp, v->button);

			if(v->w)
				{
					subTileDelete(v->w);
					XDeleteContext(d->disp, v->w->frame, v->xid);
					XDestroyWindow(d->disp, v->w->frame);
					free(v->w);
				}
			if(v->regex) 
				{
					regfree(v->regex);
					free(v->regex);
				}
			free(v->name);
			free(v);					

			v = next;
		}

	XDestroyWindow(d->disp, d->bar.win);
	XDestroyWindow(d->disp, d->bar.views);
	XDestroyWindow(d->disp, d->bar.sublets);
}

 /**
	* Switch screen
	* @param w New active screen
	**/

void
subViewSwitch(SubView *v)
{
	int xid = 0;
	assert(v);

	if(d->cv != v)	
		{
			subWinUnmap(d->cv->w);
			if(!d->cv->w->tile->first && d->cv != d->rv) UntagView(d->cv);
			d->cv = v;
		}
	if(!v->w) TagView(v);

	XMapRaised(d->disp, d->bar.win);
	subWinMap(d->cv->w);

	/* EWMH: Desktops */
	xid = d->cv->xid - 1;
	subEwmhSetCardinals(DefaultRootWindow(d->disp), SUB_EWMH_NET_CURRENT_DESKTOP, (long *)&xid, 1);

	subViewConfigure();
	subViewRender();

	printf("Switching view (%s)\n", d->cv->name);
}
