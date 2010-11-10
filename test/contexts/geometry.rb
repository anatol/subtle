#
# @package test
#
# @file Test Subtlext::Geometry functions
# @author Christoph Kappel <unexist@dorfelite.net>
# @version $Id$
#
# This program can be distributed under the terms of the GNU GPL.
# See the file COPYING.
#

context "Geometry" do
  setup { Subtlext::Geometry.new(0, 0, 1, 1) }

  asserts("Init types and compare") do
    g1 = Subtlext::Geometry.new(topic)
    g2 = Subtlext::Geometry.new([ 0, 0, 1, 1 ])
    g3 = Subtlext::Geometry.new({x: 0, y: 0, width: 1, height: 1})

    g1 == g2 and g1 == g3
  end

  asserts("Check attributes") do
    0 == topic.x and 0 == topic.y and 1 == topic.width and 1 == topic.height
  end

  asserts("Type conversions") do
    hash = topic.to_hash
    ary  = topic.to_ary

    true
  end

  asserts("Convert to string") { "0x0+1+1" == topic.to_str }
end

# vim:ts=2:bs=2:sw=2:et:fdm=marker