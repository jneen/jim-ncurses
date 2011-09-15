lappend auto_path "./lib"

# package require ncurses
source "./lib/ncurses.tcl"

ncurses.do {
	#puts "######### calling ncurses.window..."
	set win [ncurses.window 0 0 1 2]
	#puts "######### return value:"
	#puts $win

	#puts "######### calling ($win puts)"
	$win puts "puts with ncurses"
	$win box
	$win refresh
	ncurses.getc
}
