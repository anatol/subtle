
 /**
  * @package subtlext
  *
  * @file Header file
  * @copyright Copyright (c) 2005-2012 Christoph Kappel <unexist@subforge.org>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPLv2.
  * See the file COPYING for details.
  **/

#ifndef SUBTLEXT_H
#define SUBTLEXT_H 1

/* Includes {{{ */
#include <ruby.h>
#include "shared.h"
/* }}} */

/* Macros {{{ */
#define CHAR2SYM(name) ID2SYM(rb_intern(name))
#define SYM2CHAR(sym)  rb_id2name(SYM2ID(sym))

#define GET_ATTR(owner,name,value) \
  if(NIL_P(value = rb_iv_get(owner, name))) return Qnil;

#define ROOT DefaultRootWindow(display)
/* }}} */

/* Flags {{{ */
#define SUB_TYPE_CLIENT  0           ///< Client
#define SUB_TYPE_GRAVITY 1           ///< Gravity
#define SUB_TYPE_VIEW    2           ///< View
#define SUB_TYPE_TAG     3           ///< Tag
#define SUB_TYPE_TRAY    4           ///< Tray
#define SUB_TYPE_SCREEN  5           ///< Screen
#define SUB_TYPE_SUBLET  6           ///< Sublet
/* }}} */

/* Typedefs {{{ */
extern Display *display;
extern VALUE mod;

#ifdef DEBUG
extern int debug;
#endif /* DEBUG */
/* }}} */

/* client.c {{{ */
/* Singleton */
VALUE subextClientSingSelect(VALUE self);                            ///< Select client
VALUE subextClientSingFind(VALUE self, VALUE value);                 ///< Find client
VALUE subextClientSingFirst(VALUE self, VALUE value);                ///< Find first client
VALUE subextClientSingCurrent(VALUE self);                           ///< Get current client
VALUE subextClientSingVisible(VALUE self);                           ///< Get all visible clients
VALUE subextClientSingList(VALUE self);                              ///< Get all clients
VALUE subextClientSingRecent(VALUE self);                            ///< Get recent active clients

/* Class */
VALUE subextClientInstantiate(Window win);                           ///< Instantiate client
VALUE subextClientInit(VALUE self, VALUE win);                       ///< Create client
VALUE subextClientUpdate(VALUE self);                                ///< Update client
VALUE subextClientViewList(VALUE self);                              ///< Get views clients is on
VALUE subextClientFlagsAskFull(VALUE self);                          ///< Is client fullscreen
VALUE subextClientFlagsAskFloat(VALUE self);                         ///< Is client floating
VALUE subextClientFlagsAskStick(VALUE self);                         ///< Is client stick
VALUE subextClientFlagsAskResize(VALUE self);                        ///< Is client resize
VALUE subextClientFlagsAskUrgent(VALUE self);                        ///< Is client urgent
VALUE subextClientFlagsAskZaphod(VALUE self);                        ///< Is client zaphod
VALUE subextClientFlagsAskFixed(VALUE self);                         ///< Is client fixed
VALUE subextClientFlagsAskBorderless(VALUE self);                    ///< Is client borderless
VALUE subextClientFlagsToggleFull(VALUE self);                       ///< Toggle fullscreen
VALUE subextClientFlagsToggleFloat(VALUE self);                      ///< Toggle floating
VALUE subextClientFlagsToggleStick(VALUE self);                      ///< Toggle stick
VALUE subextClientFlagsToggleResize(VALUE self);                     ///< Toggle resize
VALUE subextClientFlagsToggleUrgent(VALUE self);                     ///< Toggle urgent
VALUE subextClientFlagsToggleZaphod(VALUE self);                     ///< Toggle zaphod
VALUE subextClientFlagsToggleFixed(VALUE self);                      ///< Toggle fixed
VALUE subextClientFlagsToggleBorderless(VALUE self);                 ///< Toggle borderless
VALUE subextClientFlagsWriter(VALUE self, VALUE value);              ///< Set multiple flags
VALUE subextClientRestackRaise(VALUE self);                          ///< Raise client
VALUE subextClientRestackLower(VALUE self);                          ///< Lower client
VALUE subextClientAskAlive(VALUE self);                              ///< Is client alive
VALUE subextClientGravityReader(VALUE self);                         ///< Get client gravity
VALUE subextClientGravityWriter(VALUE self, VALUE value);            ///< Set client gravity
VALUE subextClientGeometryReader(VALUE self);                        ///< Get client geometry
VALUE subextClientGeometryWriter(int argc, VALUE *argv,
  VALUE self);                                                    ///< Set client geometry
