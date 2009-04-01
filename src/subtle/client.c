
 /**
  * @package subtle
  *
  * @file Client functions
  * @copyright (c) 2005-2009 Christoph Kappel <unexist@dorfelite.net>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPL.
  * See the file COPYING.
  **/

#include <X11/Xatom.h>
#include "subtle.h"

/* Typedef {{{ */
typedef struct clientgravity_t
{
  int grav_right;
  int grav_down;
  int cells_x;
  int cells_y;
} ClientGravity;
/* }}} */

/* ClientMask {{{ */
static void
ClientMask(SubClient *c,
  XRectangle *r)
{
  XRectangle rect = { 0, 0, 0, 0 };
  
  /* Init rect */
  if(c)
    {
      rect        = c->rect;
      rect.width  = WINW(c);
      rect.height = WINH(c);
    }

  XDrawRectangle(subtle->disp, ROOT, subtle->gcs.invert, r->x - 1, r->y - 1,
    r->width - 3, r->height - 3);
} /* }}} */

/* ClientSnap {{{ */
static void
ClientSnap(SubClient *c,
  XRectangle *r)
{
  assert(c && r);

  if(SNAP > r->x) r->x = subtle->bw;
  else if(r->x > (SCREENW - WINW(c) - SNAP)) r->x = SCREENW - c->rect.width + subtle->bw;
  if(SNAP + subtle->th > r->y) r->y = subtle->bw + subtle->th;
  else if(r->y > (SCREENH - WINH(c) - SNAP)) r->y = SCREENH - c->rect.height + subtle->bw;
} /* }}} */

 /** subClientNew {{{
  * @brief Create new client
  * @param[in]  win  Main window of the new client
  * @return Returns a new #SubClient or \p NULL
  **/

