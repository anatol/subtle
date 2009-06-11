#
# example - subtlext demo script
# Copyright (c) 2005-2009 Christoph Kappel
#
# This program can be distributed under the terms of the GNU GPL.
# See the file COPYING.
#
# $Id$
#

require("subtle/subtlext")

subtle = Subtlext::Subtle.new(":2")

puts "subtle %s on %s" % 
  [subtle.version, subtle.running? ? subtle.display : "none"]

puts "Tags: %s" % [subtle.tags.join(", ")]

# Views
views = []
subtle.views.each do |v|
  views.push("%s (%s)" % [v.current? ? "[#{v}]" : v, v.tags.join(", ")])
end
puts "Views: %s" % [views.join(", ")]

# Clients
clients = []
subtle.clients.each do |c|
  clients.push("%s (%s)" % [c, c.tags.join(", ")])
end
puts "Clients: %s" % [clients.join(", ")]
