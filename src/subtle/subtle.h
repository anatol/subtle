
 /**
  * @package subtle
  *
  * @file Header file
  * @copyright Copyright (c) 2005-2009 Christoph Kappel <unexist@dorfelite.net>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPL.
  * See the file COPYING.
  **/

#ifndef SUBTLE_H
#define SUBTLE_H 1

/* Includes {{{ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <regex.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#include "config.h"
#include "shared.h"

#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif /* HAVE_SYS_INOTIFY */
/* }}} */

/* Macros {{{ */
#define FLAGS     int                                             ///< Flags
#define TAGS      int                                             ///< Tags

#define CLIENTID  1                                               ///< Client data id
#define VIEWID    2                                               ///< View data id
#define TRAYID    3                                               ///< tray data id
#define BUTTONID  4                                               ///< Button data id

#define MINW      100                                             ///< Client min. width
#define MINH      100                                             ///< Client min. height
#define SNAP      10                                              ///< Snapping threshold

#define RNGVIEW   1000                                            ///< Jump index range
#define RNGGRAV   2000                                            ///< Gravity index range

#define LENGTH(a)  (sizeof(a) / sizeof(a[0]))                     ///< Array length
#define TEXTW(s)   (strlen(s) * subtle->fx + 8)                   ///< Textwidth in pixel
#define WINW(c)    (c->rect.width - 2 * subtle->bw)               ///< Get real width
#define WINH(c)    (c->rect.height - 2 * subtle->bw)              ///< Get real height
#define ZERO(n)    (0 < n ? n : 1)                                ///< Prevent zero
#define MIN(a,b)   (a >= b ? b : a)                               ///< Minimum
#define MAX(a,b)   (a >= b ? a : b)                               ///< Maximum

#define MINMAX(val,min,max) \
  (min && val < min ? min : max && val > max ? max : val)         ///< Value min/max

#define EVENTMASK \
  (StructureNotifyMask|PropertyChangeMask|EnterWindowMask|FocusChangeMask)
#define DRAGMASK \
  (PointerMotionMask|ButtonReleaseMask|KeyPressMask|EnterWindowMask|FocusChangeMask)
#define GRABMASK \
  (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)

#define VISUAL \
  DefaultVisual(subtle->disp, DefaultScreen(subtle->disp))        ///< Default visual
#define COLORMAP \
  DefaultColormap(subtle->disp, DefaultScreen(subtle->disp))      ///< Default colormap
#define VISIBLE(v,c) \
  (v->tags & c->tags || c->flags & SUB_STATE_STICK)               ///< Visible on view
#define SCREENW \
  DisplayWidth(subtle->disp, DefaultScreen(subtle->disp))         ///< Get screen width
#define SCREENH \
  DisplayHeight(subtle->disp, DefaultScreen(subtle->disp))        ///< Get screen height

#define ROOT       DefaultRootWindow(subtle->disp)                ///< Root window
#define SCREEN     DefaultScreen(subtle->disp)                    ///< Default screen

/* Casts */
#define ARRAY(a)  ((SubArray *)a)                                 ///< Cast to SubArray
#define CLIENT(c) ((SubClient *)c)                                ///< Cast to SubClient
#define DATA(d)   ((SubData)d)                                    ///< Cast to SubData
#define GRAB(g)   ((SubGrab *)g)                                  ///< Cast to SubGrab
#define RECT(r)   ((XRectangle *)r)                               ///< Cast to XRectangle
#define SUBLET(s) ((SubSublet *)s)                                ///< Cast to SubSublet
#define SUBTLE(s) ((SubSubtle *)s)                                ///< Cast to SubSubtle
#define TAG(t)    ((SubTag *)t)                                   ///< Cast to SubTag
#define TRAY(t)   ((SubTray *)t)                                  ///< Cast to SubTray
#define VIEW(v)   ((SubView *)v)                                  ///< Cast to SubView

/* Gravities */
#define SUB_GRAVITY_UNKNOWN            0                          ///< Gravity unknown
#define SUB_GRAVITY_BOTTOM_LEFT        1                          ///< Gravity bottom left
#define SUB_GRAVITY_BOTTOM             2                          ///< Gravity bottom
#define SUB_GRAVITY_BOTTOM_RIGHT       3                          ///< Gravity bottom right
#define SUB_GRAVITY_LEFT               4                          ///< Gravity left
#define SUB_GRAVITY_CENTER             5                          ///< Gravity center
#define SUB_GRAVITY_RIGHT              6                          ///< Gravity right
#define SUB_GRAVITY_TOP_LEFT           7                          ///< Gravity top left
#define SUB_GRAVITY_TOP                8                          ///< Gravity top
#define SUB_GRAVITY_TOP_RIGHT          9                          ///< Gravity top right

