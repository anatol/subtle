#!/usr/bin/ruby
#
# @package test
#
# @file Run riot unit tests
# @author Christoph Kappel <unexist@dorfelite.net>
# @version $Id$
#
# This program can be distributed under the terms of the GNU GPL.
# See the file COPYING.
#

require "mkmf"
require "riot"

def fork_and_forget(cmd)
  pid = Process.fork
  if(pid.nil?)
    exec(cmd)
  else
    Process.detach(pid)
  end
end

# Find xterm
if((xterm = find_executable0("xterm")).nil?)
  raise "xterm not found in path"
end

# Configuration
subtle   = "../subtle"
subtlext = "../subtlext.so"
config   = "../contrib/subtle.rb"
sublets  = "./sublet"
display  = ":10"

# Start environment
fork_and_forget("#{subtle} -d #{display} -c #{config} -s #{sublets} &>/dev/null")

sleep 1

require subtlext

Subtlext::Subtle.display = display

# Run tests
require_relative "contexts/subtle_init.rb"
require_relative "contexts/color.rb"
require_relative "contexts/geometry.rb"
require_relative "contexts/gravity.rb"
require_relative "contexts/icon.rb"
require_relative "contexts/screen.rb"
require_relative "contexts/sublet.rb"
require_relative "contexts/tag.rb"
require_relative "contexts/view.rb"
require_relative "contexts/client.rb"
require_relative "contexts/subtle_finish.rb"

# vim:ts=2:bs=2:sw=2:et:fdm=marker