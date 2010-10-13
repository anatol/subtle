 /**
  * @package subtlext
  *
  * @file Client functions
  * @copyright (c) 2005-2010 Christoph Kappel <unexist@dorfelite.net>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPL.
  * See the file COPYING.
  **/

#include "subtlext.h"

/* ClientToggle {{{ */
VALUE
ClientToggle(VALUE self,
  char *type,
  int flag)
{
  Window win = 0;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Find window */
  if((win = NUM2LONG(rb_iv_get(self, "@win"))))
    {
      int flags = 0;
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      flags     = FIX2INT(rb_iv_get(self, "@flags"));
      data.l[1] = XInternAtom(display, type, False);

      /* Toggle flag */
      if(flags & flag) flags &= ~flag;
      else flags |= flag;

      rb_iv_set(self, "@flags", INT2FIX(flags));

      subSharedMessage(display, win, "_NET_WM_STATE", data, True);
    }
  else rb_raise(rb_eStandardError, "Failed toggling client");

  return Qnil;
} /* }}} */

/* ClientSelect {{{ */
VALUE
ClientSelect(VALUE self,
  int type)
{
  int i, size = 0, match = (1L << 30), score = 0, *gravity1 = NULL;
  Window win = None, *clients = NULL, *views = NULL, found = None;
  VALUE client = Qnil;
  unsigned long *cv = NULL, *tags1 = NULL;
  XRectangle geometry1 = { 0 }, geometry2 = { 0 };

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Fetch data */
  win     = NUM2LONG(rb_iv_get(self, "@win"));
  clients = subSubtlextList("_NET_CLIENT_LIST", &size);
  views   = (Window *)subSharedPropertyGet(display,
    DefaultRootWindow(display), XA_WINDOW,
    XInternAtom(display, "_NET_VIRTUAL_ROOTS", False), NULL);
  cv      = (unsigned long *)subSharedPropertyGet(display,
    DefaultRootWindow(display), XA_CARDINAL,
    XInternAtom(display, "_NET_CURRENT_DESKTOP", False), NULL);

  if(clients && cv)
    {
      tags1 = (unsigned long *)subSharedPropertyGet(display,
        views[*cv], XA_CARDINAL,
        XInternAtom(display, "SUBTLE_WINDOW_TAGS", False), NULL);
      gravity1 = (int *)subSharedPropertyGet(display, win,
        XA_CARDINAL, XInternAtom(display, "SUBTLE_WINDOW_GRAVITY", False),
        NULL);

      subSharedPropertyGeometry(display, win, &geometry1);

      /* Iterate once to find a client score-based */
      for(i = 0; i < size; i++)
        {
          unsigned long *tags2 = (unsigned long *)subSharedPropertyGet(display,
            clients[i], XA_CARDINAL,
            XInternAtom(display, "SUBTLE_WINDOW_TAGS", False), NULL);
          int *gravity2 = (int *)subSharedPropertyGet(display, clients[i],
            XA_CARDINAL, XInternAtom(display, "SUBTLE_WINDOW_GRAVITY",
              False), NULL);

          /* Check if there are common tags */
          if(win != clients[i] && *gravity1 != *gravity2 &&
              *tags1 & *tags2)
            {
              subSharedPropertyGeometry(display, clients[i], &geometry2);

              if(match > (score = subSharedMatch(type, &geometry1, &geometry2)))
                {
                  match = score;
                  found = clients[i];
                }

            }

          free(tags2);
          free(gravity2);
        }

      if(found)
        {
          client = subClientInstantiate(found);

          subClientUpdate(client);
        }

      free(tags1);
      free(gravity1);
      free(clients);
      free(cv);
    }

  return client;
} /* }}} */

/* ClientRestack {{{ */
VALUE
ClientRestack(VALUE self,
  int detail)
{
  VALUE win = rb_iv_get(self, "@win");
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  if(RTEST(win))
    {
      data.l[0] = 2; ///< Claim to be a pager
      data.l[1] = NUM2LONG(win);
      data.l[2] = detail;

      subSharedMessage(display, DefaultRootWindow(display), "_NET_RESTACK_WINDOW", data, True);
    }
  else rb_raise(rb_eStandardError, "Failed restacking client");

  return Qnil;
} /* }}} */