/* XEmbed messages */
#define XEMBED_EMBEDDED_NOTIFY         0
#define XEMBED_WINDOW_ACTIVATE         1
#define XEMBED_WINDOW_DEACTIVATE       2
#define XEMBED_REQUEST_FOCUS           3
#define XEMBED_FOCUS_IN                4
#define XEMBED_FOCUS_OUT               5
#define XEMBED_FOCUS_NEXT              6
#define XEMBED_FOCUS_PREV              7
#define XEMBED_MODALITY_ON             10
#define XEMBED_MODALITY_OFF            11
#define XEMBED_REGISTER_ACCELERATOR    12
#define XEMBED_UNREGISTER_ACCELERATOR  13
#define XEMBED_ACTIVATE_ACCELERATOR    14

/* Details for XEMBED_FOCUS_IN */
#define XEMBED_FOCUS_CURRENT           0
#define XEMBED_FOCUS_FIRST             1
#define XEMBED_FOCUS_LAST              2

/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED                  (1 << 0)
/* }}} */

/* Flags {{{ */
/* Data types */
#define SUB_TYPE_CLIENT               (1L << 1)                   ///< Client
#define SUB_TYPE_GRAB                 (1L << 2)                   ///< Grab
#define SUB_TYPE_SUBLET               (1L << 3)                   ///< Sublet
#define SUB_TYPE_TAG                  (1L << 4)                   ///< Tag
#define SUB_TYPE_TRAY                 (1L << 5)                   ///< Tray
#define SUB_TYPE_VIEW                 (1L << 6)                   ///< View

/* Client states */
#define SUB_STATE_FULL                (1L << 10)                  ///< Fullscreen window
#define SUB_STATE_FLOAT               (1L << 11)                  ///< Floating window
#define SUB_STATE_STICK               (1L << 12)                  ///< Stick window
#define SUB_STATE_DEAD                (1L << 13)                  ///< Dead window

/* Client preferences */
#define SUB_PREF_INPUT                (1L << 15)                  ///< Active/passive focus-model
#define SUB_PREF_FOCUS                (1L << 16)                  ///< Send focus message
#define SUB_PREF_CLOSE                (1L << 17)                  ///< Send close message
#define SUB_PREF_POS                  (1L << 18)                  ///< Client position
#define SUB_PREF_SIZE                 (1L << 19)                  ///< Client size

/* Data types */
#define SUB_DATA_STRING               (1L << 10)                  ///< String data
#define SUB_DATA_NUM                  (1L << 11)                  ///< Num data
#define SUB_DATA_NIL                  (1L << 12)                  ///< Nil data
#define SUB_DATA_INOTIFY              (1L << 13)                  ///< Inotify data

/* Grab types */
#define SUB_GRAB_KEY                  (1L << 15)                  ///< Key grab
#define SUB_GRAB_MOUSE                (1L << 16)                  ///< Mouse grab  
#define SUB_GRAB_EXEC                 (1L << 17)                  ///< Exec an app
#define SUB_GRAB_PROC                 (1L << 18)                  ///< Grab with proc
#define SUB_GRAB_JUMP                 (1L << 19)                  ///< Jump to view
#define SUB_GRAB_GRAVITY              (1L << 20)                  ///< Gravity type
#define SUB_GRAB_WINDOW_MOVE          (1L << 21)                  ///< Resize window
#define SUB_GRAB_WINDOW_RESIZE        (1L << 22)                  ///< Move window
#define SUB_GRAB_WINDOW_FLOAT         (1L << 23)                  ///< Toggle float
#define SUB_GRAB_WINDOW_FULL          (1L << 24)                  ///< Toggle full
#define SUB_GRAB_WINDOW_STICK         (1L << 25)                  ///< Toggle stock
#define SUB_GRAB_WINDOW_RAISE         (1L << 26)                  ///< Raise window
#define SUB_GRAB_WINDOW_LOWER         (1L << 27)                  ///< Lower window
#define SUB_GRAB_WINDOW_NEXT          (1L << 28)                  ///< Focus next window
#define SUB_GRAB_WINDOW_PREV          (1L << 29)                  ///< Focus prev window
#define SUB_GRAB_WINDOW_KILL          (1L << 30)                  ///< Kill window

