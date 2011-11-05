 /**
  * @package subtle
  *
  * @file subtle ruby extension
  * @copyright (c) 2005-2011 Christoph Kappel <unexist@subforge.org>
  * @version $Id$
  *
  * This program can be distributed under the terms of the GNU GPLv2.
  * See the file COPYING for details.
  **/

#include "subtlext.h"

/* SubletFind {{{ */
static VALUE
SubletFind(VALUE value,
  int first)
{
  int flags = 0;
  VALUE parsed = Qnil;
  char buf[50] = { 0 };

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Check object type */
  switch(rb_type(parsed = subSubtlextParse(
      value, buf, sizeof(buf), &flags)))
    {
      case T_SYMBOL:
        if(CHAR2SYM("all") == parsed)
          return subSubletSingList(Qnil);
        break;
      case T_OBJECT:
        if(rb_obj_is_instance_of(value, rb_const_get(mod, rb_intern("Sublet"))))
          return parsed;
    }

  return subSubtlextFindObjectsGeometry("SUBTLE_SUBLET_LIST",
    "Sublet", buf, flags, first);
} /* }}} */

/* Singleton */

/* subSubletSingFind {{{ */
/*
 * call-seq: find(value) -> Array
 *           [value]     -> Array
 *
 * Find Sublet by a given <i>value</i> which can be of following type:
 *
 * [Fixnum] Array index of the <code>SUBTLE_SUBLET_LIST</code> property list.
 * [String] Regexp match against name of Sublets, returns a Sublet on single
 *          match or an Array on multiple matches.
 * [Symbol] Either <i>:all</i> for an array of all Sublets or any string for
 *          an <b>exact</b> match.
 *
 *  Subtlext::Sublet.find(1)
 *  => [#<Subtlext::Sublet:xxx>]
 *
 *  Subtlext::Sublet.find("subtle")
 *  => [#<Subtlext::Sublet:xxx>]
 *
 *  Subtlext::Sublet[".*"]
 *  => [#<Subtlext::Sublet:xxx>, #<Subtlext::Sublet:xxx>]
 *
 *  Subtlext::Sublet["subtle"]
 *  => []
 *
 *  Subtlext::Sublet[:clock]
 *  => [#<Subtlext::Sublet:xxx>]

 */

VALUE
subSubletSingFind(VALUE self,
  VALUE value)
{
  return SubletFind(value, False);
} /* }}} */

/* subSubletSingFirst {{{ */
/*
 * call-seq: first(value) -> Subtlext::Sublet or nil
 *
 * Find first Sublet by a given <i>value</i> which can be of following type:
 *
 * [Fixnum] Array index of the <code>SUBTLE_SUBLET_LIST</code> property list.
 * [String] Regexp match against name of Sublets, returns a Sublet on single
 *          match or an Array on multiple matches.
 * [Symbol] Either <i>:all</i> for an array of all Sublets or any string for
 *          an <b>exact</b> match.
 *
 *  Subtlext::Sublet.first(1)
 *  => #<Subtlext::Sublet:xxx>
 *
 *  Subtlext::Sublet.first("subtle")
 *  => #<Subtlext::Sublet:xxx>
 */

VALUE
subSubletSingFirst(VALUE self,
  VALUE value)
{
  return SubletFind(value, True);
} /* }}} */

/* subSubletSingList {{{ */
/*
 * call-seq: list -> Array
 *
 * Get an array of all Sublets based on the <code>SUBTLE_SUBLET_LIST</code>
 * property list.
 *
 *  Subtlext::Sublet.list
 *  => [#<Subtlext::Sublet:xxx>, #<Subtlext::Sublet:xxx>]
 *
 *  Subtlext::Sublet.list
 *  => []
 */

VALUE
subSubletSingList(VALUE self)
{
  return subSubtlextFindObjectsGeometry("SUBTLE_SUBLET_LIST",
    "Sublet", NULL, 0, False);
} /* }}} */

/* Class */

/* subSubletInstantiate {{{ */
VALUE
subSubletInstantiate(char *name)
{
  VALUE klass = Qnil, sublet = Qnil;

  /* Create new instance */
  klass  = rb_const_get(mod, rb_intern("Sublet"));
  sublet = rb_funcall(klass, rb_intern("new"), 1, rb_str_new2(name));

  return sublet;
} /* }}} */

/* subSubletInit {{{ */
/*
 * call-seq: new(name) -> Subtlext::Sublet
 *
 * Create new Sublet object locally <b>without</b> calling #save automatically.
 *
 *  sublet = Subtlext::Sublet.new("subtle")
 *  => #<Subtlext::Sublet:xxx> *
 */