SubClient *
subClientNew(Window win)
{
  int i, n = 0, gravity = SUB_GRAVITY_CENTER;
  long vid = 1337, supplied = 0;
  Window trans = 0;
  XWMHints *hints = NULL;
  XWindowAttributes attrs;
  Atom *protos = NULL;
  SubClient *c = NULL;

  assert(win);
  
  /* Create client */
  c = CLIENT(subSharedMemoryAlloc(1, sizeof(SubClient)));
  c->flags = SUB_TYPE_CLIENT;
  c->win   = win;

  /* Dimensions */
  c->rect.x      = 0;
  c->rect.y      = subtle->th;
  c->rect.width  = SCREENW;
  c->rect.height = SCREENH - subtle->th;

  /* Fetch name */
  if(!XFetchName(subtle->disp, c->win, &c->name))
    {
      free(c);

      return NULL;
    }

  /* Update client */
  subEwmhSetWMState(c->win, WithdrawnState);
  XSelectInput(subtle->disp, c->win, EVENTMASK);
  XSetWindowBorderWidth(subtle->disp, c->win, subtle->bw);
  XAddToSaveSet(subtle->disp, c->win);
  XSaveContext(subtle->disp, c->win, CLIENTID, (void *)c);

  /* Window attributes */
  XGetWindowAttributes(subtle->disp, c->win, &attrs);
  c->cmap = attrs.colormap;

  /* Size hints */
  if(!(c->hints = XAllocSizeHints()))
    subSharedLogError("Can't alloc memory. Exhausted?\n");

  XGetWMNormalHints(subtle->disp, c->win, c->hints, &supplied);
  if(0 < supplied) c->flags |= SUB_PREF_HINTS;
  else XFree(c->hints);

  /* Protocols */
  if(XGetWMProtocols(subtle->disp, c->win, &protos, &n))
    {
      for(i = 0; i < n; i++)
        {
          int id = subEwmhFind(protos[i]);

          switch(id)
            {
              case SUB_EWMH_WM_TAKE_FOCUS:    c->flags |= SUB_PREF_FOCUS; break;
              case SUB_EWMH_WM_DELETE_WINDOW: c->flags |= SUB_PREF_CLOSE; break;
            }
        }
      XFree(protos);
    }

  /* Window manager hints */
  if((hints = XGetWMHints(subtle->disp, c->win)))
    {
      if(hints->flags & XUrgencyHint) c->tags |= SUB_TAG_STICK;
      if(hints->flags & InputHint && hints->input) c->flags |= SUB_PREF_INPUT;
      if(hints->flags & WindowGroupHint) c->group = hints->window_group;

      XFree(hints);
    }

  /* Check for transient windows */
  if(XGetTransientForHint(subtle->disp, c->win, &trans))
    {
      SubClient *t = CLIENT(subSharedFind(trans, CLIENTID));
      if(t)
        {
          c->tags = t->tags; ///< Copy tags
          subClientToggle(c, SUB_STATE_STICK|SUB_STATE_FLOAT);
        }
    } 

  /* Tags */
  if(c->name)
    for(i = 0; i < subtle->tags->ndata; i++)
      {
        SubTag *t = TAG(subtle->tags->data[i]);

        if(t->preg && subSharedRegexMatch(t->preg, c->name)) c->tags |= (1L << (i + 1));
      }

  /* Special tags */
  if(!(c->tags & ~(SUB_TAG_FLOAT|SUB_TAG_FULL|SUB_TAG_STICK)))
    c->tags |= SUB_TAG_DEFAULT; ///< Ensure that there's at least one tag
  else
    {
      /* Properties */
      if(c->tags & SUB_TAG_FLOAT) subClientToggle(c, SUB_STATE_FLOAT);
      if(c->tags & SUB_TAG_FULL)  subClientToggle(c, SUB_STATE_FULL);
      if(c->tags & SUB_TAG_STICK) subClientToggle(c, SUB_STATE_STICK|SUB_STATE_FLOAT);

      /* Gravities */
      if(c->tags & SUB_TAG_TOP)
        {
          if(c->tags & SUB_TAG_LEFT)       gravity = SUB_GRAVITY_TOP_LEFT;
          else if(c->tags & SUB_TAG_RIGHT) gravity = SUB_GRAVITY_TOP_RIGHT;
          else                             gravity = SUB_GRAVITY_TOP;
        }
      else if(c->tags & SUB_TAG_BOTTOM)
        {
          if(c->tags & SUB_TAG_LEFT)       gravity = SUB_GRAVITY_BOTTOM_LEFT;
          else if(c->tags & SUB_TAG_RIGHT) gravity = SUB_GRAVITY_BOTTOM_RIGHT;
          else                             gravity = SUB_GRAVITY_BOTTOM;
        }
      else if(c->tags & SUB_TAG_LEFT)      gravity = SUB_GRAVITY_LEFT;
      else if(c->tags & SUB_TAG_RIGHT)     gravity = SUB_GRAVITY_RIGHT;
      else if(c->tags & SUB_TAG_CENTER)    gravity = SUB_GRAVITY_CENTER;
    }

  /* Gravities */
  c->gravities = (int *)subSharedMemoryAlloc(subtle->views->ndata, sizeof(int));
  for(i = 0; i < subtle->views->ndata; i++)
    c->gravities[i] = gravity;

  /* EWMH: Tags, gravity and desktop */
  subEwmhSetCardinals(c->win, SUB_EWMH_SUBTLE_WINDOW_TAGS, (long *)&c->tags, 1);
  subEwmhSetCardinals(c->win, SUB_EWMH_SUBTLE_WINDOW_GRAVITY, (long *)&c->gravity, 1);
  subEwmhSetCardinals(c->win, SUB_EWMH_NET_WM_DESKTOP, &vid, 1);

  printf("Adding client (%s)\n", c->name);
  subSharedLogDebug("new=client, name=%s, win=%#lx, supplied=%ld\n", c->name, win, supplied);

  return c;
} /* }}} */

 /** subClientConfigure {{{
  * @brief Send a configure request to client
  * @param[in]  c  A #SubClient
  **/