/* Drag states */
#define SUB_DRAG_START                (1L << 10)                  ///< Drag start
#define SUB_DRAG_MOVE                 (1L << 11)                  ///< Drag move
#define SUB_DRAG_SWAP                 (1L << 12)                  ///< Drag swap
#define SUB_DRAG_RESIZE               (1L << 13)                  ///< Drag resize

/* Fixed tags */
#define SUB_TAG_DEFAULT               (1L << 1)                   ///< Default tag
#define SUB_TAG_FLOAT                 (1L << 2)                   ///< Float tag
#define SUB_TAG_FULL                  (1L << 3)                   ///< Full tag
#define SUB_TAG_STICK                 (1L << 4)                   ///< Stick tag
#define SUB_TAG_TOP                   (1L << 5)                   ///< Top tag
#define SUB_TAG_BOTTOM                (1L << 6)                   ///< Bottom tag
#define SUB_TAG_LEFT                  (1L << 7)                   ///< Left tag
#define SUB_TAG_RIGHT                 (1L << 8)                   ///< Right tag
#define SUB_TAG_CENTER                (1L << 9)                   ///< Center tag

/* Clear masks */
#define SUB_TAG_CLEAR                  9                          ///< Clear mask
#define SUB_GRAB_CLEAR                 14                         ///< Clear mask
/* }}} */

/* Typedefs {{{ */
typedef struct subarray_t /* {{{ */
{
  int   ndata;                                                    ///< Array data count
  void **data;                                                    ///< Array data
} SubArray; /* }}} */

typedef struct subclient_t /* {{{ */
{
  FLAGS      flags;                                               ///< Client flags
  char       *name, *klass;                                       ///< Client name, wmclass

  TAGS       tags;                                                ///< Client tags
  Window     win;                                                 ///< Client window
  Colormap   cmap;                                                ///< Client colormap
  XRectangle rect;                                                ///< Client rect

  float      minr, maxr;                                          ///< Client ratios
  int        basew, baseh, minw, minh, maxw, maxh, incw, inch;    ///< Client sizes
  int        gravity, *gravities;                                 ///< Client gravities
} SubClient; /* }}} */