/* subClientInstantiate {{{ */
VALUE
subClientInstantiate(Window win)
{
  VALUE klass = Qnil, client = Qnil;

  /* Create new instance */
  klass  = rb_const_get(mod, rb_intern("Client"));
  client = rb_funcall(klass, rb_intern("new"), 1, LONG2NUM(win));

  return client;
} /* }}} */

/* subClientInit {{{ */
/*
 * call-seq: new(win) -> Subtlext::Client
 *
 * Create a new Client object
 *
 *  client = Subtlext::Client.new(123456789)
 *  => #<Subtlext::Client:xxx>
 */

VALUE
subClientInit(VALUE self,
  VALUE win)
{
  if(!FIXNUM_P(win) && T_BIGNUM != rb_type(win))
    rb_raise(rb_eArgError, "Invalid value type");

  rb_iv_set(self, "@id",       Qnil);
  rb_iv_set(self, "@win",      win);
  rb_iv_set(self, "@name",     Qnil);
  rb_iv_set(self, "@instance", Qnil);
  rb_iv_set(self, "@klass",    Qnil);
  rb_iv_set(self, "@role",     Qnil);
  rb_iv_set(self, "@geometry", Qnil);
  rb_iv_set(self, "@gravity",  Qnil);
  rb_iv_set(self, "@screen",   Qnil);
  rb_iv_set(self, "@flags",    Qnil);
  rb_iv_set(self, "@hidden",   Qfalse);

  subSubtlextConnect(); ///< Implicit open connection

  return self;
} /* }}} */

/* subClientFind {{{ */
/*
 * call-seq: find(value) -> Subtlext::Client or nil
 *           [value]     -> Subtlext::Client or nil
 *
 * Find Client by a given value which can be of following type:
 *
 * [fixnum] Array id
 * [string] Match against WM_NAME or WM_CLASS
 * [hash]   With one of following keys: :title, :name, :class, :gravity
 * [symbol] Either :current for current Client or :all for an array
 *
 *  Subtlext::Client.find("subtle")
 *  => #<Subtlext::Client:xxx>
 *
 *  Subtlext::Client[:name => "subtle"]
 *  => #<Subtlext::Client:xxx>
 *
 *  Subtlext::Client["subtle"]
 *  => nil
 */

VALUE
subClientFind(VALUE self,
  VALUE value)
{
  int id = 0, flags = 0;
  Window win = None;
  VALUE parsed = Qnil, client = Qnil;
  char *name = NULL, buf[50] = { 0 };

  subSubtlextConnect(); ///< Implicit open connection

  /* Check object type */
  if(T_SYMBOL == rb_type(parsed = subSubtlextParse(
      value, buf, sizeof(buf), &flags)))
    {
      if(CHAR2SYM("all") == parsed)
        return subClientAll(Qnil);
      else if(CHAR2SYM("current") == parsed)
        return subClientCurrent(Qnil);
    }

  /* Find client */
  if(-1 != (id = subSubtlextFindWindow("_NET_CLIENT_LIST",
      buf, NULL, &win, flags)))
    {
      if(!NIL_P((client = subClientInstantiate(win))))
        {
          rb_iv_set(client, "@id", INT2FIX(id));

          subClientUpdate(client);
        }

      free(name);
    }

  return client;
} /* }}} */

/* subClientCurrent {{{ */
/*
 * call-seq: current -> Subtlext::Client
 *
 * Get current active Client
 *
 *  Subtlext::Client.current
 *  => #<Subtlext::Client:xxx>
 */

VALUE
subClientCurrent(VALUE self)
{
  VALUE client = Qnil;
  unsigned long *focus = NULL;

  subSubtlextConnect(); ///< Implicit open connection

  if((focus = (unsigned long *)subSharedPropertyGet(display,
      DefaultRootWindow(display), XA_WINDOW,
      XInternAtom(display, "_NET_ACTIVE_WINDOW", False), NULL)))
    {
      client = subClientInstantiate(*focus);

      if(!NIL_P(client)) subClientUpdate(client);

      free(focus);
    }
  else rb_raise(rb_eStandardError, "Failed getting current client");

  return client;
} /* }}} */

/* subClientAll {{{ */
/*
 * call-seq: all -> Array
 *
 * Get Array of all Client
 *
 *  Subtlext::Client.all
 *  => [#<Subtlext::Client:xxx>, #<Subtlext::Client:xxx>]
 *
 *  Subtlext::Client.all
 *  => []
 */

