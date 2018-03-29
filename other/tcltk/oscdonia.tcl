#!/usr/bin/wish

source osc.tcl

proc firstold {} {

	::osc::init local localhost 12345
	::osc::init out localhost 23456

	proc out {args} {
		puts $args
		puts ".s1 set [lindex $args 1] [expr [lindex $args 1]+0.1]"
		.s1 set [lindex $args 1] [expr [lindex $args 1]+0.1]
	}

	proc scale1 {p} { ::osc::send local /pos ii 1234 $p }

	button .b1 -text "command 1" -command {::osc::send local {/abcd isi 12 "bonjour ca va?" 34} {/efgh ii 1 2}}
	button .b2 -text "command 2" -command {::osc::send local {/abcd isi 12 hello 34} {/test TTF}}
	button .b3 -text "command 3" -command {::osc::send out {/abcd isi 12 aa 34} {/test TTF}}
	button .b4 -text "command 4" -command {::osc::send * {/abcd isi 12 bb 34} {/test TTF}}
	scale .s1 -orient horizontal -command scale1
	canvas .c1 -width 100 -height 100 -backg red
	bind .c1 <B1-Motion> {::osc::send local /mouse iii %x %y %t}

	button .q -text "quit" -command {::osc::uninit {local out};destroy .}

	pack .b1 .b2 .b3 .b4 .s1 .c1 .q -fill x -pady 5 -padx 5
}


proc first {} {
	toplevel .a
	wm geometry .a +200+50
	::osc::initIn 10000
	ttk::label .a.a1 -text IN
	ttk::scale .a.s1 -orient horizontal -from 0 -to 100
	ttk::label .a.a2 -text "(0,0)"
	ttk::button .a.q -text "quit" -command {::osc::uninitIn 10000;destroy .a;destroy .}
	pack .a.a1 .a.s1 .a.a2 .a.q -padx 5 -pady 5 -fill x
}
proc /pos {v} { .a.s1 set $v }
proc /mouse {x y} { .a.a2 config -text "( $x , $y )" }


proc second {} {
	::osc::initOut first localhost 10000
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


::osc::initOut blub 192.168.2.130 9997
button .b -text go -command {::osc::send blub /allo ii 1 2}
ttk::scale .s1 -orient horizontal -command {::osc::send blub /pos f } -from 0 -to 100

pack .b .s1

#first
#second
#blub
#puts [ttk::style theme names]
#puts [ttk::style theme use]
#wm withdraw .