void
subClientConfigure(SubClient *c)
{
  XRectangle r = { 0 };
  XConfigureEvent ev;

  assert(c);

  /* Client size */
  r = c->rect;
  r.y     += subtle->th;
  r.width  = WINW(c);
  r.height = WINH(c);

  if(c->flags & SUB_STATE_FULL) 
    {
      r.x      = 0;
      r.y      = 0;
      r.width  = SCREENW;
      r.height = SCREENH;
    }

  /* Tell client new geometry */
  ev.type              = ConfigureNotify;
  ev.event             = c->win;
  ev.window            = c->win;
  ev.x                 = r.x;
  ev.y                 = r.y;
  ev.width             = r.width;
  ev.height            = r.height;
  ev.above             = None;
  ev.override_redirect = 0;
  ev.border_width      = 0;

  XMoveResizeWindow(subtle->disp, c->win, r.x, r.y, r.width, r.height);
  XSendEvent(subtle->disp, c->win, False, StructureNotifyMask, (XEvent *)&ev);

  subSharedLogDebug("client=%#lx, state=%c, x=%03d, y=%03d, width=%03d, height=%03d\n",
    c->win, c->flags & SUB_STATE_FLOAT ? 'f' : c->flags & SUB_STATE_FULL ? 'u' : 'n',
    r.x, r.y, r.width, r.height);
} /* }}} */

 /** subClientRender {{{
  * @brief Render client and redraw titlebar and borders
  * @param[in]  c  A #SubClient
  **/

void
subClientRender(SubClient *c)
{
  XSetWindowAttributes attrs;

  assert(c);

  attrs.border_pixel = subtle->windows.focus == c->win ? subtle->colors.focus : subtle->colors.norm;
  XChangeWindowAttributes(subtle->disp, c->win, CWBorderPixel, &attrs);

  /* Caption */
  XResizeWindow(subtle->disp, subtle->windows.caption, TEXTW(c->name), subtle->th);
  XClearWindow(subtle->disp, subtle->windows.caption);
  XDrawString(subtle->disp, subtle->windows.caption, subtle->gcs.font, 3, subtle->fy - 1,
    c->name, strlen(c->name));
} /* }}} */

 /** subClientFocus {{{
  * @brief Set focus to client
  * @param[in]  c  A #SubClient
  **/

void
subClientFocus(SubClient *c)
{
  assert(c);
     
  /**
   * Input | Focus | Type                | Input
   * ------+-------+---------------------+------------------------
   * 0     | 0     | No Input            | Never
   * 1     | 0     | Passive             | Set by wm (PointerRoot)
   * 1     | 1     | Locally active      | Set by wm (PointerRoot)
   * 0     | 1     | Globally active     | Sets the focus by itself
   **/

  /* Check client input focus type */
  if(!(c->flags & SUB_PREF_INPUT) && c->flags & SUB_PREF_FOCUS)
    {
      subEwmhMessage(c->win, c->win, SUB_EWMH_WM_PROTOCOLS, 
        subEwmhGet(SUB_EWMH_WM_TAKE_FOCUS), CurrentTime, 0, 0, 0);
    }   
  else XSetInputFocus(subtle->disp, c->win, RevertToNone, CurrentTime);

  subSharedLogDebug("Focus: win=%#lx, input=%d, focus=%d\n", c->win,
    !!(c->flags & SUB_PREF_INPUT), !!(c->flags & SUB_PREF_FOCUS));
} /* }}} */

 /** subClientDrag {{{
  * @brief Move and/or drag client
  * @param[in]  c     A #SubClient
  * @param[in]  mode  Drag/move mode
  **/