VALUE
subClientAll(VALUE self)
{
  int i, size = 0;
  Window *clients = NULL;
  VALUE meth = Qnil, klass = Qnil, array = Qnil, client = Qnil;

  subSubtlextConnect(); ///< Implicit open connection

  /* Fetch data */
  meth    = rb_intern("new");
  klass   = rb_const_get(mod, rb_intern("Client"));
  clients = subSubtlextList("_NET_CLIENT_LIST", &size);
  array   = rb_ary_new2(size);

  /* Populate array */
  if(clients)
    {
      for(i = 0; i < size; i++)
        {
          if(!NIL_P(client = rb_funcall(klass, meth, 1, LONG2NUM(clients[i]))))
            {
              subClientUpdate(client);
              rb_ary_push(array, client);
            }
        }

      free(clients);
    }

  return array;
} /* }}} */

/* subClientUpdate {{{ */
/*
 * call-seq: update -> nil
 *
 * Update Client properties
 *
 *  client.update
 *  => nil
 */

VALUE
subClientUpdate(VALUE self)
{
  VALUE win = rb_iv_get(self, "@win");

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Check object type */
  if(T_FIXNUM == rb_type(win) || T_BIGNUM == rb_type(win))
    {
      int id = 0;
      char buf[20] = { 0 };

      snprintf(buf, sizeof(buf), "%#lx", NUM2LONG(win));
      if(-1 != (id = subSubtlextFindWindow("_NET_CLIENT_LIST", buf,
          NULL, NULL, (SUB_MATCH_NAME|SUB_MATCH_CLASS))))
        {
          int *flags = NULL;
          char *wmname = NULL, *wminstance = NULL, *wmclass = NULL, *role = NULL;

          subSharedPropertyClass(display, NUM2LONG(win), &wminstance, &wmclass);
          subSharedPropertyName(display, NUM2LONG(win), &wmname, wmclass);

          flags = (int *)subSharedPropertyGet(display, NUM2LONG(win), XA_CARDINAL,
            XInternAtom(display, "SUBTLE_WINDOW_FLAGS", False), NULL);
          role = subSharedPropertyGet(display, NUM2LONG(win), XA_STRING,
            XInternAtom(display, "WM_WINDOW_ROLE", False), NULL);

          /* Set properties */
          rb_iv_set(self, "@id",       INT2FIX(id));
          rb_iv_set(self, "@flags",    INT2FIX(*flags));
          rb_iv_set(self, "@name",     rb_str_new2(wmname));
          rb_iv_set(self, "@instance", rb_str_new2(wminstance));
          rb_iv_set(self, "@klass",    rb_str_new2(wmclass));
          rb_iv_set(self, "@role",     role ? rb_str_new2(role) : Qnil);

          /* Set to nil for on demand loading */
          rb_iv_set(self, "@geometry", Qnil);
          rb_iv_set(self, "@gravity",  Qnil);
          rb_iv_set(self, "@screen",   Qnil);

          free(flags);
          free(wmname);
          free(wminstance);
          free(wmclass);
          if(role) free(role);
        }
      else rb_raise(rb_eStandardError, "Failed finding client");
    }
  else rb_raise(rb_eArgError, "Unknown value type");

  return Qnil;
} /* }}} */

/* subClientViewList {{{ */
/*
 * call-seq: views -> Array
 *
 * Get Array of View Client is on
 *
 *  client.views
 *  => [#<Subtlext::View:xxx>, #<Subtlext::View:xxx>]
 *
 *  client.views
 *  => nil
 */