VALUE
subSubletInit(VALUE self,
  VALUE name)
{
  if(T_STRING != rb_type(name))
    rb_raise(rb_eArgError, "Unexpected value-type `%s'",
      rb_obj_classname(name));

  /* Init object */
  rb_iv_set(self, "@id",   Qnil);
  rb_iv_set(self, "@name", name);

  subSubtlextConnect(NULL); ///< Implicit open connection

  return self;
} /* }}} */

/* subSubletUpdate {{{ */
/*
 * call-seq: update -> nil
 *
 * Force subtle to update the data of this Sublet.
 *
 *  sublet.update
 *  => nil
 */

VALUE
subSubletUpdate(VALUE self)
{
  VALUE id = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  /* Send message */
  data.l[0] = FIX2INT(id);

  subSharedMessage(display, DefaultRootWindow(display),
    "SUBTLE_SUBLET_UPDATE", data, 32, True);

  return Qnil;
} /* }}} */

/* subSubletDataReader {{{ */
/*
 * call-seq: data -> String
 *
 * Get data of this Sublet.
 *
 *  puts sublet.data
 *  => "subtle"
 */

VALUE
subSubletDataReader(VALUE self)
{
  VALUE id = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  return id;
} /* }}} */

/* subSubletDataWriter {{{ */
/*
 * call-seq: data=(string) -> String
 *
 * Set data of this Sublet.
 *
 *  sublet.data = "subtle"
 *  => "subtle"
 */

VALUE
subSubletDataWriter(VALUE self,
  VALUE value)
{
  VALUE id = Qnil;

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  /* Check object type */
  if(T_STRING == rb_type(value))
    {
      char *list = NULL;
      SubMessageData data = { { 0, 0, 0, 0, 0 } };

      /* Store data */
      list = strdup(RSTRING_PTR(value));
      subSharedPropertySetStrings(display, DefaultRootWindow(display),
        XInternAtom(display, "SUBTLE_DATA", False), &list, 1);
      free(list);

      data.l[0] = FIX2LONG(id);

      subSharedMessage(display, DefaultRootWindow(display),
        "SUBTLE_SUBLET_DATA", data, 32, True);
    }
  else rb_raise(rb_eArgError, "Unexpected value-type `%s'",
    rb_obj_classname(value));

  return value;
} /* }}} */

/* subSubletVisibilityShow {{{ */
/*
 * call-seq: show -> nil
 *
 * Show sublet in the panels.
 *
 *  sublet.show
 *  => nil
 */

VALUE
subSubletVisibilityShow(VALUE self)
{
  VALUE id = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  data.l[0] = FIX2LONG(id);
  data.l[1] = SUB_EWMH_VISIBLE;

  subSharedMessage(display, DefaultRootWindow(display),
    "SUBTLE_SUBLET_FLAGS", data, 32, True);

  return Qnil;
} /* }}} */

/* subSubletVisibilityHide {{{ */
/*
 * call-seq: hide -> nil
 *
 * Hide sublet from the panels.
 *
 *  sublet.hide
 *  => nil
 */

VALUE
subSubletVisibilityHide(VALUE self)
{
  VALUE id = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  data.l[0] = FIX2LONG(id);
  data.l[1] = SUB_EWMH_HIDDEN;

  subSharedMessage(display, DefaultRootWindow(display),
    "SUBTLE_SUBLET_FLAGS", data, 32, True);

  return Qnil;
} /* }}} */

/* subSubletToString {{{ */
/*
 * call-seq: to_str -> String
 *
 * Convert Sublet object to string.
 *
 *  puts sublet
 *  => sublet
 */

VALUE
subSubletToString(VALUE self)
{
  VALUE name = Qnil;

  /* Check ruby object */
  GET_ATTR(self, "@name", name);

  return name;
} /* }}} */

/* subSubletKill {{{ */
/*
 * call-seq: kill -> nil
 *
 * Remove this Sublet from subtle and <b>freeze</b> this object.
 *
 *  sublet.kill
 *  => nil
 */

VALUE
subSubletKill(VALUE self)
{
  VALUE id = Qnil;
  SubMessageData data = { { 0, 0, 0, 0, 0 } };

  /* Check ruby object */
  rb_check_frozen(self);
  GET_ATTR(self, "@id", id);

  subSubtlextConnect(NULL); ///< Implicit open connection

  /* Send message */
  data.l[0] = FIX2INT(id);

  subSharedMessage(display, DefaultRootWindow(display),
    "SUBTLE_SUBLET_KILL", data, 32, True);

  rb_obj_freeze(self); ///< Freeze object

  return Qnil;
} /* }}} */

// vim:ts=2:bs=2:sw=2:et:fdm=marker
