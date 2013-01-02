
 /**
  * @package subtlext
  *
  * @file Main functions
  * @copyright (c) 2005-2013 Christoph Kappel <unexist@subforge.org>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPLv2.
  * See the file COPYING for details.
  **/

#include <unistd.h>
#include <locale.h>
#include <ctype.h>
#include "subtlext.h"

#ifdef HAVE_X11_EXTENSIONS_XTEST_H
#include <X11/extensions/XTest.h>
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */

Display *display = NULL;
VALUE mod = Qnil;

/* SubtlextStringify {{{ */
static void
SubtlextStringify(char *string)
{
  assert(string);

  /* Lowercase and replace strange characters */
  while('\0' != *string)
    {
      *string = toupper(*string);

      if(!isalnum(*string)) *string = '_';

      string++;
    }
} /* }}} */

/* SubtlextSweep {{{ */
static void
SubtlextSweep(void)
{
  if(display)
    {
      XCloseDisplay(display);

      display = NULL;
    }
} /* }}} */

/* SubtlextPidReader {{{ */
/*
 * call-seq: pid => Fixnum
 *
 * Get window pid
 *
 *  object.pid
 *  => 123
 **/

static VALUE
SubtlextPidReader(VALUE self)
{
  Window win = None;
  VALUE pid = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  /* Load on demand */
  if(NIL_P((pid = rb_iv_get(self, "@pid"))))
    {
      int *id = NULL;

      /* Get pid */
      if((id = (int *)subSharedPropertyGet(display, win, XA_CARDINAL,
          XInternAtom(display, "_NET_WM_PID", False), NULL)))
        {
          pid = INT2FIX(*id);

          rb_iv_set(self, "@pid", pid);

          free(id);
        }
    }

  return pid;
} /* }}} */

/* SubtlextFlags {{{ */
static int
SubtlextFlags(VALUE key,
  VALUE value,
  VALUE data)
{
  VALUE *rargs = (VALUE *)data;

  /* Set flags */
  if(CHAR2SYM("name")          == key) rargs[0] = SUB_MATCH_NAME;
  else if(CHAR2SYM("instance") == key) rargs[0] = SUB_MATCH_INSTANCE;
  else if(CHAR2SYM("class")    == key) rargs[0] = SUB_MATCH_CLASS;
  else if(CHAR2SYM("gravity")  == key) rargs[0] = SUB_MATCH_GRAVITY;
  else if(CHAR2SYM("role")     == key) rargs[0] = SUB_MATCH_ROLE;
  else if(CHAR2SYM("pid")      == key) rargs[0] = SUB_MATCH_PID;

  /* Set value */
  if(0 != rargs[0] && RTEST(value))
    {
      rargs[1] = value;

      return ST_STOP;
    }

  return ST_CONTINUE;
} /* }}} */

/* SubtlextStyle {{{ */
/*
 * call-seq: style=(string) -> nil
 *           style=(symbol) -> nil
 *           style=(nil)    -> nil
 *
 * Set style state of this Object, use <b>nil</b> to reset state.
 *
 *  object.style = :blue
 *  => nil
 */

static VALUE
SubtlextStyle(VALUE self,
  VALUE value)
{
  char *prop = NULL;
  VALUE id = Qnil, str = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  /* Check object type */
  if(rb_obj_is_instance_of(self, rb_const_get(mod, rb_intern("View"))))
    prop = "SUBTLE_VIEW_STYLE";
  else prop = "SUBTLE_SUBLET_STYLE";

  /* Check value type */
  switch(rb_type(value))
    {
      case T_SYMBOL: str = rb_sym_to_s(value);
      case T_STRING:
        snprintf(data.b, sizeof(data.b), "%d#%s",
          (int)FIX2INT(id), RSTRING_PTR(str));

        subSharedMessage(display, ROOT, prop, data, 32, True);
        break;
      case T_NIL:
        snprintf(data.b, sizeof(data.b), "%d#", (int)FIX2INT(id));
        subSharedMessage(display, ROOT, prop, data, 32, True);
        break;
      default: rb_raise(rb_eArgError, "Unexpected value-type `%s'",
        rb_obj_classname(value));
    }

  return Qnil;
} /* }}} */

/* SubtlextHash {{{ */
/*
 * call-seq: hash -> Hash
 *
 * Convert this object to hash.
 *
 *  puts object.hash
 *  => 1746246187916025425
 */

static VALUE
SubtlextHash(VALUE self)
{
  VALUE str = Qnil, id = rb_intern("to_str");

  /* Convert to string */
  if(rb_respond_to(self, id))
    str = rb_funcall(self, id, 0, Qnil);

  return T_STRING == rb_type(str) ? INT2FIX(rb_str_hash(str)) : Qnil;
} /* }}} */

 /* SubtlextXError {{{ */
static int
SubtlextXError(Display *disp,
  XErrorEvent *ev)
{
#ifdef DEBUG
  if(42 != ev->request_code) /* X_SetInputFocus */
    {
      char error[255] = { 0 };

      XGetErrorText(disp, ev->error_code, error, sizeof(error));
      printf("<XERROR> %s: win=%#lx, request=%d\n",
        error, ev->resourceid, ev->request_code);
    }
#endif /* DEBUG */

  return 0;
} /* }}} */

/* Tags */

/* SubtlextTagFind {{{ */
static int
SubtlextTagFind(VALUE value)
{
  int tags = 0;

  /* Check object type */
  switch(rb_type(value))
    {
      case T_SYMBOL:
      case T_STRING:
          {
            int id, flags = 0;
            char *string = NULL;

            /* Handle symbols and strings */
            if(T_SYMBOL == rb_type(value))
              {
                flags  |= SUB_MATCH_EXACT;
                string  = (char *)SYM2CHAR(value);
              }
            else string = RSTRING_PTR(value);

            /* Find tag and get id */
            if(-1 != (id = subextSubtlextFindString("SUBTLE_TAG_LIST",
                string, NULL, flags)))
              tags |= (1L << (id + 1));
          }
        break;
      case T_OBJECT:
        /* Check instance type and fetch id */
        if(rb_obj_is_instance_of(value, rb_const_get(mod, rb_intern("Tag"))))
          {
            VALUE id = Qnil;

            if(FIXNUM_P((id = rb_iv_get(value, "@id"))))
              tags |= (1L << (FIX2INT(id) + 1));
          }
        break;
      case T_ARRAY:
          {
            int i;
            VALUE entry = Qnil;

            /* Collect tags and raise if a tag wasn't found. Empty
             * arrays reset tags and never enter this loop */
            for(i = 0; Qnil != (entry = rb_ary_entry(value, i)); ++i)
              tags |= SubtlextTagFind(entry);
          }
        break;
      default:
        rb_raise(rb_eArgError, "Unexpected value-type `%s'",
          rb_obj_classname(value));
    }

  return tags;
} /* }}} */

/* SubtlextTag {{{ */
static VALUE
SubtlextTag(VALUE self,
  VALUE value,
  int action)
{
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);

  /* Convert tags to bitmask */
  data.l[1] |= SubtlextTagFind(value);

  /* Get and update tag mask */
  if(0 != action)
    {
      int tags = FIX2INT(rb_iv_get(self, "@tags"));

      /* Update masks */
     if(1 == action)       data.l[1] = tags |  data.l[1];
     else if(-1 == action) data.l[1] = tags & ~data.l[1];
    }

  /* Send message based on object type */
  if(rb_obj_is_instance_of(self, rb_const_get(mod, rb_intern("Client"))))
    {
      VALUE win = Qnil;

      GET_ATTR(self, "@win", win);
      data.l[0] = NUM2LONG(win);

      subSharedMessage(display, ROOT, "SUBTLE_CLIENT_TAGS", data, 32, True);
    }
  else
    {
      VALUE id = Qnil;

      GET_ATTR(self, "@id", id);
      data.l[0] = FIX2LONG(id);

      subSharedMessage(display, ROOT, "SUBTLE_VIEW_TAGS", data, 32, True);
    }

  return Qnil;
} /* }}} */

/* SubtlextTagWriter {{{ */
/*
 * call-seq: tags=(value) -> nil
 *
 * Set or remove all tags at once
 *
 *  # Set new tags
 *  object.tags=([ #<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx> ])
 *  => nil
 *
 *  # Remove all tags
 *  object.tags=([])
 *  => nil
 */

static VALUE
SubtlextTagWriter(VALUE self,
  VALUE value)
{
  return SubtlextTag(self, value, 0);
} /* }}} */

/* SubtlextTagReader {{{ */
/*
 * call-seq: tags -> Array
 *
 * Get list of tags for window
 *
 *  object.tags
 *  => [#<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx>]
 */

static VALUE
SubtlextTagReader(VALUE self)
{
  char **tags = NULL;
  int i, ntags = 0, value_tags = 0;
  VALUE method = Qnil, klass = Qnil, t = Qnil;
  VALUE array = rb_ary_new();

  /* Check ruby object */
  rb_check_frozen(self);

  /* Fetch data */
  method     = rb_intern("new");
  klass      = rb_const_get(mod, rb_intern("Tag"));
  value_tags = FIX2INT(rb_iv_get(self, "@tags"));

  /* Check results */
  if((tags = subSharedPropertyGetStrings(display, ROOT,
      XInternAtom(display, "SUBTLE_TAG_LIST", False), &ntags)))
    {
      for(i = 0; i < ntags; i++)
        {
          if(value_tags & (1L << (i + 1)))
            {
              /* Create new tag */
              t = rb_funcall(klass, method, 1, rb_str_new2(tags[i]));
              rb_iv_set(t, "@id", INT2FIX(i));
              rb_ary_push(array, t);
            }
        }

      XFreeStringList(tags);
    }

  return array;
} /* }}} */