VALUE
subClientViewList(VALUE self)
{
  int i, size = 0;
  char **names = NULL;
  Window *views = NULL;
  VALUE win = Qnil, array = Qnil, method = Qnil, klass = Qnil;
  unsigned long *tags1 = NULL, *flags = NULL;

  win     = rb_iv_get(self, "@win");
  method  = rb_intern("new");
  klass   = rb_const_get(mod, rb_intern("View"));
  array   = rb_ary_new2(size);
  names   = subSharedPropertyStrings(display, DefaultRootWindow(display),
    XInternAtom(display, "_NET_DESKTOP_NAMES", False), &size);
  views   = (Window *)subSharedPropertyGet(display, DefaultRootWindow(display),
    XA_WINDOW, XInternAtom(display, "_NET_VIRTUAL_ROOTS", False), NULL);
  tags1   = (unsigned long *)subSharedPropertyGet(display, NUM2LONG(win),
    XA_CARDINAL, XInternAtom(display, "SUBTLE_WINDOW_TAGS", False), NULL);
  flags   = (unsigned long *)subSharedPropertyGet(display, NUM2LONG(win),
    XA_CARDINAL, XInternAtom(display, "SUBTLE_WINDOW_FLAGS", False), NULL);

  if(names && views)
    {
      for(i = 0; i < size; i++)
        {
          unsigned long *tags2 = (unsigned long *)subSharedPropertyGet(display,
            views[i], XA_CARDINAL,
            XInternAtom(display, "SUBTLE_WINDOW_TAGS", False), NULL);

          /* Check if there are common tags or window is stick */
          if((tags2 && *tags1 & *tags2) || *flags & SUB_EWMH_STICK)
            {
              VALUE v = rb_funcall(klass, method, 1, rb_str_new2(names[i]));

              rb_iv_set(v, "@id",  INT2FIX(i));
              rb_iv_set(v, "@win", LONG2NUM(views[i]));
              rb_ary_push(array, v);
            }

          if(tags2) free(tags2);
        }

      XFreeStringList(names);
      free(views);
    }

  if(tags1) free(tags1);
  if(flags) free(flags);

  return array;
} /* }}} */

/* subClientStateFullAsk {{{ */
/*
 * call-seq: is_full? -> true or false
 *
 * Check if a Client is fullscreen
 *
 *  client.is_full?
 *  => true
 *
 *  client.is_full?
 *  => false
 */

VALUE
subClientStateFullAsk(VALUE self)
{
  rb_check_frozen(self);

  return FIX2INT(rb_iv_get(self, "@flags")) & SUB_EWMH_FULL ? Qtrue : Qfalse;
} /* }}} */

/* subClientStateFloatAsk {{{ */
/*
 * call-seq: is_float? -> true or false
 *
 * Check if a Client is floating
 *
 *  client.is_float?
 *  => true
 *
 *  client.is_float?
 *  => false
 */

VALUE
subClientStateFloatAsk(VALUE self)
{
  rb_check_frozen(self);

  return FIX2INT(rb_iv_get(self, "@flags")) & SUB_EWMH_FLOAT ? Qtrue : Qfalse;
} /* }}} */

/* subClientStateStickAsk {{{ */
/*
 * call-seq: is_stick? -> true or false
 *
 * Check if a client is sticky
 *
 *  client.is_stick?
 *  => true
 *
 *  client.is_stick?
 *  => false
 */

VALUE
subClientStateStickAsk(VALUE self)
{
  rb_check_frozen(self);

  return FIX2INT(rb_iv_get(self, "@flags")) & SUB_EWMH_STICK ? Qtrue : Qfalse;
} /* }}} */

/* subClientToggleFull {{{ */
/*
 * call-seq: toggle_full -> nil
 *
 * Toggle Client fullscreen state
 *
 *  client.toggle_full
 *  => nil
 */

VALUE
subClientToggleFull(VALUE self)
{
  return ClientToggle(self, "_NET_WM_STATE_FULLSCREEN", SUB_EWMH_FULL);
} /* }}} */

/* subClientToggleFloat {{{ */
/*
 * call-seq: toggle_float -> nil
 *
 * Toggle Client floating state
 *
 *  client.toggle_float
 *  => nil
 */

VALUE
subClientToggleFloat(VALUE self)
{
  return ClientToggle(self, "_NET_WM_STATE_ABOVE", SUB_EWMH_FLOAT);
} /* }}} */

/* subClientToggleStick {{{ */
/*
 * call-seq: toggle_stick -> nil
 *
 * Toggle Client sticky state
 *
 *  client.toggle_stick
 *  => nil
 */

VALUE
subClientToggleStick(VALUE self)
{
  return ClientToggle(self, "_NET_WM_STATE_STICKY", SUB_EWMH_STICK);
} /* }}} */

/* subClientRestackRaise {{{ */
/*
 * call-seq: raise -> nil
 *
 * Raise Client window
 *
 *  client.raise
 *  => nil
 */

VALUE
subClientRestackRaise(VALUE self)
{
  return ClientRestack(self, Above);
} /* }}} */

