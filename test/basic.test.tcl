lappend auto_path "./lib"

# package require ncurses
source "./lib/ncurses.tcl"

ncurses.do {
	set win [ncurses.window 10 10 20 20]

	$win box
	$win puts "puts with ncurses\n"
	ncurses.getc
}
