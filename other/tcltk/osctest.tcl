#!/usr/bin/wish

source osc.tcl


proc first {} {
	toplevel .a
	wm geometry .a +200+50
	::osc::initIn 10000 udp
	ttk::label .a.a1 -text IN
	ttk::scale .a.s1 -orient horizontal -from 0 -to 100
	ttk::label .a.a2 -text "(0,0)"
	ttk::button .a.q -text "quit" -command {::osc::uninitIn 10000;destroy .a;destroy .}
	pack .a.a1 .a.s1 .a.a2 .a.q -padx 5 -pady 5 -fill x
}
proc /pos {v} { .a.s1 set $v }
proc /mouse {x y} { .a.a2 config -text "( $x , $y )" }


proc second {} {
	::osc::initOut first localhost 10000 udp
	toplevel .b
	wm geometry .b +400+50

	ttk::button .b.b1 -text "go" -command {::osc::send first /pos i 50}
	ttk::label .b.a1 -text OUT
	ttk::scale .b.s1 -orient horizontal -command {::osc::send first /pos f } -from 0 -to 100
	canvas .b.c1 -width 200 -height 200 -backg red
	bind .b.c1 <B1-Motion> {::osc::send first /mouse ii %x %y}
	ttk::button .b.q -text "quit" -command {::osc::uninitOut first;destroy .b;destroy .}
	pack .b.b1 .b.a1 .b.s1 .b.c1 .b.q -padx 5 -pady 5 -fill x

}

proc blub {} {
	pack [ttk::notebook .nb]
		.nb add [frame .nb.f1] -text "First tab"
		.nb add [frame .nb.f2] -text "Second tab"
		.nb select .nb.f2
	ttk::notebook::enableTraversal .nb
	set names [ttk::style theme names]
	menubutton .mb -text Theme -underline 0 -menu .mb.m
	menu .mb.m -tearoff 0
	foreach n $names {
		.mb.m add command -label $n  -underline 2 -command "ttk::style theme use $n"
	}
	#.mb.m add command -label "Exit" -underline 1 -command exit
	pack .mb
}



first
second
blub
puts [ttk::style theme names]
puts [ttk::style theme use]
#wm withdraw .