/* subClientRestackLower {{{ */
/*
 * call-seq: lower -> nil
 *
 * Raise Client window
 *
 *  client.raise
 *  => nil
 */

VALUE
subClientRestackLower(VALUE self)
{
  return ClientRestack(self, Below);
} /* }}} */

/* subClientSelectUp {{{ */
/*
 * call-seq: up -> Subtlext::Client or nil
 *
 * Get Client above
 *
 *  client.up
 *  => #<Subtlext::Client:xxx>
 *
 *  client.up
 *  => nil
 */

VALUE
subClientSelectUp(VALUE self)
{
  return ClientSelect(self, SUB_WINDOW_UP);
} /* }}} */

/* subClientSelectLeft {{{ */
/*
 * call-seq: left -> Subtlext::Client or nil
 *
 * Get Client left
 *
 *  client.left
 *  => #<Subtlext::Client:xxx>
 *
 *  client.left
 *  => nil
 */

VALUE
subClientSelectLeft(VALUE self)
{
  return ClientSelect(self, SUB_WINDOW_LEFT);
} /* }}} */

/* subClientSelectRight {{{ */
/*
 * call-seq: right -> Subtlext::Client or nil
 *
 * Get Client right
 *
 *  client.right
 *  => #<Subtlext::Client:xxx>
 *
 *  client.right
 *  => nil
 */

VALUE
subClientSelectRight(VALUE self)
{
  return ClientSelect(self, SUB_WINDOW_RIGHT);
} /* }}} */

/* subClientSelectDown {{{ */
/*
 * call-seq: down -> Subtlext::Client or nil
 *
 * Get Client below
 *
 *  client.down
 *  => #<Subtlext::Client:xxx>
 *
 *  client.down
 *  => nil
 */

VALUE
subClientSelectDown(VALUE self)
{
  return ClientSelect(self, SUB_WINDOW_DOWN);
} /* }}} */

/* subClientAliveAsk {{{ */
/*
 * call-seq: alive? -> true or false
 *
 * Check if client is alive
 *
 *  client.alive?
 *  => true
 *
 *  client.alive?
 *  => false
 */

VALUE
subClientAliveAsk(VALUE self)
{
  VALUE ret = Qfalse, name = rb_iv_get(self, "@name");

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Just find the client */
  if(RTEST(name) && -1 != subSubtlextFindWindow("_NET_CLIENT_LIST",
      RSTRING_PTR(name), NULL, NULL, (SUB_MATCH_NAME|SUB_MATCH_CLASS)))
    ret = Qtrue;

  return ret;
} /* }}} */

/* subClientGravityReader {{{ */
/*
 * call-seq: gravity -> Subtlext::Gravity
 *
 * Get Client Gravity
 *
 *  client.gravity
 *  => #<Subtlext::Gravity:xxx>
 */

VALUE
subClientGravityReader(VALUE self)
{
  Window win = None;
  VALUE gravity = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Load on demand */
  if(NIL_P((gravity = rb_iv_get(self, "@gravity"))) &&
      (win = NUM2LONG(rb_iv_get(self, "@win"))))
    {
      int *id = NULL;
      char buf[5] = { 0 };

      /* Get gravity */
      if((id = (int *)subSharedPropertyGet(display, win, XA_CARDINAL,
          XInternAtom(display, "SUBTLE_WINDOW_GRAVITY", False), NULL)))
        {
          /* Create gravity */
          snprintf(buf, sizeof(buf), "%d", *id);
          gravity = subGravityInstantiate(buf);

          subGravityUpdate(gravity);

          rb_iv_set(self, "@gravity", gravity);

          free(id);
        }
    }

  return gravity;
} /* }}} */

/* subClientGravityWriter {{{ */
/*
 * call-seq: gravity=(fixnum) -> nil
 *           gravity=(symbol) -> nil
 *           gravity=(object) -> nil
 *
 * Set Client Gravity
 *
 *  client.gravity = 0
 *  => nil
 *
 *  client.gravity = :center
 *  => nil
 *
 *  client.gravity = Subtlext::Gravity[0]
 *  => nil
 *
 */