typedef enum subewmh_t /* {{{ */
{
  /* ICCCM */
  SUB_EWMH_WM_NAME,                                               ///< Name of window
  SUB_EWMH_WM_CLASS,                                              ///< Class of window
  SUB_EWMH_WM_STATE,                                              ///< Window state
  SUB_EWMH_WM_PROTOCOLS,                                          ///< Supported protocols 
  SUB_EWMH_WM_TAKE_FOCUS,                                         ///< Send focus messages
  SUB_EWMH_WM_DELETE_WINDOW,                                      ///< Send close messages
  SUB_EWMH_WM_NORMAL_HINTS,                                       ///< Window normal hints
  SUB_EWMH_WM_SIZE_HINTS,                                         ///< Window size hints

  /* EWMH */
  SUB_EWMH_NET_SUPPORTED,                                         ///< Supported states
  SUB_EWMH_NET_CLIENT_LIST,                                       ///< List of clients
  SUB_EWMH_NET_CLIENT_LIST_STACKING,                              ///< List of clients
  SUB_EWMH_NET_NUMBER_OF_DESKTOPS,                                ///< Total number of views
  SUB_EWMH_NET_DESKTOP_NAMES,                                     ///< Names of the views
  SUB_EWMH_NET_DESKTOP_GEOMETRY,                                  ///< Desktop geometry
  SUB_EWMH_NET_DESKTOP_VIEWPORT,                                  ///< Viewport of the view
  SUB_EWMH_NET_CURRENT_DESKTOP,                                   ///< Number of current view
  SUB_EWMH_NET_ACTIVE_WINDOW,                                     ///< Focus window
  SUB_EWMH_NET_WORKAREA,                                          ///< Workarea of the views
  SUB_EWMH_NET_SUPPORTING_WM_CHECK,                               ///< Check for compliant window manager
  SUB_EWMH_NET_VIRTUAL_ROOTS,                                     ///< List of virtual destops
  SUB_EWMH_NET_CLOSE_WINDOW,                                      ///< Close window             
  SUB_EWMH_NET_RESTACK_WINDOW,                                    ///< Change window stacking

  SUB_EWMH_NET_WM_NAME,                            
  SUB_EWMH_NET_WM_PID,                                            ///< PID of client
  SUB_EWMH_NET_WM_DESKTOP,                                        ///< Desktop client is on
  SUB_EWMH_NET_SHOWING_DESKTOP,                                   ///< Showing desktop mode 

  SUB_EWMH_NET_WM_STATE,                                          ///< Window state
  SUB_EWMH_NET_WM_STATE_HIDDEN,                                   ///< Hidden window
  SUB_EWMH_NET_WM_STATE_FULLSCREEN,                               ///< Fullscreen window
  SUB_EWMH_NET_WM_STATE_ABOVE,                                    ///< Floating window
  SUB_EWMH_NET_WM_STATE_STICKY,                                   ///< Urgent window

  SUB_EWMH_NET_WM_WINDOW_TYPE,
  SUB_EWMH_NET_WM_WINDOW_TYPE_DESKTOP,
  SUB_EWMH_NET_WM_WINDOW_TYPE_NORMAL,
  SUB_EWMH_NET_WM_WINDOW_TYPE_DIALOG,

  SUB_EWMH_NET_WM_ALLOWED_ACTIONS,
  SUB_EWMH_NET_WM_ACTION_MOVE,
  SUB_EWMH_NET_WM_ACTION_RESIZE,
  SUB_EWMH_NET_WM_ACTION_FULLSCREEN,
  SUB_EWMH_NET_WM_ACTION_CHANGE_DESKTOP,
  SUB_EWMH_NET_WM_ACTION_CLOSE,

  /* Tray */
  SUB_EWMH_NET_SYSTEM_TRAY_OPCODE,                                ///< Tray messages
  SUB_EWMH_NET_SYSTEM_TRAY_MESSAGE_DATA,                          ///< Tray message data
  SUB_EWMH_NET_SYSTEM_TRAY_SELECTION,                             ///< Tray selection

  /* Misc */
  SUB_EWMH_UTF8,                                                  ///< String encoding
  SUB_EWMH_MANAGER,                                               ///< Selection manager

  /* XEmbed */
  SUB_EWMH_XEMBED,                                                ///< XEmbed
  SUB_EWMH_XEMBED_INFO,                                           ///< XEmbed info

  /* subtle */
  SUB_EWMH_SUBTLE_WINDOW_TAG,                                     ///< subtle window tag
  SUB_EWMH_SUBTLE_WINDOW_UNTAG,                                   ///< subtle window untag
  SUB_EWMH_SUBTLE_WINDOW_TAGS,                                    ///< subtle window tags
  SUB_EWMH_SUBTLE_WINDOW_GRAVITY,                                 ///< subtle window gravity
  SUB_EWMH_SUBTLE_TAG_NEW,                                        ///< subtle tag new
  SUB_EWMH_SUBTLE_TAG_LIST,                                       ///< subtle tag list
  SUB_EWMH_SUBTLE_TAG_KILL,                                       ///< subtle tag kill
  SUB_EWMH_SUBTLE_VIEW_NEW,                                       ///< subtle view new
  SUB_EWMH_SUBTLE_VIEW_KILL,                                      ///< subtle view kill
  SUB_EWMH_SUBTLE_SUBLET_UPDATE,                                  ///< subtle sublet update
  SUB_EWMH_SUBTLE_SUBLET_LIST,                                    ///< subtle sublet list
  SUB_EWMH_SUBTLE_SUBLET_KILL,                                    ///< subtle sublet kill
  SUB_EWMH_SUBTLE_RELOAD,                                         ///< subtle reload config

  SUB_EWMH_TOTAL
} SubEwmh; /* }}} */

typedef union subdata_t /* {{{ */
{
  unsigned long num;                                              ///< Data num
  char          *string;                                          ///< Data string
} SubData; /* }}} */

typedef struct subgrab_t /* {{{ */
{
  FLAGS           flags;                                          ///< Grab flags
  unsigned int    code, mod;                                      ///< Grab code modifier

  union subdata_t data;                                           ///< Grab data
} SubGrab; /* }}} */