/* SubtlextTagAdd {{{ */
/*
 * call-seq: tag(value) -> nil
 *           +(value)   -> nil
 *
 * Add an existing tag to window
 *
 *  object.tag("subtle")
 *  => nil
 *
 *  object.tag([ #<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx> ])
 *  => nil
 *
 *  object + "subtle"
 *  => nil
 */

static VALUE
SubtlextTagAdd(VALUE self,
  VALUE value)
{
  return SubtlextTag(self, value, 1);
} /* }}} */

/* SubtlextTagDel {{{ */
/*
 * call-seq: untag(value) -> nil
 *           -(value)     -> nil
 *
 * Remove an existing tag from window
 *
 *  object.untag("subtle")
 *  => nil
 *
 *  object.untag([ #<Subtlext::Tag:xxx>, #<Subtlext::Tag:xxx> ])
 *  => nil
 *
 *  object - "subtle"
 *  => nil
 */

static VALUE
SubtlextTagDel(VALUE self,
  VALUE value)
{
  return SubtlextTag(self, value, -1);
} /* }}} */

/* SubtlextTagAsk {{{ */
static VALUE
SubtlextTagAsk(VALUE self,
  VALUE value)
{
  VALUE sym = Qnil, tag = Qnil, ret = Qfalse;

  /* Check ruby object */
  rb_check_frozen(self);

  /* Check value type */
  switch(rb_type(value))
    {
      case T_STRING: sym = CHAR2SYM(RSTRING_PTR(value)); break;
      case T_SYMBOL:
      case T_OBJECT: sym = value;                        break;
      default: rb_raise(rb_eArgError, "Unexpected value-type `%s'",
        rb_obj_classname(value));
    }

  /* Find tag */
  if(RTEST(tag = subextTagSingFirst(Qnil, sym)))
    {
      VALUE id = Qnil, tags = Qnil;

      /* Get properties */
      id   = rb_iv_get(tag,  "@id");
      tags = rb_iv_get(self, "@tags");

      if(FIX2INT(tags) & (1L << (FIX2INT(id) + 1))) ret = Qtrue;
    }

  return ret;
} /* }}} */

/* SubtlextTagReload {{{ */
static VALUE
SubtlextTagReload(VALUE self)
{
  VALUE win = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  /* Send message */
  data.l[0] = NUM2LONG(win);

  subSharedMessage(display, ROOT, "SUBTLE_CLIENT_RETAG", data, 32, True);

  return Qnil;
} /* }}} */

/* Send button/key */

/* SubtlextSendButton {{{ */
/*
 * call-seq: send_button(button, x, y) -> Object
 *
 * Emulate a click on a window with optional button
 * and x/y position
 *
 *  object.send_button
 *  => nil
 *
 *  object.send_button(2)
 *  => Object
 */

static VALUE
SubtlextSendButton(int argc,
  VALUE *argv,
  VALUE self)
{
  Window subwin = None;
  XEvent event = { 0 };
  VALUE button = Qnil, x = Qnil, y = Qnil, win = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  rb_scan_args(argc, argv, "03", &button, &x, &y);

  /* Assemble button event */
  event.type                  = EnterNotify;
  event.xcrossing.window      = NUM2LONG(win);
  event.xcrossing.root        = ROOT;
  event.xcrossing.subwindow   = NUM2LONG(win);
  event.xcrossing.same_screen = True;
  event.xcrossing.x           = FIXNUM_P(x) ? FIX2INT(x) : 5;
  event.xcrossing.y           = FIXNUM_P(y) ? FIX2INT(y) : 5;

  /* Translate window x/y to root x/y */
  XTranslateCoordinates(display, event.xcrossing.window,
    event.xcrossing.root, event.xcrossing.x, event.xcrossing.y,
    &event.xcrossing.x_root, &event.xcrossing.y_root, &subwin);

  //XSetInputFocus(display, event.xany.window, RevertToPointerRoot, CurrentTime);
  XSendEvent(display, NUM2LONG(win), True, EnterWindowMask, &event);

  /* Send button press event */
  event.type           = ButtonPress;
  event.xbutton.button = FIXNUM_P(button) ? FIX2INT(button) : 1;

  XSendEvent(display, NUM2LONG(win), True, ButtonPressMask, &event);
  XFlush(display);

  usleep(12000);

  /* Send button release event */
  event.type = ButtonRelease;

  XSendEvent(display, NUM2LONG(win), True, ButtonReleaseMask, &event);
  XFlush(display);

  return self;
} /* }}} */

#ifdef HAVE_X11_EXTENSIONS_XTEST_H
/* SubtlextSendModifier {{{ */
static void
SubtlextSendModifier(unsigned long state,
  int press)
{
  /* Send modifier press/release events */
  if(state & ShiftMask) ///< Shift key
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L),
        press, CurrentTime);
    }
  else if(state & ControlMask) ///< Ctrl key
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Control_L),
        press, CurrentTime);
    }
  else if(state & Mod1Mask) ///< Alt key
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Alt_L),
        press, CurrentTime);
    }
  else if(state & Mod3Mask) /// Mod key<
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Meta_L),
        press, CurrentTime);
    }
  else if(state & Mod4Mask) ///< Super key
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Super_L),
        press, CurrentTime);
    }
  else if(state & Mod5Mask) ///< Alt Gr key
    {
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_ISO_Level3_Shift),
        press, CurrentTime);
    }
} /* }}} */
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */

/* SubtlextSendKey {{{ */
/*
 * call-seq: send_key(key, x, y) -> Object
 *
 * Emulate a keypress on a window
 *
 *  object.send_key("d")
 *  => Object
 */

static VALUE
SubtlextSendKey(int argc,
  VALUE *argv,
  VALUE self)
{
  VALUE keys = Qnil, x = Qnil, y = Qnil, win = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  rb_scan_args(argc, argv, "12", &keys, &x, &y);

  /* Check object type */
  if(T_STRING == rb_type(keys))
    {
      int mouse = False;
      unsigned int code = 0, state = 0;
      char *tokens = NULL, *tok = NULL, *save = NULL;
      Window subwin = None;
      KeySym sym = None;
      XEvent event = { 0 };

      /* Assemble enter event */
      event.type                  = EnterNotify;
      event.xcrossing.window      = NUM2LONG(win);
      event.xcrossing.root        = ROOT;
      event.xcrossing.subwindow   = NUM2LONG(win);
      event.xcrossing.same_screen = True;
      event.xcrossing.x           = FIXNUM_P(x) ? FIX2INT(x) : 5;
      event.xcrossing.y           = FIXNUM_P(y) ? FIX2INT(y) : 5;

      /* Translate window x/y to root x/y */
      XTranslateCoordinates(display, event.xcrossing.window,
        event.xcrossing.root, event.xcrossing.x, event.xcrossing.y,
        &event.xcrossing.x_root, &event.xcrossing.y_root, &subwin);

      XSendEvent(display, NUM2LONG(win), True, EnterWindowMask, &event);

      /* Parse keys */
      tokens = strdup(RSTRING_PTR(keys));
      tok    = strtok_r(tokens, " ", &save);

      while(tok)
        {
          /* Parse key chain */
          if(NoSymbol == (sym = subSharedParseKey(display,
              tok, &code, &state, &mouse)))
            {
              rb_raise(rb_eStandardError, "Unknown key");

              return Qnil;
            }

          /* Check mouse */
          if(True == mouse)
            {
              rb_raise(rb_eNotImpError, "Use #send_button instead");

              return Qnil;
            }

#ifdef HAVE_X11_EXTENSIONS_XTEST_H
          XTestGrabControl(display, True);

          /* Send key press/release events */
          SubtlextSendModifier(state, True);
          XTestFakeKeyEvent(display, code, True, CurrentTime);
          XTestFakeKeyEvent(display, code, False, CurrentTime);
          SubtlextSendModifier(state, False);

          XTestGrabControl(display, False);
#else /* HAVE_X11_EXTENSIONS_XTEST_H */
          /* Send key press event */
          event.type         = KeyPress;
          event.xkey.state   = state;
          event.xkey.keycode = code;

          XSendEvent(display, NUM2LONG(win), True, KeyPressMask, &event);
          XFlush(display);

          usleep(12000);

          /* Send key release event */
          event.type = KeyRelease;

          XSendEvent(display, NUM2LONG(win), True, KeyReleaseMask, &event);
#endif /* HAVE_X11_EXTENSIONS_XTEST_H */

          tok = strtok_r(NULL, " ", &save);
        }

      XFlush(display);

      free(tokens);
    }
  else rb_raise(rb_eArgError, "Unexpected value-type `%s'",
    rb_obj_classname(keys));

  return self;
} /* }}} */

/* Focus */

/* SubtlextFocus {{{ */
/*
 * call-seq: focus -> nil
 *
 * Set focus to window
 *
 *  object.focus
 *  => nil
 */