VALUE
subClientGravityWriter(VALUE self,
  VALUE value)
{
  VALUE gravity = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Check instance type */
  if(rb_obj_is_instance_of(value, rb_const_get(mod, rb_intern("Gravity"))))
    gravity = value;
  else gravity = subGravityFind(Qnil, value);

  /* Set gravity */
  if(Qnil != gravity)
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = FIX2LONG(rb_iv_get(self,    "@id"));
      data.l[1] = FIX2LONG(rb_iv_get(gravity, "@id"));

      subSharedMessage(display, DefaultRootWindow(display), "SUBTLE_WINDOW_GRAVITY", data, True);

      rb_iv_set(self, "@gravity", gravity);
    }
  else rb_raise(rb_eArgError, "Failed setting value type `%s'", rb_obj_classname(value));

  return Qnil;
} /* }}} */

/* subClientScreenReader {{{ */
/*
 * call-seq: screen -> Subtlext::Screen
 *
 * Get screen Client is on as Screen object
 *
 *  client.screen
 *  => #<Subtlext::Screen:xxx>
 */

VALUE
subClientScreenReader(VALUE self)
{
  Window win = None;
  VALUE screen = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Load on demand */
  if(NIL_P((screen = rb_iv_get(self, "@screen"))) &&
      (win = NUM2LONG(rb_iv_get(self, "@win"))))
    {
      int *id = NULL;

      /* Collect data */
      id     = (int *)subSharedPropertyGet(display, win, XA_CARDINAL,
        XInternAtom(display, "SUBTLE_WINDOW_SCREEN", False), NULL);
      screen = subScreenInstantiate(*id);

      if(!NIL_P(screen)) subScreenUpdate(screen);

      rb_iv_set(self, "@screen", screen);

      free(id);
    }

  return screen;
} /* }}} */

/* subClientScreenWriter {{{ */
/*
 * call-seq: screen=(screen) -> nil
 *
 * Set client screen
 *
 *  client.screen = 0
 *  => nil
 *
 *  client.screen = subtle.find_screen(0)
 *  => nil
 */

VALUE
subClientScreenWriter(VALUE self,
  VALUE value)
{
  VALUE screen = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Check instance type */
  if(rb_obj_is_instance_of(value, rb_const_get(mod, rb_intern("Screen"))))
    screen = value;
  else subScreenFind(Qnil, value);

  /* Set screen */
  if(RTEST(screen))
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = FIX2LONG(rb_iv_get(self,   "@id"));
      data.l[1] = FIX2LONG(rb_iv_get(screen, "@id"));

      subSharedMessage(display, DefaultRootWindow(display), "SUBTLE_WINDOW_SCREEN", data, True);

      rb_iv_set(self, "@screen", INT2FIX(screen));
    }
  else rb_raise(rb_eArgError, "Failed setting value type `%s'", rb_obj_classname(value));

  return Qnil;
} /* }}} */

/* subClientGeometryReader {{{ */
/*
 * call-seq: geometry -> Subtlext::Geometry
 *
 * Get Client Geometry
 *
 *  client.geometry
 *  => #<Subtlext::Geometry:xxx>
 */

VALUE
subClientGeometryReader(VALUE self)
{
  Window win = None;
  VALUE geom = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  geom = rb_iv_get(self, "@geometry");
  win = NUM2LONG(rb_iv_get(self, "@win"));

  /* Load on demand */
  if(NIL_P((geom = rb_iv_get(self, "@geometry"))) &&
      (win = NUM2LONG(rb_iv_get(self, "@win"))))
    {
      XRectangle geometry = { 0 };

      subSharedPropertyGeometry(display, win, &geometry);

      geom = subGeometryInstantiate(geometry.x, geometry.y,
        geometry.width, geometry.height);

      rb_iv_set(self, "@geometry", geom);
    }

  return geom;
} /* }}} */

/* subClientGeometryWriter {{{ */
/*
 * call-seq: geometry=(x, y, width, height) -> Subtlext::Geometry
 *           geometry=(array)               -> Subtlext::Geometry
 *           geometry=(geometry)            -> Subtlext::Geometry
 *
 * Set Client geometry
 *
 *  client.geometry = 0, 0, 100, 100
 *  => #<Subtlext::Geometry:xxx>
 *
 *  client.geometry = [ 0, 0, 100, 100 ]
 *  => #<Subtlext::Geometry:xxx>
 *
 *  client.geometry = Subtlext::Geometry(0, 0, 100, 100)
 *  => #<Subtlext::Geometry:xxx>
 */