typedef struct subsublet_t /* {{{ */
{
  FLAGS              flags;                                       ///< Sublet flags
  char               *name;                                       ///< Sublet name

#ifdef HAVE_SYS_INOTIFY_H
  char               *path;                                       ///< Sublet inotify path
#endif /* HAVE_SYS_INOTIFY */  

  unsigned long      recv;                                        ///< Sublet Ruby receiver
  int                width;                                       ///< Sublet width
  time_t             time, interval;                              ///< Sublet update time, interval time

  struct subsublet_t *next;                                       ///< Sublet next sibling
  union  subdata_t   data;                                        ///< Sublet data
} SubSublet; /* }}} */

typedef struct subsubtle_t /* {{{ */
{
  int                th, bw, fx, fy, step;                        ///< Subtle properties

  Display            *disp;                                       ///< Subtle Xorg display
  XFontStruct        *xfs;                                        ///< Subtle font

  struct subview_t   *cv;                                         ///< Subtle current view
  struct subclient_t *cc;                                         ///< Subtle current client

  struct subsublet_t *sublet;                                     ///< Subtle first sublet

  struct subarray_t  *clients;                                    ///< Subtle clients
  struct subarray_t  *grabs;                                      ///< Subtle grabs
  struct subarray_t  *sublets;                                    ///< Subtle sublets
  struct subarray_t  *tags;                                       ///< Subtle tags
  struct subarray_t  *trays;                                      ///< Subtle trays
  struct subarray_t  *views;                                      ///< Subtle views

#ifdef DEBUG
  int                debug;                                       ///< Subtle debug
#endif /* DEBUG */

#ifdef HAVE_SYS_INOTIFY_H
  int                notify;                                      ///< Subtle inotify descriptor
#endif /* HAVE_SYS_INOTIFY_H */

  struct
  {
    Window           bar, views, caption, tray, sublets, focus; 
  } windows;                                                      ///< Subtle windows

  struct
  {
    unsigned long    font, border, norm, focus, bg;                            
  } colors;                                                       ///< Subtle colors

  struct
  {
    GC               font, stipple, invert;                  
  } gcs;                                                          ///< Subtle graphic contexts

  struct
  {
    Cursor           arrow, move, resize;                                
  } cursors;                                                      ///< Subtle cursors
} SubSubtle; /* }}} */

typedef struct subtag_t /* {{{ */
{
  FLAGS    flags;                                                 ///< Tag flags
  char    *name;                                                  ///< Tag name
  regex_t *preg;                                                  ///< Tag regex
} SubTag; /* }}} */

typedef struct subtray_t /* {{{ */
{
  FLAGS  flags;                                                   ///< Tray flags
  char   *name;                                                   ///< Tray name

  Window win;                                                     ///< Tray window
  int    width;                                                   ///< Tray width
} SubTray; /* }}} */

typedef struct subview_t /* {{{ */
{
  FLAGS  flags;                                                   ///< View flags
  char   *name;                                                   ///< View name
  TAGS   tags;                                                    ///< View tags

  Window button;                                                  ///< View button
  int    width;                                                   ///< View width
} SubView; /* }}} */

extern SubSubtle *subtle;
/* }}} */

/* array.c {{{ */
SubArray *subArrayNew(void);                                      ///< Create array
void subArrayPush(SubArray *a, void *e);                          ///< Push element to array
void subArrayRemove(SubArray *a, void *e);                        ///< Remove element from array
void *subArrayGet(SubArray *a, int idx);                          ///< Get element
int subArrayIndex(SubArray *a, void *e);                          ///< Find array id of element
void subArraySort(SubArray *a,                                    ///< Sort array with given compare function 
  int(*compar)(const void *a, const void *b));
void subArrayClear(SubArray *a, int clean);                       ///< Delete all elements
void subArrayKill(SubArray *a, int clean);                        ///< Kill array with all elements
/* }}} */

/* client.c {{{ */
SubClient *subClientNew(Window win);                              ///< Create client
void subClientConfigure(SubClient *c);                            ///< Send configure request
void subClientRender(SubClient *c);                               ///< Render client
void subClientFocus(SubClient *c);                                ///< Focus client
void subClientDrag(SubClient *c, int mode);                       ///< Move/drag client
void subClientUpdate(int vid);                                    ///< Update clients
void subClientSetGravity(SubClient *c, int type);                 ///< Set client gravity
void subClientSetSize(SubClient *c);                              ///< Set client site
void subClientToggle(SubClient *c, int type);                     ///< Toggle client state
void subClientPublish(void);                                      ///< Publish all clients
void subClientKill(SubClient *c, int close);                      ///< Kill client
/* }}} */