VALUE subextClientScreenReader(VALUE self);                          ///< Get client screen
VALUE subextClientResizeWriter(VALUE self, VALUE value);             ///< Set Client resize
VALUE subextClientToString(VALUE self);                              ///< Client to string
VALUE subextClientKill(VALUE self);                                  ///< Kill client
/* }}} */

/* color.c {{{ */
unsigned long subextColorPixel(VALUE red, VALUE green,
  VALUE blue, XColor *xcolor);                                    ///< Get pixel value
VALUE subextColorInstantiate(unsigned long pixel);                   ///< Instantiate color
VALUE subextColorInit(int argc, VALUE *argv, VALUE self);            ///< Create new color
VALUE subextColorToHex(VALUE self);                                  ///< Convert to hex string
VALUE subextColorToArray(VALUE self);                                ///< Color to array
VALUE subextColorToHash(VALUE self);                                 ///< Color to hash
VALUE subextColorToString(VALUE self);                               ///< Convert to string
VALUE subextColorOperatorPlus(VALUE self, VALUE value);              ///< Concat string
VALUE subextColorEqual(VALUE self, VALUE other);                     ///< Whether objects are equal
VALUE subextColorEqualTyped(VALUE self, VALUE other);                ///< Whether objects are equal typed
/* }}} */

/* geometry.c {{{ */
VALUE subextGeometryInstantiate(int x, int y, int width,
  int height);                                                    ///< Instantiate geometry
void subextGeometryToRect(VALUE self, XRectangle *r);                ///< Geometry to rect
VALUE subextGeometryInit(int argc, VALUE *argv, VALUE self);         ///< Create new geometry
VALUE subextGeometryToArray(VALUE self);                             ///< Geometry to array
VALUE subextGeometryToHash(VALUE self);                              ///< Geometry to hash
VALUE subextGeometryToString(VALUE self);                            ///< Geometry to string
VALUE subextGeometryEqual(VALUE self, VALUE other);                  ///< Whether objects are equal
VALUE subextGeometryEqualTyped(VALUE self, VALUE other);             ///< Whether objects are equal typed
/* }}} */

/* gravity.c {{{ */
/* Singleton */
VALUE subextGravitySingFind(VALUE self, VALUE value);                ///< Find gravity
VALUE subextGravitySingFirst(VALUE self, VALUE value);               ///< Find first gravity
VALUE subextGravitySingList(VALUE self);                             ///< Get all gravities

/* Class */
VALUE subextGravityInstantiate(char *name);                          ///< Instantiate gravity
VALUE subextGravityInit(int argc, VALUE *argv, VALUE self);          ///< Create new gravity
VALUE subextGravitySave(VALUE self);                                 ///< Save gravity
VALUE subextGravityClients(VALUE self);                              ///< List clients with gravity
VALUE subextGravityGeometryFor(VALUE self, VALUE value);             ///< Get geometry gravity for screen
VALUE subextGravityGeometryReader(VALUE self);                       ///< Get geometry gravity
VALUE subextGravityGeometryWriter(int argc, VALUE *argv, VALUE self);///< Get geometry gravity
VALUE subextGravityTilingWriter(VALUE self, VALUE value);            ///< Set gravity tiling
VALUE subextGravityToString(VALUE self);                             ///< Gravity to string
VALUE subextGravityToSym(VALUE self);                                ///< Gravity to symbol
VALUE subextGravityKill(VALUE self);                                 ///< Kill gravity
/* }}} */