void
subClientDrag(SubClient *c,
  int mode)
{
  XEvent ev;
  Window win;
  unsigned int mask = 0;
  int loop = True, wx = 0, wy = 0, rx = 0, ry = 0;
  XRectangle r;
  SubClient *c2 = NULL;
  short *dirx = NULL, *diry = NULL, minx = 10, miny = 10, maxx = SCREENW, maxy = SCREENH;

  assert(c);
 
  /* Get window position on root window */
  XQueryPointer(subtle->disp, c->win, &win, &win, &rx, &ry, &wx, &wy, &mask);
  r.x      = rx - wx;
  r.y      = ry - wy;
  r.width  = c->rect.width;
  r.height = c->rect.height; 

  /* Directions and steppings  {{{ */
  switch(mode)
    {
      case SUB_DRAG_MOVE:
        dirx = &r.x;
        diry = &r.y;
        break;
      case SUB_DRAG_RESIZE_LEFT:
      case SUB_DRAG_RESIZE_RIGHT:
        dirx = (short *)&r.width;
        diry = (short *)&r.height;

        /* Respect size hints */
        if(c->flags & SUB_PREF_HINTS)
          {
            if(c->hints->flags & PResizeInc) ///< Resize hints
              {
                subtle->step = c->hints->width_inc;
                subtle->step = c->hints->height_inc;
              }
            if(c->hints->flags & PMinSize) ///< Min. size
              {
                minx = MINMAX(c->hints->min_width, MINW, 0);
                miny = MINMAX(c->hints->min_height, MINH, 0);
              }
            if(c->hints->flags & PMaxSize) ///< Max. size
              {
                maxx = c->hints->max_width;
                maxy = c->hints->max_height;
              }
         }
    } /* }}} */

  if(XGrabPointer(subtle->disp, c->win, True, 
    ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None,
    mode & (SUB_DRAG_RESIZE_LEFT|SUB_DRAG_RESIZE_RIGHT) ? subtle->cursors.resize : 
    subtle->cursors.move, CurrentTime)) return;

  XGrabServer(subtle->disp);
  if(mode & (SUB_DRAG_MOVE|SUB_DRAG_RESIZE_LEFT|SUB_DRAG_RESIZE_RIGHT)) 
    ClientMask(c, &r);

  while(loop) ///< Event loop
    {
      XMaskEvent(subtle->disp, 
        PointerMotionMask|ButtonReleaseMask|KeyPressMask|EnterWindowMask, &ev);
      switch(ev.type)
        {
          case EnterNotify:   win = ev.xcrossing.window; break; ///< Find destination window
          case ButtonRelease: loop = False;              break;
          case KeyPress: /* {{{ */
            if(mode & (SUB_DRAG_MOVE|SUB_DRAG_RESIZE_LEFT|SUB_DRAG_RESIZE_RIGHT))
              {
                KeySym sym = XKeycodeToKeysym(subtle->disp, ev.xkey.keycode, 0);
                ClientMask(c, &r);

                switch(sym)
                  {
                    case XK_Left:   *dirx -= subtle->step; break;
                    case XK_Right:  *dirx += subtle->step; break;
                    case XK_Up:     *diry -= subtle->step; break;
                    case XK_Down:   *diry += subtle->step; break;
                    case XK_Return: loop = False;   break;
                  }

                *dirx = MINMAX(*dirx, minx, maxx);
                *diry = MINMAX(*diry, miny, maxy);
              
                ClientMask(c, &r);
              }
            break; /* }}} */
          case MotionNotify: /* {{{ */
            if(mode & (SUB_DRAG_MOVE|SUB_DRAG_RESIZE_LEFT|SUB_DRAG_RESIZE_RIGHT)) /* Move/Resize {{{ */
              {
                ClientMask(c, &r);
          
                /* Calculate selection rect */
                switch(mode)
                  {
                    case SUB_DRAG_MOVE:
                      r.x = (rx - wx) - (rx - ev.xmotion.x_root);
                      r.y = (ry - wy) - (ry - ev.xmotion.y_root);

                      ClientSnap(c, &r);
                      break;
                    case SUB_DRAG_RESIZE_LEFT: 
                    case SUB_DRAG_RESIZE_RIGHT:
                      /* Stepping and snapping */
                      if(c->rect.width + ev.xmotion.x_root >= rx)
                        {
                          if(SUB_DRAG_RESIZE_LEFT == mode)
                            {
                              int rxmot = rx - ev.xmotion.x_root, rxstep = 0;

                              r.x     = MINMAX((rx - wx) - rxmot, 0, 
                                c->rect.x + c->rect.width - minx);
                              rxstep  = r.x % subtle->step;

                              r.width = MINMAX(c->rect.width + rxmot + rxstep, 
                                minx + rxstep, maxx);
                              r.x    -= rxstep;
                            }
                          else
                            {
                              r.width = MINMAX(c->rect.width + (ev.xmotion.x_root - rx),
                                minx, maxx);
                              r.width -= r.width % subtle->step;
                            }
                        }
                      if(c->rect.height + ev.xmotion.y_root >= ry)
                        {
                          r.height = MINMAX(c->rect.height + (ev.xmotion.y_root - ry),
                            miny, maxy);
                          r.height -= r.height % subtle->step;
                        }
                      break;
                  }  

                ClientMask(c, &r);
              } /* }}} */
            break; /* }}} */
        }
    }

  ClientMask(c2, &r); ///< Erase mask

  if(c->flags & SUB_STATE_FLOAT) 
    {
      r.y     -= (subtle->th + subtle->bw); ///< Border and bar height
      r.x     -= subtle->bw;
      c->rect  = r;

      subClientConfigure(c);
    }
  
  XUngrabPointer(subtle->disp, CurrentTime);
  XUngrabServer(subtle->disp);
} /* }}} */

  /** subClientGravitate {{{ 
   * @brief Calc rect in grid
   * @param[in]  c     A #SubClient
   * @param[in]  type  A #SubGravity
   **/

