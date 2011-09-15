#!./deps/jimtcl/jimsh

puts "######### loading library"
load "./lib/ncurses.so"

puts "######### calling ncurses.init"
ncurses.init

#puts "######### calling ncurses.window..."
set win [ncurses.window 0 0 1 2]
#puts "######### return value:"
#puts $win

#puts "######### calling ($win puts)"
$win puts "puts with ncurses"

ncurses.end