static VALUE
SubtlextFocus(VALUE self)
{
  VALUE win = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  /* Send message */
  data.l[0] = NUM2LONG(win);

  subSharedMessage(display, ROOT, "_NET_ACTIVE_WINDOW", data, 32, True);

  return self;
} /* }}} */

/* SubtlextAskFocus {{{ */
/*
 * call-seq: has_focus? -> true or false
 *
 * Check if window has focus
 *
 *  object.focus?
 *  => true
 *
 *  object.focus?
 *  => false
 */

static VALUE
SubtlextAskFocus(VALUE self)
{
  VALUE ret = Qfalse, win = Qnil;
  unsigned long *focus = NULL;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@win", win);

  /* Fetch data */
  if((focus = (unsigned long *)subSharedPropertyGet(display, ROOT,
      XA_WINDOW, XInternAtom(display, "_NET_ACTIVE_WINDOW", False), NULL)))
    {
      if(*focus == NUM2LONG(win)) ret = Qtrue;

      free(focus);
    }

  return ret;
} /* }}} */

/* Properties */

/* SubtlextPropReader {{{ */
/*
 * call-seq: [value] -> String or Nil
 *
 * Get arbitrary persistent property string or symbol value
 *
 *  object["wm"]
 *  => "subtle"
 *
 *  object[:wm]
 *  => "subtle"
 */

static VALUE
SubtlextPropReader(VALUE self,
  VALUE key)
{
  char *prop = NULL;
  VALUE ret = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);

  /* Check object type */
  switch(rb_type(key))
    {
      case T_STRING: prop = RSTRING_PTR(key);      break;
      case T_SYMBOL: prop = (char *)SYM2CHAR(key); break;
      default:
        rb_raise(rb_eArgError, "Unexpected key value type `%s'",
          rb_obj_classname(key));

        return Qnil;
    }

  /* Check results */
  if(prop)
    {
      char propname[255] = { 0 }, *name = NULL, *result = NULL;
      Window win = ROOT;
      VALUE val = Qnil;

      /* Sanitize property name */
      name = strdup(prop);
      SubtlextStringify(name);

      /* Check object type */
      if(rb_obj_is_instance_of(self, rb_const_get(mod, rb_intern("View"))))
        {
          GET_ATTR(self, "@name", val);
          snprintf(propname, sizeof(propname), "SUBTLE_PROPERTY_%s_%s",
            RSTRING_PTR(val), name);
        }
      else ///< Client
        {
          GET_ATTR(self, "@win", val);
          win = NUM2LONG(val);
          snprintf(propname, sizeof(propname), "SUBTLE_PROPERTY_%s", name);
        }

      /* Get actual property */
      if((result = subSharedPropertyGet(display, win, XInternAtom(display,
          "UTF8_STRING", False), XInternAtom(display, propname, False), NULL)))
        {
          ret = rb_str_new2(result);

          free(result);
        }

      free(name);
    }

  return ret;
} /* }}} */

/* SubtlextPropWriter {{{ */
/*
 * call-seq: [key]= value -> Nil
 *
 * Set arbitrary persistent property string or symbol value
 *
 * Symbols are implictly converted to string, to remove a property just
 * set it to +nil+.
 *
 *  object["wm"] = "subtle"
 *  => nil
 *
 *  object[:wm] = "subtle"
 *  => nil
 *
 *  object[:wm] = nil
 *  => nil
 */

static VALUE
SubtlextPropWriter(VALUE self,
  VALUE key,
  VALUE value)
{
  VALUE val = Qnil, str = value;
  char *prop = NULL, *name = NULL, propname[255] = { 0 };
  Window win = ROOT;

  /* Check ruby object */
  rb_check_frozen(self);

  /* Check object type */
  switch(rb_type(key))
    {
      case T_STRING: prop = RSTRING_PTR(key);      break;
      case T_SYMBOL: prop = (char *)SYM2CHAR(key); break;
      default:
        rb_raise(rb_eArgError, "Unexpected key value-type `%s'",
          rb_obj_classname(key));

        return Qnil;
    }

  /* Sanitize property name */
  name = strdup(prop);
  SubtlextStringify(name);

  /* Assemble property name */
  if(rb_obj_is_instance_of(self, rb_const_get(mod, rb_intern("View"))))
    {
      GET_ATTR(self, "@name", val);
      snprintf(propname, sizeof(propname), "SUBTLE_PROPERTY_%s_%s",
        RSTRING_PTR(val), name);
    }
  else ///< Client
    {
      GET_ATTR(self, "@win", val);
      win = NUM2LONG(val);
      snprintf(propname, sizeof(propname), "SUBTLE_PROPERTY_%s", name);
    }

  /* Check value type */
  switch(rb_type(value))
    {
      case T_SYMBOL: str = rb_sym_to_s(value);
      case T_STRING:
        XChangeProperty(display, win, XInternAtom(display, propname, False),
          XInternAtom(display, "UTF8_STRING", False), 8, PropModeReplace,
          (unsigned char *)RSTRING_PTR(str), RSTRING_LEN(str));
        break;
      case T_NIL:
        XDeleteProperty(display, win, XInternAtom(display, propname, False));
        break;
      default:
        rb_raise(rb_eArgError, "Unexpected value value-type `%s'",
          rb_obj_classname(value));
    }

  XSync(display, False); ///< Sync all changes

  if(name) free(name);

  return Qnil;
} /* }}} */

/* Comparisons */

/* SubtlextEqual {{{ */
static VALUE
SubtlextEqual(VALUE self,
  VALUE other,
  const char *attr,
  int check_type)
{
  int ret = False;
  VALUE val1 = Qnil, val2 = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self,  attr, val1);
  GET_ATTR(other, attr, val2);

  /* Check ruby object types */
  if(check_type)
    {
      ret = (rb_obj_class(self) == rb_obj_class(other) && val1 == val2);
    }
  else ret = (val1 == val2);

  return ret ? Qtrue : Qfalse;
} /* }}} */

/* SubtlextSpaceship {{{ */
static VALUE
SubtlextSpaceship(VALUE self,
  VALUE other,
  const char *attr)
{
  VALUE val1 = Qnil, val2 = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self,  attr, val1);
  GET_ATTR(other, attr, val2);

  return INT2FIX(val1 < val2 ? -1 : (val1 == val2 ? 0 : 1));
} /* }}} */

/* SubtlextEqualId {{{ */
/*
 * call-seq: ==(other) -> True or False
 *
 * Whether both objects have the same values (based on id)
 *
 *  object1 == object2
 *  => true
 */

static VALUE
SubtlextEqualId(VALUE self,
  VALUE other)
{
  return SubtlextEqual(self, other, "@id", False);
} /* }}} */

/* SubtlextEqualWindow {{{ */
/*
 * call-seq: ==(other) -> True or False
 *
 * Whether both objects have the same values (based on win)
 *
 *  object1 == object2
 *  => true
 */

static VALUE
SubtlextEqualWindow(VALUE self,
  VALUE other)
{
  return SubtlextEqual(self, other, "@win", False);
} /* }}} */

/* SubtlextEqualTypedId {{{ */
/*
 * call-seq: eql?(other) -> True or False
 *
 * Whether both objects have the same values and types (based on id)
 *
 *  object1.eql? object2
 *  => true
 */

static VALUE
SubtlextEqualTypedId(VALUE self,
  VALUE other)
{
  return SubtlextEqual(self, other, "@id", True);
} /* }}} */

/* SubtlextEqualTypedWindow {{{ */
/*
 * call-seq: eql?(other) -> True or False
 *
 * Whether both objects have the same value and types (based on win)
 *
 *  object1.eql? object2
 *  => true
 */

static VALUE
SubtlextEqualTypedWindow(VALUE self,
  VALUE other)
{
  return SubtlextEqual(self, other, "@win", True);
} /* }}} */

/* SubtlextEqualSpaceWindow {{{ */
/*
 * call-seq: <=>(other) -> -1, 0 or 1
 *
 * Whether both objects have the same value. Returns -1, 0 or 1 when self is
 * less than, equal to or grater than other. (based on win)
 *
 *  object1 <=> object2
 *  => 0
 */

static VALUE
SubtlextEqualSpaceWindow(VALUE self,
  VALUE other)
{
  return SubtlextSpaceship(self, other, "@win");
} /* }}} */

/* SubtlextEqualSpacePixel {{{ */
/*
 * call-seq: <=>(other) -> -1, 0 or 1
 *
 * Whether both objects have the same value. Returns -1, 0 or 1 when self is
 * less than, equal to or grater than other. (based on pixel)
 *
 *  object1 <=> object2
 *  => 0
 */

static VALUE
SubtlextEqualSpacePixel(VALUE self,
  VALUE other)
{
  return SubtlextSpaceship(self, other, "@pixel");
} /* }}} */

/* SubtlextEqualSpacePixmap {{{ */
/*
 * call-seq: <=>(other) -> -1, 0 or 1
 *
 * Whether both objects have the same value. Returns -1, 0 or 1 when self is
 * less than, equal to or grater than other. (based on pixmap)
 *
 *  object1 <=> object2
 *  => 0
 */

static VALUE
SubtlextEqualSpacePixmap(VALUE self,
  VALUE other)
{
  return SubtlextSpaceship(self, other, "@pixmap");
} /* }}} */