/* icon.c {{{ */
VALUE subextIconAlloc(VALUE self);                                   ///< Allocate icon
VALUE subextIconInit(int argc, VALUE *argv, VALUE self);             ///< Init icon
VALUE subextIconDrawPoint(int argc, VALUE *argv, VALUE self);        ///< Draw a point
VALUE subextIconDrawLine(int argc, VALUE *argv, VALUE self);         ///< Draw a line
VALUE subextIconDrawRect(int argc, VALUE *argv, VALUE self);         ///< Draw a rect
VALUE subextIconCopyArea(int argc, VALUE *argv, VALUE self);         ///< Copy icon area
VALUE subextIconClear(int argc, VALUE *argv, VALUE self);            ///< Clear icon
VALUE subextIconAskBitmap(VALUE self);                               ///< Whether icon is bitmap
VALUE subextIconToString(VALUE self);                                ///< Convert to string
VALUE subextIconOperatorPlus(VALUE self, VALUE value);               ///< Concat string
VALUE subextIconOperatorMult(VALUE self, VALUE value);               ///< Concat string
VALUE subextIconEqual(VALUE self, VALUE other);                      ///< Whether objects are equal
VALUE subextIconEqualTyped(VALUE self, VALUE other);                 ///< Whether objects are equal typed
/* }}} */

/* screen.c {{{ */
/* Singleton */
VALUE subextScreenSingFind(VALUE self, VALUE id);                    ///< Find screen
VALUE subextScreenSingList(VALUE self);                              ///< Get all screens
VALUE subextScreenSingCurrent(VALUE self);                           ///< Get current screen

/* Class */
VALUE subextScreenInstantiate(int id);                               ///< Instantiate screen
VALUE subextScreenInit(VALUE self, VALUE id);                        ///< Create new screen
VALUE subextScreenUpdate(VALUE self);                                ///< Update screen
VALUE subextScreenJump(VALUE self);                                  ///< Jump to this screen
VALUE subextScreenViewReader(VALUE self);                            ///< Get screen view
VALUE subextScreenViewWriter(VALUE self, VALUE value);               ///< Set screen view
VALUE subextScreenAskCurrent(VALUE self);                            ///< Whether screen is current
VALUE subextScreenToString(VALUE self);                              ///< Screen to string
/* }}} */

/* sublet.c {{{ */
/* Singleton */
VALUE subextSubletSingFind(VALUE self, VALUE value);                 ///< Find sublet
VALUE subextSubletSingFirst(VALUE self, VALUE value);                ///< Find first sublet
VALUE subextSubletSingList(VALUE self);                              ///< Get all sublets

/* Class */
VALUE subextSubletInit(VALUE self, VALUE name);                      ///< Create sublet
VALUE subextSubletUpdate(VALUE self);                                ///< Update sublet
VALUE subextSubletSend(VALUE self, VALUE value);                     ///< Send data to sublet
VALUE subextSubletVisibilityShow(VALUE self);                        ///< Show sublet
VALUE subextSubletVisibilityHide(VALUE self);                        ///< Hide sublet
VALUE subextSubletGeometryReader(VALUE self);                        ///< Get sublet geometry
VALUE subextSubletToString(VALUE self);                              ///< Sublet to string
VALUE subextSubletKill(VALUE self);                                  ///< Kill sublet
/* }}} */