VALUE
subClientGeometryWriter(int argc,
  VALUE *argv,
  VALUE self)
{
  VALUE klass = Qnil, geometry = Qnil;

  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Delegate arguments */
  klass    = rb_const_get(mod, rb_intern("Geometry"));
  geometry = rb_funcall2(klass, rb_intern("new"), argc, argv);

  /* Update geometry */
  if(RTEST(geometry))
    {
      Window win = None;
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      win       = NUM2LONG(rb_iv_get(self, "@win"));
      data.l[1] = FIX2INT(rb_iv_get(geometry,  "@x"));
      data.l[2] = FIX2INT(rb_iv_get(geometry,  "@y"));
      data.l[3] = FIX2INT(rb_iv_get(geometry,  "@width"));
      data.l[4] = FIX2INT(rb_iv_get(geometry,  "@height"));

      subSharedMessage(display, win, "_NET_MOVERESIZE_WINDOW", data, True);

      rb_iv_set(self, "@geometry", geometry);
    }

  return geometry;
} /* }}} */

/* subClientResizeWriter {{{ */
/*
 * call-seq: resize=(bool) -> nil
 *
 * Set Client resize
 *
 *  client.resize = :false
 *  => nil
 *
 */

VALUE
subClientResizeWriter(VALUE self,
  VALUE value)
{
  rb_check_frozen(self);
  subSubtlextConnect(); ///< Implicit open connection

  /* Check instance type */
  if(Qtrue == value || Qfalse == value)
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = FIX2LONG(rb_iv_get(self, "@id"));
      data.l[1] = (Qtrue == value);

      subSharedMessage(display, DefaultRootWindow(display), "SUBTLE_WINDOW_RESIZE", data, True);
    }
  else rb_raise(rb_eArgError, "Unknown value type");

  return Qnil;
} /* }}} */

/* subClientShow {{{ */
/*
 * call-seq: show() -> nil
 *
 * Show a Client
 *
 *  client.show
 *  => nil
 */

VALUE
subClientShow(VALUE self)
{
  VALUE win = rb_iv_get(self, "@win");

  rb_check_frozen(self);

  if(RTEST(win))
    {
      rb_iv_set(self, "@hidden", Qfalse);

      XMapWindow(display, NUM2LONG(win));
      XSync(display, False); ///< Sync all changes
    }

  return Qnil;
} /* }}} */

/* subClientHide {{{ */
/*
 * call-seq: hide() -> nil
 *
 * Hide a Client
 *
 *  client.hide
 *  => nil
 */

VALUE
subClientHide(VALUE self)
{
  VALUE win = rb_iv_get(self, "@win");

  rb_check_frozen(self);

  if(RTEST(win))
    {
      rb_iv_set(self, "@hidden", Qtrue);

      XUnmapWindow(display, NUM2LONG(win));
      XSync(display, False); ///< Sync all changes
    }

  return Qnil;
} /* }}} */

/* subClientHiddenAsk {{{ */
/*
 * call-seq: hidden -> true or false
 *
 * Whether Client is hidden
 *
 *  client.hidden?
 *  => true
 */

VALUE
subClientHiddenAsk(VALUE self)
{
  return rb_iv_get(self, "@hidden");
} /* }}} */

/* subClientToString {{{ */
/*
 * call-seq: to_str -> String
 *
 * Convert Client object to String
 *
 *  puts client
 *  => "subtle"
 */

VALUE
subClientToString(VALUE self)
{
  VALUE name = rb_iv_get(self, "@name");

  return RTEST(name) ? name : Qnil;
} /* }}} */

/* subClientKill {{{ */
/*
 * call-seq: kill -> nil
 *
 * Send a close signal to Client
 *
 *  client.kill
 *  => nil
 */

VALUE
subClientKill(VALUE self)
{
  VALUE win = rb_iv_get(self, "@win");

  subSubtlextConnect(); ///< Implicit open connection

  if(RTEST(win))
    {
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      data.l[0] = CurrentTime;
      data.l[1] = 2; ///< Claim to be a pager

      subSharedMessage(display, NUM2LONG(win), "_NET_CLOSE_WINDOW", data, True);
    }
  else rb_raise(rb_eStandardError, "Failed killing client");

  rb_obj_freeze(self);

  return Qnil;
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