/* SubtlextEqualSpaceId {{{ */
/*
 * call-seq: <=>(other) -> -1, 0 or 1
 *
 * Whether both objects have the same value. Returns -1, 0 or 1 when self is
 * less than, equal to or grater than other. (based on id)
 *
 *  object1 <=> object2
 *  => 0
 */

static VALUE
SubtlextEqualSpaceId(VALUE self,
  VALUE other)
{
  return SubtlextSpaceship(self, other, "@id");
} /* }}} */

/* SubtlextWindowMatch {{{ */
static int
SubtlextWindowMatch(Window win,
  regex_t *preg,
  const char *source,
  char **name,
  int flags)
{
  int ret = False;
  char *wminstance = NULL, *wmclass = NULL;

  /* Fetch when needed */
  if(name || flags & (SUB_MATCH_INSTANCE|SUB_MATCH_CLASS))
    subSharedPropertyClass(display, win, &wminstance, &wmclass);

  /* Check window WM_NAME */
  if(!ret && flags & SUB_MATCH_NAME)
    {
      char *wmname = NULL;

      subSharedPropertyName(display, win, &wmname, "subtle");

      if(wmname)
        {
          ret = (flags & SUB_MATCH_EXACT ? 0 == strcmp(source, wmname) :
            subSharedRegexMatch(preg, wmname));

          free(wmname);
        }
    }

  /* Check window WM_CLASS */
  if(!ret && flags & (SUB_MATCH_INSTANCE|SUB_MATCH_CLASS))
    {
      /* Check instance */
      if(wminstance && flags & SUB_MATCH_INSTANCE)
        {
          ret = (flags & SUB_MATCH_EXACT ?
            0 == strcmp(source, wminstance) :
            subSharedRegexMatch(preg, wminstance));
        }

      /* Check class */
      if(!ret && wmclass && flags & SUB_MATCH_CLASS)
        {
          ret = (flags & SUB_MATCH_EXACT ?
            0 == strcmp(source, wmclass) :
            subSharedRegexMatch(preg, wmclass));

          free(wmclass);
        }
    }

  /* Check window role */
  if(!ret && flags & SUB_MATCH_ROLE)
    {
      char *role = NULL;

      if((role = subSharedPropertyGet(display, win, XA_STRING,
          XInternAtom(display, "WM_WINDOW_ROLE", False), NULL)))
        {
          ret = (flags & SUB_MATCH_EXACT ? 0 == strcmp(source, role) :
            subSharedRegexMatch(preg, role));

          free(role);
        }
     }

  /* Check window gravity */
  if(!ret && flags & SUB_MATCH_GRAVITY)
    {
      int *gravity = NULL, ngravities = 0;
      char **gravities = NULL;

      /* Fetch gravities */
      gravities = subSharedPropertyGetStrings(display,
        ROOT, XInternAtom(display, "SUBTLE_GRAVITY_LIST",
        False), &ngravities);
      gravity = (int *)subSharedPropertyGet(display, win,
        XA_CARDINAL, XInternAtom(display, "SUBTLE_CLIENT_GRAVITY",
        False), NULL);

      /* Finally compare gravities */
      if(gravities && gravity && 0 <= *gravity && *gravity < ngravities)
        {
          ret = (flags & SUB_MATCH_EXACT ?
            0 == strcmp(source, gravities[*gravity]) :
            subSharedRegexMatch(preg, gravities[*gravity]));
        }

      if(gravities) XFreeStringList(gravities);
      if(gravity)   free(gravity);
    }

  /* Check window pid */
  if(!ret && flags & SUB_MATCH_PID)
    {
      int *pid = NULL;

      /* Fetch pid from window */
      if((pid = (int *)subSharedPropertyGet(display, win, XA_CARDINAL,
          XInternAtom(display, "_NET_WM_PID", False), NULL)))
        {
          char pidbuf[10] = { 0 };

          /* Convert pid to string */
          snprintf(pidbuf, sizeof(pidbuf), "%d", (int)*pid);

          ret = (flags & SUB_MATCH_EXACT ? 0 == strcmp(source, pidbuf) :
            subSharedRegexMatch(preg, pidbuf));

          free(pid);
        }
    }

  /* Copy instance name */
  if(ret && name)
    {
      *name = (char *)subSharedMemoryAlloc(
        strlen(wminstance) + 1, sizeof(char));
      strncpy(*name, wminstance, strlen(wminstance));
    }

  if(wminstance) free(wminstance);

  return ret;
} /* }}} */

/* Exported */

 /** subextSubtlextConnect {{{
  * @brief Open connection to X display
  * @param[in]  display_string  Display name
  **/

void
subextSubtlextConnect(char *display_string)
{
  /* Open display */
  if(!display)
    {
      if(!(display = XOpenDisplay(display_string)))
        rb_raise(rb_eStandardError, "Invalid display `%s'", display_string);

      XSetErrorHandler(SubtlextXError);

      if(!setlocale(LC_CTYPE, "")) XSupportsLocale();

      /* Register sweeper */
      atexit(SubtlextSweep);
    }
} /* }}} */

  /** subextSubtlextBacktrace {{{
   * @brief Print ruby backtrace
   **/

void
subextSubtlextBacktrace(void)
{
  VALUE lasterr = Qnil;

  /* Get last error */
  if(!NIL_P(lasterr = rb_gv_get("$!")))
    {
      int i;
      VALUE message = Qnil, klass = Qnil, backtrace = Qnil, entry = Qnil;

      /* Fetching backtrace data */
      message   = rb_obj_as_string(lasterr);
      klass     = rb_class_path(CLASS_OF(lasterr));
      backtrace = rb_funcall(lasterr, rb_intern("backtrace"), 0, NULL);

      /* Print error and backtrace */
      printf("%s: %s\n", RSTRING_PTR(klass), RSTRING_PTR(message));
      for(i = 0; Qnil != (entry = rb_ary_entry(backtrace, i)); ++i)
        printf("\tfrom %s\n", RSTRING_PTR(entry));
    }
} /* }}} */

 /** subextSubtlextConcat {{{
  * @brief Concat string2 to string1
  * @param[inout]  str1  First string
  * @param[in]     str2  Second string
  * @return Concatted string
  **/

VALUE
subextSubtlextConcat(VALUE str1,
  VALUE str2)
{
  VALUE ret = Qnil;

  /* Check values */
  if(RTEST(str1) && RTEST(str2) && T_STRING == rb_type(str1))
    {
      VALUE string = str2;

      /* Convert argument to string */
      if(T_STRING != rb_type(str2) && rb_respond_to(str2, rb_intern("to_s")))
        string = rb_funcall(str2, rb_intern("to_s"), 0, NULL);

      /* Concat strings */
      if(T_STRING == rb_type(string))
        ret = rb_str_cat(str1, RSTRING_PTR(string), RSTRING_LEN(string));
    }
  else rb_raise(rb_eArgError, "Unexpected value type");

  return ret;
} /* }}} */

 /** subextSubtlextParse {{{
  * @brief Parse finder values
  * @param[in]     buf    Passed buffer
  * @param[in]     len    Buffer length
  * @param[inout]  flags  Set flags
  **/

VALUE
subextSubtlextParse(VALUE value,
  char *buf,
  int len,
  int *flags)
{
  VALUE ret = Qnil;

  /* Handle flags {{{ */
  if(flags)
    {
      /* Set defaults */
      *flags = (SUB_MATCH_INSTANCE|SUB_MATCH_CLASS);

      /* Set flags from hash */
      if(T_HASH == rb_type(value))
        {
          VALUE rargs[2] = { 0, Qnil };

          rb_hash_foreach(value, SubtlextFlags, (VALUE)&rargs);

          *flags = (int)rargs[0];
          value  = rargs[1];
        }
    } /* }}} */

  /* Check object type */
  switch(rb_type(value))
    {
      case T_FIXNUM: /* {{{ */
        snprintf(buf, len, "%d", (int)FIX2INT(value));
        break; /* }}} */
      case T_STRING: /* {{{ */
        snprintf(buf, len, "%s", RSTRING_PTR(value));
        break; /* }}} */
      case T_SYMBOL: /* {{{ */
        ret     = value;
        *flags |= SUB_MATCH_EXACT;
        snprintf(buf, len, "%s", SYM2CHAR(value));
        break; /* }}} */
      case T_OBJECT: /* {{{ */
        ret = value;
        break; /* }}} */
      default: /* {{{ */
        rb_raise(rb_eArgError, "Unexpected value-type `%s'",
          rb_obj_classname(value)); /* }}} */
    }

  return ret;
} /* }}} */

 /** subextSubtlextOneOrMany {{{
  * @brief Return one value or many in an array
  * @param[in]  value  Current value
  * @param[in]  prev   Previous value or array
  * @retval Object  Just one value
  * @retval Array   Many values
  **/

VALUE
subextSubtlextOneOrMany(VALUE value,
  VALUE prev)
{
  VALUE ret = Qnil;

  /* Handle different value types */
  switch(rb_type(prev))
    {
      case T_NIL:
        ret = value;
        break;
      case T_ARRAY:
        /* Just append */
        rb_ary_push(prev, value);
        ret = prev;
        break;
      case T_DATA:
      case T_OBJECT:
        {
          /* Create new array and add data */
          ret = rb_ary_new();

          rb_ary_push(ret, prev);
          rb_ary_push(ret, value);
        }
    }

  return ret;
} /* }}} */

 /** subextSubtlextManyToOne {{{
  * @brief Return one value or nil from array or the value
  * @param[in]  value  Given value
  * @retval Object  Just one value
  * @retval nil     Empty array
  **/