/* subtle.c {{{ */
/* Singleton */
VALUE subextSubtleSingDisplayReader(VALUE self);                     ///< Get display
VALUE subextSubtleSingDisplayWriter(VALUE self, VALUE display);      ///< Set display
VALUE subextSubtleSingAskRunning(VALUE self);                        ///< Is subtle running
VALUE subextSubtleSingSelect(VALUE self);                            ///< Select window
VALUE subextSubtleSingRender(VALUE self);                            ///< Render panels
VALUE subextSubtleSingReload(VALUE self);                            ///< Reload config and sublets
VALUE subextSubtleSingRestart(VALUE self);                           ///< Restart subtle
VALUE subextSubtleSingQuit(VALUE self);                              ///< Quit subtle
VALUE subextSubtleSingColors(VALUE self);                            ///< Get colors
VALUE subextSubtleSingFont(VALUE self);                              ///< Get font
VALUE subextSubtleSingSpawn(VALUE self, VALUE cmd);                  ///< Spawn command
/* }}} */

/* subtlext.c {{{ */
void subextSubtlextConnect(char *display_string);                    ///< Connect to display
void subextSubtlextBacktrace(void);                                  ///< Print ruby backtrace
VALUE subextSubtlextConcat(VALUE str1, VALUE str2);                  ///< Concat strings
VALUE subextSubtlextParse(VALUE value, char *buf,
  int len, int *flags);                                           ///< Parse arguments
VALUE subextSubtlextOneOrMany(VALUE value, VALUE prev);              ///< Return one or many
VALUE subextSubtlextManyToOne(VALUE value);                          ///< Return one from many
Window *subextSubtlextWindowList(char *prop_name, int *size);        ///< Get window list
int subextSubtlextFindString(char *prop_name, char *source,
  char **name, int flags);                                        ///< Find string id
VALUE subextSubtlextFindObjects(char *prop_name, char *class_name,
  char *source, int flags, int first);                            ///< Find objects
VALUE subextSubtlextFindWindows(char *prop_name, char *class_name,
  char *source, int flags, int first);                            ///< Find objects
VALUE subextSubtlextFindObjectsGeometry(char *prop_name,
  char *class_name, char *source, int flags, int first);          ///< Find objects with geometries
/* }}} */

/* tag.c {{{ */
/* Singleton */
VALUE subextTagSingFind(VALUE self, VALUE value);                    ///< Find tag
VALUE subextTagSingFirst(VALUE self, VALUE value);                   ///< Find first tag
VALUE subextTagSingVisible(VALUE self);                              ///< Get all visible tags
VALUE subextTagSingList(VALUE self);                                 ///< Get all tags

/* Class */
VALUE subextTagInstantiate(char *name);                              ///< Instantiate tag
VALUE subextTagInit(VALUE self, VALUE name);                         ///< Create tag
VALUE subextTagSave(VALUE self);                                     ///< Save tag
VALUE subextTagClients(VALUE self);                                  ///< Get clients with tag
VALUE subextTagViews(VALUE self);                                    ///< Get views with tag
VALUE subextTagToString(VALUE self);                                 ///< Tag to string
VALUE subextTagKill(VALUE self);                                     ///< Kill tag
/* }}} */

/* tray.c {{{ */
/* Singleton */
VALUE subextTraySingFind(VALUE self, VALUE name);                    ///< Find tray
VALUE subextTraySingFirst(VALUE self, VALUE name);                   ///< Find first tray
VALUE subextTraySingList(VALUE self);                                ///< Get all trays

/* Class */
VALUE subextTrayInstantiate(Window win);                             ///< Instantiate tray
VALUE subextTrayInit(VALUE self, VALUE win);                         ///< Create tray
VALUE subextTrayUpdate(VALUE self);                                  ///< Update tray
VALUE subextTrayToString(VALUE self);                                ///< Tray to string
VALUE subextTrayKill(VALUE self);                                    ///< Kill tray
/* }}} */

/* view.c {{{ */
/* Singleton */
VALUE subextViewSingFind(VALUE self, VALUE name);                    ///< Find view
VALUE subextViewSingFirst(VALUE self, VALUE name);                   ///< Find first view
VALUE subextViewSingCurrent(VALUE self);                             ///< Get current view
VALUE subextViewSingVisible(VALUE self);                             ///< Get all visible views
VALUE subextViewSingList(VALUE self);                                ///< Get all views