void
subClientGravitate(SubClient *c,
  int type)
{
  XRectangle workarea = { 0, 0, SCREENW, SCREENH - subtle->th };
  XRectangle slot = { 0 }, desired = { 0 }, current = { 0 };

  static const ClientGravity props[] =
  {
    { 0, 1, 1, 1 }, ///< Gravity unknown
    { 0, 1, 2, 2 }, ///< Gravity bottom left
    { 0, 1, 1, 2 }, ///< Gravity bottom
    { 1, 1, 2, 2 }, ///< Gravity bottom right
    { 0, 0, 2, 1 }, ///< Gravity left
    { 0, 0, 1, 1 }, ///< Gravity center
    { 1, 0, 2, 1 }, ///< Gravity right
    { 0, 0, 2, 2 }, ///< Gravity top left
    { 0, 0, 1, 2 }, ///< Gravity top
    { 1, 0, 2, 2 }, ///< Gravity top right
  }; 

  assert(c);

  /* Compute slot */
  slot.x      = workarea.x + props[type].grav_right * (workarea.width / props[type].cells_x);
  slot.y      = workarea.y + props[type].grav_down * (workarea.height / props[type].cells_y);
  slot.height = workarea.height / props[type].cells_y;
  slot.width  = workarea.width / props[type].cells_x;

  desired = slot;
  current = c->rect;

	if(desired.x == current.x && desired.width == current.width)
	  {
	    int height33 = workarea.height / 3;
	    int height66 = workarea.height - height33;
      int comp     = abs(workarea.height - 3 * height33); ///< Int rounding fix

	    if(2 == props[type].cells_y)
	      {
	       if(current.height == desired.height && current.y == desired.y)
	         {
	           slot.y      = workarea.y + props[type].grav_down * height33;
	           slot.height = height66;
        		}
      		else
        		{
              XRectangle rect33, rect66;

              rect33        = slot;
              rect33.y      = workarea.y + props[type].grav_down * height66;
              rect33.height = height33;

              rect66        = slot;
              rect66.y      = workarea.y + props[type].grav_down * height33;
              rect66.height = height66;

              if(current.height == rect66.height && current.y == rect66.y)
                {
                  slot.height = height33;
                  slot.y      = workarea.y + props[type].grav_down * height66;
	             }
	         }
	      }
	    else
	      {
          if(current.height == desired.height && current.y == desired.y)
            {
              slot.y      = workarea.y + height33;
              slot.height = height33 + comp;
            }
        }

      desired = slot;
    }    

  /* Update client rect */
  c->rect.x      = desired.x;
  c->rect.y      = desired.y;
  c->rect.width  = desired.width;
  c->rect.height = desired.height;
  c->gravity     = type;

  /* EWMH: Gravity */
  subEwmhSetCardinals(c->win, SUB_EWMH_SUBTLE_WINDOW_GRAVITY, (long *)&c->gravity, 1);
} /* }}} */

 /** subClientToggle {{{
  * @brief Toggle various states of client
  * @param[in]  c     A #SubClient
  * @param[in]  type  Toggle type
  **/