/* display.c {{{ */
void subDisplayInit(const char *display);                         ///< Create display
void subDisplayConfigure(void);                                   ///< Configure display
void subDisplayScan(void);                                        ///< Scan root window
void subDisplayPublish(void);                                     ///< Publish display
void subDisplayFinish(void);                                      ///< Kill display
/* }}} */

/* event.c {{{ */
void subEventLoop(void);                                          ///< Event loop
/* }}} */

/* ewmh.c  {{{ */
void subEwmhInit(void);                                           ///< Init atoms/hints
Atom subEwmhGet(SubEwmh e);                                       ///< Get atom
SubEwmh subEwmhFind(Atom atom);                                   ///< Find atom id
char *subEwmhGetProperty(Window win, 
  Atom type, SubEwmh e, unsigned long *size);                     ///< Get property
long subEwmhGetWMState(Window win);                               ///< Get window WM state
void subEwmhSetWindows(Window win, SubEwmh e, 
  Window *values, int size);                                      ///< Set window properties
void subEwmhSetCardinals(Window win, SubEwmh e,
  long *values, int size);                                        ///< Set cardinal properties
void subEwmhSetString(Window win, SubEwmh e, 
  char *value);                                                   ///< Set string property
void subEwmhSetStrings(Window win, SubEwmh e,                     ///< Set string properties
  char **values, int size);
void subEwmhSetWMState(Window win, long state);                   ///< Set window WM state
int subEwmhMessage(Window dst, Window win, SubEwmh e, 
  long data0, long data1, long data2, long data3, 
  long data4);                                                    ///< Send message
/* }}} */

/* grab.c {{{ */
void subGrabInit(void);                                           ///< Init keymap
SubGrab *subGrabNew(const char *chain, int type,                  ///< Create grab
  SubData data);
KeySym subGrabGet(void);                                          ///< Get grab
SubGrab *subGrabFind(int code, unsigned int mod);                 ///< Find grab
void subGrabSet(Window win);                                      ///< Grab window
void subGrabUnset(Window win);                                    ///< Ungrab window
int subGrabCompare(const void *a, const void *b);                 ///< Compare grabs
void subGrabKill(SubGrab *g);                                     ///< Kill grab
/* }}} */

/* ruby.c {{{ */
void subRubyInit(void);                                           ///< Init Ruby stack 
void subRubyLoadConfig(const char *file);                         ///< Load config file
void subRubyLoadSublets(const char *path);                        ///< Load sublets
void subRubyLoadSublext(void);                                    ///< Load sublext
void subRubyRun(void *script);                                    ///< Run Ruby script
void subRubyFinish(void);                                         ///< Kill Ruby stack
/* }}} */

/* sublet.c {{{ */
SubSublet *subSubletNew(void);                                    ///< Create sublet
void subSubletUpdate(void);                                       ///< Update sublet bar
void subSubletRender(void);                                       ///< Render sublet
int subSubletCompare(const void *a, const void *b);               ///< Compare two sublets
void subSubletPublish(void);                                      ///< Publish sublets
void subSubletKill(SubSublet *s, int unlink);                     ///< Kill sublet
/* }}} */

/* tag.c {{{ */
void subTagInit(void);                                            ///< Init tags
SubTag *subTagNew(char *name, char *regex);                       ///< Create tag
void subTagPublish(void);                                         ///< Publish tags
void subTagKill(SubTag *t);                                       ///< Delete tag
/* }}} */

/* tray.c {{{ */
SubTray *subTrayNew(Window win);                                  ///< Create tray 
void subTrayConfigure(SubTray *t);                                ///< Configure tray
void subTrayFocus(SubTray * t);                                   ///< Focus tray
void subTrayUpdate(void);                                         ///< Update tray bar
void subTraySelect(void);                                         ///< Get selection
void subTrayFocus(SubTray *t);                                    ///< Focus tray
void subTraySetState(SubTray *t);                                 ///< Set state
void subTrayKill(SubTray *t);                                     ///< Delete tray
/* }}} */

/* view.c {{{ */
SubView *subViewNew(char *name, char *tags);                      ///< Create view
void subViewConfigure(SubView *v);                                ///< Configure view
void subViewUpdate(void);                                         ///< Update views
void subViewRender(void);                                         ///< Render views
void subViewJump(SubView *v);                                     ///< Jump to view
void subViewPublish(void);                                        ///< Publish views
void subViewKill(SubView *v);                                     ///< Kill view
/* }}} */
#endif /* SUBTLE_H */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
