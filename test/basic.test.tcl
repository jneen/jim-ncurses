#!./deps/jimtcl/jimsh

load "./lib/ncurses.so"

puts "calling ncurses.window..."
set win [ncurses.window 0 0 1 2]
puts "return value:"
puts $win

$win puts "puts with ncurses"