VALUE
subextSubtlextManyToOne(VALUE value)
{
  VALUE ret = Qnil;

  /* Handle different value types */
  if(T_ARRAY == rb_type(value))
    {
      /* Just fetch first */
      if(0 < RARRAY_LEN(value))
        ret = rb_ary_entry(value, 0);
    }
  else ret = value;

  return ret;
} /* }}} */

 /** subextSubtlextWindowList {{{
  * @brief Get property window list
  * @param[in]  prop_name  Property name
  * @param[inout]  size  List length
  * @return Property list
  **/

Window *
subextSubtlextWindowList(char *prop_name,
  int *size)
{
  Window *wins = NULL;
  unsigned long len = 0;

  assert(prop_name && size);

  /* Get property list */
  if((wins = (Window *)subSharedPropertyGet(display, ROOT,
      XA_WINDOW, XInternAtom(display, prop_name, False), &len)))
    {
      if(size) *size = len;
    }
  else if(size) *size = 0;

  return wins;
} /* }}} */

 /** subextSubtlextFindString {{{
  * @brief Find string in property list
  * @param[in]     prop_name  Property name
  * @param[in]     source     Regexp source
  * @param[inout]  name       Found name
  * @param[in]     flags      Match flags
  * @retval  -1   String not found
  * @retval  >=0  Found id
  **/

int
subextSubtlextFindString(char *prop_name,
  char *source,
  char **name,
  int flags)
{
  int ret = -1, size = 0;
  char **strings = NULL;
  regex_t *preg = NULL;

  assert(prop_name && source);

  /* Fetch data */
  preg    = subSharedRegexNew(source);
  strings = subSharedPropertyGetStrings(display, ROOT,
    XInternAtom(display, prop_name, False), &size);

  /* Check results */
  if(preg && strings)
    {
      int selid = -1, i;

      /* Special values */
      if(isdigit(source[0])) selid = atoi(source);

      for(i = 0; i < size; i++)
        {
          if(selid == i || (-1 == selid &&
              ((flags & SUB_MATCH_EXACT && 0 == strcmp(source, strings[i])) ||
              (preg && !(flags & SUB_MATCH_EXACT) &&
                subSharedRegexMatch(preg, strings[i])))))
            {
              if(name) *name = strdup(strings[i]);

              ret = i;
              break;
            }
        }
    }

  if(preg)    subSharedRegexKill(preg);
  if(strings) XFreeStringList(strings);

  return ret;
} /* }}} */

 /** subextSubtlextFindObjects {{{
  * @brief Find match in propery list and create objects
  * @param[in]  prop_name   Property name
  * @param[in]  class_name  Class name
  * @param[in]  source      Regexp source
  * @param[in]  flags       Match flags
  * @param[in]  first       Return first or all
  * @retval  Qnil    No match
  * @retval  Object  One match
  * @retval  Array   Multiple matches
  **/

VALUE
subextSubtlextFindObjects(char *prop_name,
  char *class_name,
  char *source,
  int flags,
  int first)
{
  int i, nstrings = 0;
  char **strings = NULL;
  VALUE ret = first ? Qnil : rb_ary_new();

  assert(prop_name && class_name && source);

  /* Check results */
  if((strings = subSharedPropertyGetStrings(display, ROOT,
      XInternAtom(display, prop_name, False), &nstrings)))
    {
      int selid = -1;
      VALUE meth_new = Qnil, meth_update = Qnil, klass = Qnil, obj = Qnil;
      regex_t *preg = subSharedRegexNew(source);

      /* Special values */
      if(isdigit(source[0])) selid = atoi(source);

      /* Fetch data */
      meth_new    = rb_intern("new");
      meth_update = rb_intern("update");
      klass       = rb_const_get(mod, rb_intern(class_name));

      /* Check each string */
      for(i = 0; i < nstrings; i++)
        {
          /* Check if string matches */
          if(selid == i || (-1 == selid &&
              ((flags & SUB_MATCH_EXACT && 0 == strcmp(source, strings[i])) ||
              (preg && !(flags & SUB_MATCH_EXACT) &&
                subSharedRegexMatch(preg, strings[i])))))
            {
              /* Create new object */
              if(RTEST((obj = rb_funcall(klass, meth_new, 1,
                  rb_str_new2(strings[i])))))
                {
                  rb_iv_set(obj, "@id", INT2FIX(i));

                  /* Call update method of object */
                  if(rb_respond_to(obj, meth_update))
                    rb_funcall(obj, meth_update, 0, Qnil);

                  /* Select first or many */
                  if(first)
                    {
                      ret = obj;

                      break;
                    }
                  else ret = subextSubtlextOneOrMany(obj, ret);
                }
            }
        }

      if(preg) subSharedRegexKill(preg);
      XFreeStringList(strings);
    }
  else rb_raise(rb_eStandardError, "Unknown property list `%s'", prop_name);

  return ret;
} /* }}} */

 /** subextSubtlextFindWindows {{{
  * @brief Find match in propery list and create objects
  * @param[in]  prop_name   Property name
  * @param[in]  class_name  Class name
  * @param[in]  source      Regexp source
  * @param[in]  flags       Match flags
  * @retval  Qnil    No match
  * @retval  Object  One match
  * @retval  Array   Multiple matches
  **/

VALUE
subextSubtlextFindWindows(char *prop_name,
  char *class_name,
  char *source,
  int flags,
  int first)
{
  int i, size = 0;
  Window *wins = NULL;
  VALUE ret = first ? Qnil : rb_ary_new();

  /* Get window list */
  if((wins = subextSubtlextWindowList(prop_name, &size)))
    {
      int selid = -1;
      Window selwin = None;
      VALUE meth_new = Qnil, meth_update = Qnil, klass = Qnil, obj = Qnil;
      regex_t *preg = NULL;

      /* Create regexp when required */
      if(!(flags & SUB_MATCH_EXACT)) preg = subSharedRegexNew(source);

      /* Special values */
      if(isdigit(source[0])) selid  = atoi(source);
      if('#' == source[0])   selwin = subextSubtleSingSelect(Qnil);

      /* Fetch data */
      meth_new    = rb_intern("new");
      meth_update = rb_intern("update");
      klass       = rb_const_get(mod, rb_intern(class_name));

      /* Check results */
      for(i = 0; i < size; i++)
        {
          if(selid == i || selid == wins[i] || selwin == wins[i] ||
              (-1 == selid && SubtlextWindowMatch(wins[i], preg,
              source, NULL, flags)))
            {
              /* Create new obj */
              if(RTEST((obj = rb_funcall(klass, meth_new,
                  1, LONG2NUM(wins[i])))))
                {
                  /* Call update method of object */
                  rb_funcall(obj, meth_update, 0, Qnil);

                  /* Select first or many */
                  if(first)
                    {
                      ret = obj;

                      break;
                    }
                  else ret = subextSubtlextOneOrMany(obj, ret);
                }
            }
        }

      if(preg) subSharedRegexKill(preg);
      free(wins);
    }

  return ret;
} /* }}} */

 /** subextSubtlextFindObjectsGeometry {{{
  * @brief Find match in propery list and create objects
  * @param[in]  prop_name   Property name
  * @param[in]  class_name  Class name
  * @param[in]  source      Regexp source
  * @param[in]  flags       Match flags
  * @param[in]  first       Return first or all
  * @retval  Qnil    No match
  * @retval  Object  One match
  * @retval  Array   Multiple matches
  **/

VALUE
subextSubtlextFindObjectsGeometry(char *prop_name,
  char *class_name,
  char *source,
  int flags,
  int first)
{
  int nstrings = 0;
  char **strings = NULL;
  VALUE ret = first ? Qnil : rb_ary_new();

  subextSubtlextConnect(NULL); ///< Implicit open connection

  /* Get string list */
  if((strings = subSharedPropertyGetStrings(display, DefaultRootWindow(display),
      XInternAtom(display, prop_name, False), &nstrings)))
    {
      int i, selid = -1;
      XRectangle geometry = { 0 };
      char buf[30] = { 0 };
      VALUE klass_obj = Qnil, klass_geom = Qnil, meth = Qnil;
      VALUE obj = Qnil, geom = Qnil;
      regex_t *preg = NULL;

      /* Fetch data */
      klass_obj  = rb_const_get(mod, rb_intern(class_name));
      klass_geom = rb_const_get(mod, rb_intern("Geometry"));
      meth       = rb_intern("new");

      /* Create if source is given */
      if(source)
        {
          if(isdigit(source[0])) selid = atoi(source);
          preg = subSharedRegexNew(source);
        }

      /* Create object list */
      for(i = 0; i < nstrings; i++)
        {
          sscanf(strings[i], "%hdx%hd+%hd+%hd#%s", &geometry.x, &geometry.y,
            &geometry.width, &geometry.height, buf);

          /* Check if string matches */
          if(!source || (source && (selid == i || (-1 == selid &&
              ((flags & SUB_MATCH_EXACT && 0 == strcmp(source, buf)) ||
              (preg && !(flags & SUB_MATCH_EXACT) &&
                subSharedRegexMatch(preg, buf)))))))
            {
              /* Create new object and geometry */
              obj  = rb_funcall(klass_obj, meth, 1, rb_str_new2(buf));
              geom = rb_funcall(klass_geom, meth, 4, INT2FIX(geometry.x),
                INT2FIX(geometry.y), INT2FIX(geometry.width),
                INT2FIX(geometry.height));

              rb_iv_set(obj, "@id",       INT2FIX(i));
              rb_iv_set(obj, "@geometry", geom);

              /* Select first or many */
              if(first)
                {
                  ret = obj;

                  break;
                }
              else ret = subextSubtlextOneOrMany(obj, ret);
            }
        }

      if(preg) subSharedRegexKill(preg);
      XFreeStringList(strings);
    }
  else rb_raise(rb_eStandardError, "Unknown property list `%s'", prop_name);

  return ret;
} /* }}} */