/* Class */
VALUE subextViewInstantiate(char *name);                             ///< Instantiate view
VALUE subextViewInit(VALUE self, VALUE name);                        ///< Create view
VALUE subextViewUpdate(VALUE self);                                  ///< Update view
VALUE subextViewSave(VALUE self);                                    ///< Save view
VALUE subextViewClients(VALUE self);                                 ///< Get clients of view
VALUE subextViewJump(VALUE self);                                    ///< Jump to view
VALUE subextViewSelectNext(VALUE self);                              ///< Select next view
VALUE subextViewSelectPrev(VALUE self);                              ///< Select next view
VALUE subextViewAskCurrent(VALUE self);                              ///< Whether view the current
VALUE subextViewIcon(VALUE self);                                    ///< View icon if any
VALUE subextViewToString(VALUE self);                                ///< View to string
VALUE subextViewKill(VALUE self);                                    ///< Kill view
/* }}} */

/* window.c {{{ */
/* Singleton */
VALUE subextWindowSingOnce(VALUE self, VALUE geometry);              ///< Run window once

/* Class */
VALUE subextWindowInstantiate(VALUE geometry);                       ///< Instantiate window
VALUE subextWindowDispatcher(int argc, VALUE *argv, VALUE self);     ///< Window dispatcher
VALUE subextWindowAlloc(VALUE self);                                 ///< Allocate window
VALUE subextWindowInit(VALUE self, VALUE geometry);                  ///< Init window
VALUE subextWindowSubwindow(VALUE self, VALUE geometry);             ///< Create a subwindow
VALUE subextWindowNameWriter(VALUE self, VALUE value);               ///< Set name
VALUE subextWindowFontWriter(VALUE self, VALUE value);               ///< Set font
VALUE subextWindowFontYReader(VALUE self);                           ///< Get y offset of font
VALUE subextWindowFontHeightReader(VALUE self);                      ///< Get height of font
VALUE subextWindowFontWidth(VALUE self, VALUE string);               ///< Get string width for font
VALUE subextWindowForegroundWriter(VALUE self, VALUE value);         ///< Set foreground
VALUE subextWindowBackgroundWriter(VALUE self, VALUE value);         ///< Set background
VALUE subextWindowBorderColorWriter(VALUE self, VALUE value);        ///< Set border color
VALUE subextWindowBorderSizeWriter(VALUE self, VALUE value);         ///< Set border size
VALUE subextWindowGeometryReader(VALUE self);                        ///< Get geometry
VALUE subextWindowGeometryWriter(VALUE self, VALUE value);           ///< Set geometry
VALUE subextWindowOn(int argc, VALUE *argv, VALUE self);             ///< Add event handler
VALUE subextWindowDrawPoint(int argc, VALUE *argv, VALUE self);      ///< Draw a point
VALUE subextWindowDrawLine(int argc, VALUE *argv, VALUE self);       ///< Draw a line
VALUE subextWindowDrawRect(int argc, VALUE *argv, VALUE self);       ///< Draw a rect
VALUE subextWindowDrawText(int arcg, VALUE *argv, VALUE self);       ///< Draw text
VALUE subextWindowDrawIcon(int arcg, VALUE *argv, VALUE self);       ///< Draw icon
VALUE subextWindowClear(int argc, VALUE *argv, VALUE self);          ///< Clear area or window
VALUE subextWindowRedraw(VALUE self);                                ///< Redraw window
VALUE subextWindowRaise(VALUE self);                                 ///< Raise window
VALUE subextWindowLower(VALUE self);                                 ///< Lower window
VALUE subextWindowShow(VALUE self);                                  ///< Show window
VALUE subextWindowHide(VALUE self);                                  ///< Hide window
VALUE subextWindowAskHidden(VALUE self);                             ///< Whether window is hidden
VALUE subextWindowKill(VALUE self);                                  ///< Kill window
/* }}} */

#endif /* SUBTLEXT_H */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