void
subClientToggle(SubClient *c,
  int type)
{
  assert(c);

  if(c->flags & type) 
    {
      c->flags  &= ~type;
      c->gravity = -1;

      if(type & SUB_STATE_FULL) /* {{{ */
        {
          XSetWindowBorderWidth(subtle->disp, c->win, subtle->bw);
          XReparentWindow(subtle->disp, c->win, subtle->cv->win, 0, 0);
        } /* }}} */
    }
  else 
    {
      c->flags |= type;

      if(type & SUB_STATE_FLOAT) /* {{{ */
        {
          if(c->flags & SUB_PREF_HINTS)
            {
              /* Default values */
              c->rect.width  = MINW; 
              c->rect.height = MINH;

              if(c->hints->flags & PMinSize) ///< Min size
                {
                  c->rect.width  = c->hints->min_width;
                  c->rect.height = c->hints->min_height;
                }
              if(c->hints->flags & PBaseSize) ///< Base size
                {
                  c->rect.width  = c->hints->base_width;
                  c->rect.height = c->hints->base_height;
                }
              if(c->hints->flags & (USSize|PSize)) ///< User/program size
                {
                  c->rect.width  = c->hints->width;
                  c->rect.height = c->hints->height;
                }

              /* Limit width/height to max. screen size*/
              c->rect.width  = MINMAX(c->rect.width, MINW, SCREENW) + 2 * subtle->bw;
              c->rect.height = MINMAX(c->rect.height, MINH, SCREENH - 
                (subtle->th + 2 * subtle->bw)) + 2 * subtle->bw;

              if(c->hints && c->hints->flags & (USPosition|PPosition)) ///< User/program pos
                {
                  c->rect.x = c->hints->x;
                  c->rect.y = c->hints->y;
                }
              else
                {
                  c->rect.x = (SCREENW - c->rect.width) / 2;
                  c->rect.y = (SCREENH - c->rect.height) / 2;
                }
            }
          else ///< Fallback
            {
              c->rect.width  = MINW + 2 * subtle->bw;
              c->rect.height = MINH + 2 * subtle->bw;
              c->rect.x      = (SCREENW - c->rect.width) / 2;
              c->rect.y      = (SCREENH - c->rect.height) / 2;
            }        
        } /* }}} */
      if(type & SUB_STATE_FULL) /* {{{ */
        {
          XSetWindowBorderWidth(subtle->disp, c->win, 0);
          XReparentWindow(subtle->disp, c->win, ROOT, 0, 0);
        } /* }}} */

      subClientConfigure(c);
    }

  subClientFocus(c);
} /* }}} */

 /** subClientPublish {{{
  * @brief Publish clients
  **/

void
subClientPublish(void)
{
  if(0 < subtle->clients->ndata )
    {
      int i;
      Window *wins = (Window *)subSharedMemoryAlloc(subtle->clients->ndata, sizeof(Window));

      for(i = 0; i < subtle->clients->ndata; i++) 
        wins[i] = CLIENT(subtle->clients->data[i])->win;

      /* EWMH: Client list and client list stacking */
      subEwmhSetWindows(ROOT, SUB_EWMH_NET_CLIENT_LIST, wins, subtle->clients->ndata);
      subEwmhSetWindows(ROOT, SUB_EWMH_NET_CLIENT_LIST_STACKING, wins, 
        subtle->clients->ndata);

      subSharedLogDebug("publish=client, clients=%d\n", subtle->clients->ndata);

      free(wins);
    }
} /* }}} */

 /** subClientKill {{{
  * @brief Send interested clients the close signal and/or kill it
  * @param[in]  c      A #SubClient
  * @param[in]  close  Close window
  **/

void
subClientKill(SubClient *c,
  int close)
{
  assert(c);

  printf("Killing client (%s)\n", c->name);

  /* Ignore further events and delete context */
  XSelectInput(subtle->disp, c->win, NoEventMask);
  XDeleteContext(subtle->disp, c->win, CLIENTID);

  /* Focus stuff */
  if(subtle->windows.focus == c->win) 
    {
      XUnmapWindow(subtle->disp, subtle->windows.caption);
      subtle->windows.focus = 0;
    }

  /* Close window */
  if(close && !(c->flags & SUB_STATE_DEAD))
    {
      if(c->flags & SUB_PREF_CLOSE) ///< Honor window preferences
        {
          subEwmhMessage(c->win, c->win, SUB_EWMH_WM_PROTOCOLS, 
            subEwmhGet(SUB_EWMH_WM_DELETE_WINDOW), CurrentTime, 0, 0, 0);
        }
      else XKillClient(subtle->disp, c->win);
    }

  if(c->gravities) free(c->gravities);
  if(c->hints) XFree(c->hints);
  if(c->name) XFree(c->name);
  free(c);

  subSharedLogDebug("kill=client\n");
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