/* Plugin */

/* Init_subtlext {{{ */
/*
 * Subtlext is the module of the extension
 */
void
Init_subtlext(void)
{
  VALUE client = Qnil, color = Qnil, geometry = Qnil, gravity = Qnil;
  VALUE icon = Qnil, screen = Qnil, subtle = Qnil, sublet = Qnil;
  VALUE tag = Qnil, tray = Qnil, view = Qnil, window = Qnil;

 /*
   * Document-class: Subtlext
   *
   * Subtlext is the toplevel module
   */

  mod = rb_define_module("Subtlext");

  /* Subtlext version */
  rb_define_const(mod, "VERSION", rb_str_new2(PKG_VERSION));

  /*
   * Document-class: Subtlext::Client
   *
   * Class for interaction with clients
   */

  client = rb_define_class_under(mod, "Client", rb_cObject);

  /* Window id */
  rb_define_attr(client, "win",      1, 0);

  /* WM_NAME */
  rb_define_attr(client, "name",     1, 0);

  /* Instance of WM_CLASS */
  rb_define_attr(client, "instance", 1, 0);

  /* Class of WM_CLASS */
  rb_define_attr(client, "klass",    1, 0);

    /* Window role */
  rb_define_attr(client, "role",     1, 0);

  /* Bitfield of window states */
  rb_define_attr(client, "flags",    1, 0);

  /* Singleton methods */
  rb_define_singleton_method(client, "select",  subextClientSingSelect,  0);
  rb_define_singleton_method(client, "find",    subextClientSingFind,    1);
  rb_define_singleton_method(client, "first",   subextClientSingFirst,   1);
  rb_define_singleton_method(client, "current", subextClientSingCurrent, 0);
  rb_define_singleton_method(client, "visible", subextClientSingVisible, 0);
  rb_define_singleton_method(client, "list",    subextClientSingList,    0);
  rb_define_singleton_method(client, "recent",  subextClientSingRecent,  0);
  rb_define_singleton_method(client, "spawn",   subextClientSingSpawn,   1);

  /* General methods */
  rb_define_method(client, "has_tag?",    SubtlextTagAsk,           1);
  rb_define_method(client, "tags",        SubtlextTagReader,        0);
  rb_define_method(client, "tags=",       SubtlextTagWriter,        1);
  rb_define_method(client, "tag",         SubtlextTagAdd,           1);
  rb_define_method(client, "untag",       SubtlextTagDel,           1);
  rb_define_method(client, "retag",       SubtlextTagReload,        0);
  rb_define_method(client, "send_button", SubtlextSendButton,      -1);
  rb_define_method(client, "send_key",    SubtlextSendKey,         -1);
  rb_define_method(client, "focus",       SubtlextFocus,            0);
  rb_define_method(client, "has_focus?",  SubtlextAskFocus,         0);
  rb_define_method(client, "[]",          SubtlextPropReader,       1);
  rb_define_method(client, "[]=",         SubtlextPropWriter,       2);
  rb_define_method(client, "<=>",         SubtlextEqualSpaceWindow, 1);
  rb_define_method(client, "==",          SubtlextEqualWindow,      1);
  rb_define_method(client, "eql?",        SubtlextEqualTypedWindow, 1);
  rb_define_method(client, "hash",        SubtlextHash,             0);

  /* Class methods */
  rb_define_method(client, "initialize",        subextClientInit,                  1);
  rb_define_method(client, "update",            subextClientUpdate,                0);
  rb_define_method(client, "views",             subextClientViewList,              0);
  rb_define_method(client, "is_full?",          subextClientFlagsAskFull,          0);
  rb_define_method(client, "is_float?",         subextClientFlagsAskFloat,         0);
  rb_define_method(client, "is_stick?",         subextClientFlagsAskStick,         0);
  rb_define_method(client, "is_resize?",        subextClientFlagsAskResize,        0);
  rb_define_method(client, "is_urgent?",        subextClientFlagsAskUrgent,        0);
  rb_define_method(client, "is_zaphod?",        subextClientFlagsAskZaphod,        0);
  rb_define_method(client, "is_fixed?",         subextClientFlagsAskFixed,         0);
  rb_define_method(client, "is_borderless?",    subextClientFlagsAskBorderless,    0);
  rb_define_method(client, "toggle_full",       subextClientFlagsToggleFull,       0);
  rb_define_method(client, "toggle_float",      subextClientFlagsToggleFloat,      0);
  rb_define_method(client, "toggle_stick",      subextClientFlagsToggleStick,      0);
  rb_define_method(client, "toggle_resize",     subextClientFlagsToggleResize,     0);
  rb_define_method(client, "toggle_urgent",     subextClientFlagsToggleUrgent,     0);
  rb_define_method(client, "toggle_zaphod",     subextClientFlagsToggleZaphod,     0);
  rb_define_method(client, "toggle_fixed",      subextClientFlagsToggleFixed,      0);
  rb_define_method(client, "toggle_borderless", subextClientFlagsToggleBorderless, 0);
  rb_define_method(client, "flags=",            subextClientFlagsWriter,           1);
  rb_define_method(client, "raise",             subextClientRestackRaise,          0);
  rb_define_method(client, "lower",             subextClientRestackLower,          0);
  rb_define_method(client, "to_str",            subextClientToString,              0);
  rb_define_method(client, "gravity",           subextClientGravityReader,         0);
  rb_define_method(client, "gravity=",          subextClientGravityWriter,         1);
  rb_define_method(client, "geometry",          subextClientGeometryReader,        0);
  rb_define_method(client, "geometry=",         subextClientGeometryWriter,       -1);
  rb_define_method(client, "screen",            subextClientScreenReader,          0);
  rb_define_method(client, "pid",               SubtlextPidReader,                0);
  rb_define_method(client, "alive?",            subextClientAskAlive,              0);
  rb_define_method(client, "kill",              subextClientKill,                  0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(client), "[]",  "first");
  rb_define_alias(rb_singleton_class(client), "all", "list");

  /* Aliases */
  rb_define_alias(client, "+",     "tag");
  rb_define_alias(client, "-",     "untag");
  rb_define_alias(client, "save",  "update");
  rb_define_alias(client, "to_s",  "to_str");
  rb_define_alias(client, "click", "send_button");

  /*
   * Document-class: Subtlext::Color
   *
   * Color class for interaction with colors
   */

  color = rb_define_class_under(mod, "Color", rb_cObject);

  /* Red fraction */
  rb_define_attr(color, "red", 1, 0);

  /* Green fraction */
  rb_define_attr(color, "green", 1, 0);

  /* Blue fraction */
  rb_define_attr(color, "blue", 1, 0);

  /* Pixel number */
  rb_define_attr(color, "pixel", 1, 0);

  /* General methods */
  rb_define_method(color, "<=>",  SubtlextEqualSpacePixel, 1);
  rb_define_method(color, "hash", SubtlextHash,            0);

  /* Class methods */
  rb_define_method(color, "initialize", subextColorInit,         -1);
  rb_define_method(color, "to_hex",     subextColorToHex,         0);
  rb_define_method(color, "to_ary",     subextColorToArray,       0);
  rb_define_method(color, "to_hash",    subextColorToHash,        0);
  rb_define_method(color, "to_str",     subextColorToString,      0);
  rb_define_method(color, "+",          subextColorOperatorPlus,  1);
  rb_define_method(color, "==",         subextColorEqual,         1);
  rb_define_method(color, "eql?",       subextColorEqualTyped,    1);

  /* Aliases */
  rb_define_alias(color, "to_a", "to_ary");
  rb_define_alias(color, "to_h", "to_hash");
  rb_define_alias(color, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Geometry
   *
   * Class for various sizes
   */

  geometry = rb_define_class_under(mod, "Geometry", rb_cObject);

  /* X offset */
  rb_define_attr(geometry, "x",      1, 1);

  /* Y offset */
  rb_define_attr(geometry, "y",      1, 1);

  /* Geometry width */
  rb_define_attr(geometry, "width",  1, 1);

  /* Geometry height */
  rb_define_attr(geometry, "height", 1, 1);

  /* General methods */
  rb_define_method(geometry, "hash", SubtlextHash, 0);

  /* Class methods */
  rb_define_method(geometry, "initialize", subextGeometryInit,      -1);
  rb_define_method(geometry, "to_ary",     subextGeometryToArray,    0);
  rb_define_method(geometry, "to_hash",    subextGeometryToHash,     0);
  rb_define_method(geometry, "to_str",     subextGeometryToString,   0);
  rb_define_method(geometry, "==",         subextGeometryEqual,      1);
  rb_define_method(geometry, "eql?",       subextGeometryEqualTyped, 1);

  /* Aliases */
  rb_define_alias(geometry, "to_a", "to_ary");
  rb_define_alias(geometry, "to_h", "to_hash");
  rb_define_alias(geometry, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Gravity
   *
   * Class for Client placement
   */

  gravity = rb_define_class_under(mod, "Gravity", rb_cObject);

  /* Gravity id */
  rb_define_attr(gravity, "id",       1, 0);

  /* Name of the gravity */
  rb_define_attr(gravity, "name",     1, 0);

  /* Geometry */
  rb_define_attr(gravity, "geometry", 1, 0);

  /* Singleton methods */
  rb_define_singleton_method(gravity, "find",  subextGravitySingFind,  1);
  rb_define_singleton_method(gravity, "first", subextGravitySingFirst, 1);
  rb_define_singleton_method(gravity, "list",  subextGravitySingList,  0);

  /* General methods */
  rb_define_method(gravity, "<=>",  SubtlextEqualSpaceId, 1);
  rb_define_method(gravity, "==",   SubtlextEqualId,      1);
  rb_define_method(gravity, "eql?", SubtlextEqualTypedId, 1);
  rb_define_method(gravity, "hash", SubtlextHash,         0);

  /* Class methods */
  rb_define_method(gravity, "initialize",   subextGravityInit,           -1);
  rb_define_method(gravity, "save",         subextGravitySave,            0);
  rb_define_method(gravity, "clients",      subextGravityClients,         0);
  rb_define_method(gravity, "geometry_for", subextGravityGeometryFor,     1);
  rb_define_method(gravity, "geometry",     subextGravityGeometryReader,  0);
  rb_define_method(gravity, "geometry=",    subextGravityGeometryWriter, -1);
  rb_define_method(gravity, "tiling=",      subextGravityTilingWriter,    1);
  rb_define_method(gravity, "to_str",       subextGravityToString,        0);
  rb_define_method(gravity, "to_sym",       subextGravityToSym,           0);
  rb_define_method(gravity, "kill",         subextGravityKill,            0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(gravity), "[]",  "first");
  rb_define_alias(rb_singleton_class(gravity), "all", "list");

  /* Aliases */
  rb_define_alias(gravity, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Icon
   *
   * Icon class for interaction with icons
   */

  icon = rb_define_class_under(mod, "Icon", rb_cObject);

  /* Icon width */
  rb_define_attr(icon, "width",  1, 0);

  /* Icon height */
  rb_define_attr(icon, "height", 1, 0);

  /* Icon pixmap idt */
  rb_define_attr(icon, "pixmap", 1, 0);

  /* Allocate */
  rb_define_alloc_func(icon, subextIconAlloc);

  /* General methods */
  rb_define_method(icon, "<=>",  SubtlextEqualSpacePixmap, 1);
  rb_define_method(icon, "hash", SubtlextHash,             0);

  /* Class methods */
  rb_define_method(icon, "initialize", subextIconInit,         -1);
  rb_define_method(icon, "draw_point", subextIconDrawPoint,    -1);
  rb_define_method(icon, "draw_line",  subextIconDrawLine,     -1);
  rb_define_method(icon, "draw_rect",  subextIconDrawRect,     -1);
  rb_define_method(icon, "copy_area",  subextIconCopyArea,     -1);
  rb_define_method(icon, "clear",      subextIconClear,        -1);
  rb_define_method(icon, "bitmap?",    subextIconAskBitmap,     0);
  rb_define_method(icon, "to_str",     subextIconToString,      0);
  rb_define_method(icon, "+",          subextIconOperatorPlus,  1);
  rb_define_method(icon, "*",          subextIconOperatorMult,  1);
  rb_define_method(icon, "==",         subextIconEqual,         1);
  rb_define_method(icon, "eql?",       subextIconEqualTyped,    1);

  /* Aliases */
  rb_define_alias(icon, "to_s", "to_str");
  rb_define_alias(icon, "draw", "draw_point");

  /*
   * Document-class: Subtlext::Screen
   *
   * Class for interaction with screens
   */

  screen = rb_define_class_under(mod, "Screen", rb_cObject);

  /* Screen id */
  rb_define_attr(screen, "id",       1, 0);

  /* Geometry */
  rb_define_attr(screen, "geometry", 1, 0);

  /* Singleton methods */
  rb_define_singleton_method(screen, "find",    subextScreenSingFind,    1);
  rb_define_singleton_method(screen, "list",    subextScreenSingList,    0);
  rb_define_singleton_method(screen, "current", subextScreenSingCurrent, 0);

  /* General methods */
  rb_define_method(screen, "<=>",  SubtlextEqualSpaceId, 1);
  rb_define_method(screen, "==",   SubtlextEqualId,      1);
  rb_define_method(screen, "eql?", SubtlextEqualTypedId, 1);
  rb_define_method(screen, "hash", SubtlextHash,         0);

  /* Class methods */
  rb_define_method(screen, "initialize", subextScreenInit,       1);
  rb_define_method(screen, "update",     subextScreenUpdate,     0);
  rb_define_method(screen, "jump",       subextScreenJump,       0);
  rb_define_method(screen, "view",       subextScreenViewReader, 0);
  rb_define_method(screen, "view=",      subextScreenViewWriter, 1);
  rb_define_method(screen, "current?",   subextScreenAskCurrent, 0);
  rb_define_method(screen, "to_str",     subextScreenToString,   0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(screen), "[]",  "find");
  rb_define_alias(rb_singleton_class(screen), "all", "list");

  /* Aliases */
  rb_define_alias(screen, "save", "update");
  rb_define_alias(screen, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Subtle
   *
   * Module for interaction with the window manager
   */

  subtle = rb_define_module_under(mod, "Subtle");

  /* Singleton methods */
  rb_define_singleton_method(subtle, "display",       subextSubtleSingDisplayReader, 0);
  rb_define_singleton_method(subtle, "display=",      subextSubtleSingDisplayWriter, 1);
  rb_define_singleton_method(subtle, "select_window", subextSubtleSingSelect,        0);
  rb_define_singleton_method(subtle, "running?",      subextSubtleSingAskRunning,    0);
  rb_define_singleton_method(subtle, "render",        subextSubtleSingRender,        0);
  rb_define_singleton_method(subtle, "reload",        subextSubtleSingReload,        0);
  rb_define_singleton_method(subtle, "restart",       subextSubtleSingRestart,       0);
  rb_define_singleton_method(subtle, "quit",          subextSubtleSingQuit,          0);
  rb_define_singleton_method(subtle, "colors",        subextSubtleSingColors,        0);
  rb_define_singleton_method(subtle, "font",          subextSubtleSingFont,          0);

  /* Aliases */
  rb_define_alias(rb_singleton_class(subtle), "reload_config", "reload");

  /*
   * Document-class: Subtlext::Sublet
   *
   * Class for interaction with sublets
   */

  sublet = rb_define_class_under(mod, "Sublet", rb_cObject);

  /* Sublet id */
  rb_define_attr(sublet, "id",   1, 0);

  /* Name of the sublet */
  rb_define_attr(sublet, "name", 1, 0);

  /* Geometry */
  rb_define_attr(sublet, "geometry", 1, 0);

  /* Singleton methods */
  rb_define_singleton_method(sublet, "find",  subextSubletSingFind,  1);
  rb_define_singleton_method(sublet, "first", subextSubletSingFirst, 1);
  rb_define_singleton_method(sublet, "list",  subextSubletSingList,  0);

  /* General methods */
  rb_define_method(sublet, "<=>",    SubtlextEqualSpaceId, 1);
  rb_define_method(sublet, "==",     SubtlextEqualId,      1);
  rb_define_method(sublet, "eql?",   SubtlextEqualTypedId, 1);
  rb_define_method(sublet, "style=", SubtlextStyle,        1);
  rb_define_method(sublet, "hash",   SubtlextHash,         0);

  /* Class methods */
  rb_define_method(sublet, "initialize", subextSubletInit,           1);
  rb_define_method(sublet, "update",     subextSubletUpdate,         0);
  rb_define_method(sublet, "send_data",  subextSubletSend,           1);
  rb_define_method(sublet, "show",       subextSubletVisibilityShow, 0);
  rb_define_method(sublet, "hide",       subextSubletVisibilityHide, 0);
  rb_define_method(sublet, "to_str",     subextSubletToString,       0);
  rb_define_method(sublet, "kill",       subextSubletKill,           0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(sublet), "[]",  "first");
  rb_define_alias(rb_singleton_class(sublet), "all", "list");

  /* Aliases */
  rb_define_alias(sublet, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Tag
   *
   * Class for interaction with tags
   */

  tag = rb_define_class_under(mod, "Tag", rb_cObject);

  /* Tag id */
  rb_define_attr(tag,   "id",   1, 0);

  /* Name of the tag */
  rb_define_attr(tag,   "name", 1, 0);

  /* Singleton methods */
  rb_define_singleton_method(tag, "find",    subextTagSingFind,    1);
  rb_define_singleton_method(tag, "first",   subextTagSingFirst,   1);
  rb_define_singleton_method(tag, "visible", subextTagSingVisible, 0);
  rb_define_singleton_method(tag, "list",    subextTagSingList,    0);

  /* General methods */
  rb_define_method(tag, "<=>",  SubtlextEqualSpaceId, 1);
  rb_define_method(tag, "==",   SubtlextEqualId,      1);
  rb_define_method(tag, "eql?", SubtlextEqualTypedId, 1);
  rb_define_method(tag, "hash", SubtlextHash,         0);

  /* Class methods */
  rb_define_method(tag, "initialize", subextTagInit,     1);
  rb_define_method(tag, "save",       subextTagSave,     0);
  rb_define_method(tag, "clients",    subextTagClients,  0);
  rb_define_method(tag, "views",      subextTagViews,    0);
  rb_define_method(tag, "to_str",     subextTagToString, 0);
  rb_define_method(tag, "kill",       subextTagKill,     0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(tag), "[]",  "first");
  rb_define_alias(rb_singleton_class(tag), "all", "list");

  /* Aliases */
  rb_define_alias(tag, "to_s", "to_str");

  /*
   * Document-class: Subtlext::Tray
   *
   * Class for interaction with trays
   */

  tray = rb_define_class_under(mod, "Tray", rb_cObject);

  /* Window id */
  rb_define_attr(tray, "win",      1, 0);

  /* WM_NAME */
  rb_define_attr(tray, "name",     1, 0);

  /* Instance of WM_CLASS */
  rb_define_attr(tray, "instance", 1, 0);

  /* Class of WM_CLASS */
  rb_define_attr(tray, "klass",    1, 0);

  /* Singleton methods */
  rb_define_singleton_method(tray, "find",  subextTraySingFind,  1);
  rb_define_singleton_method(tray, "first", subextTraySingFirst, 1);
  rb_define_singleton_method(tray, "list",  subextTraySingList,  0);

  /* General methods */
  rb_define_method(tray, "send_button", SubtlextSendButton,      -1);
  rb_define_method(tray, "send_key",    SubtlextSendKey,         -1);
  rb_define_method(tray, "focus",       SubtlextFocus,            0);
  rb_define_method(tray, "has_focus?",  SubtlextAskFocus,         0);
  rb_define_method(tray, "[]",          SubtlextPropReader,       1);
  rb_define_method(tray, "[]=",         SubtlextPropWriter,       2);
  rb_define_method(tray, "<=>",         SubtlextEqualSpaceWindow, 1);
  rb_define_method(tray, "==",          SubtlextEqualWindow,      1);
  rb_define_method(tray, "eql?",        SubtlextEqualTypedWindow, 1);
  rb_define_method(tray, "hash",        SubtlextHash,             0);

  /* Class methods */
  rb_define_method(tray, "initialize", subextTrayInit,       1);
  rb_define_method(tray, "update",     subextTrayUpdate,     0);
  rb_define_method(tray, "pid",        SubtlextPidReader, 0);
  rb_define_method(tray, "to_str",     subextTrayToString,   0);
  rb_define_method(tray, "kill",       subextTrayKill,       0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(tray), "[]",  "first");
  rb_define_alias(rb_singleton_class(tray), "all", "list");

  /* Aliases */
  rb_define_alias(tray, "to_s",  "to_str");
  rb_define_alias(tray, "click", "send_button");

  /*
   * Document-class: Subtlext::View
   *
   * Class for interaction with views
   */

  view = rb_define_class_under(mod, "View", rb_cObject);

  /* View id */
  rb_define_attr(view, "id",   1, 0);

  /* Name of the view */
  rb_define_attr(view, "name", 1, 0);

  /* Singleton methods */
  rb_define_singleton_method(view, "find",    subextViewSingFind,    1);
  rb_define_singleton_method(view, "first",   subextViewSingFirst,   1);
  rb_define_singleton_method(view, "current", subextViewSingCurrent, 0);
  rb_define_singleton_method(view, "visible", subextViewSingVisible, 0);
  rb_define_singleton_method(view, "list",    subextViewSingList,    0);

  /* General methods */
  rb_define_method(view, "has_tag?", SubtlextTagAsk,       1);
  rb_define_method(view, "tags",     SubtlextTagReader,    0);
  rb_define_method(view, "tags=",    SubtlextTagWriter,    1);
  rb_define_method(view, "tag",      SubtlextTagAdd,       1);
  rb_define_method(view, "untag",    SubtlextTagDel,       1);
  rb_define_method(view, "[]",       SubtlextPropReader,   1);
  rb_define_method(view, "[]=",      SubtlextPropWriter,   2);
  rb_define_method(view, "<=>",      SubtlextEqualSpaceId, 1);
  rb_define_method(view, "==",       SubtlextEqualId,      1);
  rb_define_method(view, "eql?",     SubtlextEqualTypedId, 1);
  rb_define_method(view, "style=",   SubtlextStyle,        1);
  rb_define_method(view, "hash",     SubtlextHash,         0);

  /* Class methods */
  rb_define_method(view, "initialize", subextViewInit,          1);
  rb_define_method(view, "update",     subextViewUpdate,        0);
  rb_define_method(view, "save",       subextViewSave,          0);
  rb_define_method(view, "clients",    subextViewClients,       0);
  rb_define_method(view, "jump",       subextViewJump,          0);
  rb_define_method(view, "next",       subextViewSelectNext,    0);
  rb_define_method(view, "prev",       subextViewSelectPrev,    0);
  rb_define_method(view, "current?",   subextViewAskCurrent,    0);
  rb_define_method(view, "icon",       subextViewIcon,          0);
  rb_define_method(view, "to_str",     subextViewToString,      0);
  rb_define_method(view, "kill",       subextViewKill,          0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(view), "[]",  "first");
  rb_define_alias(rb_singleton_class(view), "all", "list");

  /* Aliases */
  rb_define_alias(view, "+",     "tag");
  rb_define_alias(view, "-",     "untag");
  rb_define_alias(view, "click", "jump");
  rb_define_alias(view, "to_s",  "to_str");

  /*
   * Document-class: Subtlext::Window
   *
   * Class for interaction with windows
   */

  window = rb_define_class_under(mod, "Window", rb_cObject);

  /* Window id */
  rb_define_attr(window, "win",    1, 0);

  /* Window visibility */
  rb_define_attr(window, "hidden", 1, 0);

  /* Allocate */
  rb_define_alloc_func(window, subextWindowAlloc);

  /* Singleton methods */
  rb_define_singleton_method(window, "once", subextWindowSingOnce, 1);

  /* General methods */
  rb_define_method(window, "send_button", SubtlextSendButton,      -1);
  rb_define_method(window, "send_key",    SubtlextSendKey,         -1);
  rb_define_method(window, "focus",       SubtlextFocus,            0);
  rb_define_method(window, "[]",          SubtlextPropReader,       1);
  rb_define_method(window, "[]=",         SubtlextPropWriter,       2);
  rb_define_method(window, "<=>",         SubtlextEqualSpaceWindow, 1);
  rb_define_method(window, "==",          SubtlextEqualWindow,      1);
  rb_define_method(window, "eql?",        SubtlextEqualTypedWindow, 1);

  /* Class methods */
  rb_define_method(window, "initialize",    subextWindowInit,              1);
  rb_define_method(window, "subwindow",     subextWindowSubwindow,         1);
  rb_define_method(window, "name=",         subextWindowNameWriter,        1);
  rb_define_method(window, "font=",         subextWindowFontWriter,        1);
  rb_define_method(window, "font_y",        subextWindowFontYReader,       0);
  rb_define_method(window, "font_height",   subextWindowFontHeightReader,  0);
  rb_define_method(window, "font_width",    subextWindowFontWidth,         1);
  rb_define_method(window, "foreground=",   subextWindowForegroundWriter,  1);
  rb_define_method(window, "background=",   subextWindowBackgroundWriter,  1);
  rb_define_method(window, "border_color=", subextWindowBorderColorWriter, 1);
  rb_define_method(window, "border_size=",  subextWindowBorderSizeWriter,  1);
  rb_define_method(window, "on",            subextWindowOn,               -1);
  rb_define_method(window, "draw_point",    subextWindowDrawPoint,        -1);
  rb_define_method(window, "draw_line",     subextWindowDrawLine,         -1);
  rb_define_method(window, "draw_rect",     subextWindowDrawRect,         -1);
  rb_define_method(window, "draw_text",     subextWindowDrawText,         -1);
  rb_define_method(window, "draw_icon",     subextWindowDrawIcon,         -1);
  rb_define_method(window, "clear",         subextWindowClear,            -1);
  rb_define_method(window, "redraw",        subextWindowRedraw,            0);
  rb_define_method(window, "geometry",      subextWindowGeometryReader,    0);
  rb_define_method(window, "geometry=",     subextWindowGeometryWriter,    1);
  rb_define_method(window, "raise",         subextWindowRaise,             0);
  rb_define_method(window, "lower",         subextWindowLower,             0);
  rb_define_method(window, "show",          subextWindowShow,              0);
  rb_define_method(window, "hide",          subextWindowHide,              0);
  rb_define_method(window, "hidden?",       subextWindowAskHidden,         0);
  rb_define_method(window, "kill",          subextWindowKill,              0);

  /* Singleton aliases */
  rb_define_alias(rb_singleton_class(window), "configure", "new");

  /* Aliases */
  rb_define_alias(window, "click", "send_button");
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
