#!/usr/bin/wish8.6

source itemutil.tcl

proc defaultfont {} {
    set f [[button ._] cget -font]
    destroy ._
    return $f
}

set style(font)     [eval font create fontNormal [font actual [defaultfont]]]
set style(boldfont) [eval font create fontBold   [font actual [defaultfont]] -weight bold]
set style(plugin-bg-color) #606060
set style(plugin-txt-color) #00F000
set style(plugin-port-active) white
set style(plugin-port-inactive) green
set style(queue-text-color) #ff9090
set style(queue-bg) #606060
set style(text-height) 16



button .quit -text quit -command {destroy .}
pack .quit

canvas .c -width 500 -height 500 -relief raised -bg black
pack .c

#
# compteur de groupes
#
set nextGroup 0

#
# compteur de ports
# un tag port0 port1 ... est donne au port et dans une ligne aussi
#
set nextPort 0

#
# chaque groupe peut etre un plugin, une queue ou autre chose
#


#
# moving stuff... keep x,y, and tag
#
# if a line has from-groupx or to-groupx , you must move it too
#

.c bind mobile <1> {
	global x y tag xoff yoff
	set x %X
	set y %Y
 	set t [%W itemcget current -tags]
 	set tag [lindex $t [lsearch $t "group*"]]
	puts "start move $x $y $tag"
	set xoff [expr [winfo x %W]+[winfo x .]]
	set yoff [expr [winfo y %W]+[winfo y .]]
}

.c bind mobile <B1-Motion> {
	global x y tag xoff yoff
	#puts "moving %X %Y"
	set dx [expr %X-$x]
	set dy [expr %Y-$y]
	%W move $tag $dx $dy
	# pour deplacer le cote 'port' de la ligne...
	# trouve les lignes, et pour chaque ligne, trouve le portxx et utilise ca comme reference
	foreach line [%W find withtag from-$tag] {
		set t [%W gettags $line]
		set ref [lindex $t [lsearch $t prt*]]
		%W imove $line 2 {*}[itemGetCenter %W (ports&&$ref)]
	}
	#%W imove from-$tag 2 [expr %X-$xoff] [expr %Y-$yoff]
	%W imove to-$tag 0 {*}[itemGetCenter %W $tag]
	set x %X
	set y %Y
}

#
# line stuff
#

.c bind line <Enter> {
	global style
	%W itemconfig current -fill red
	puts "bbox is [%W bbox current]"
}

.c bind line <Leave> {
	global style
	%W itemconfig current -fill yellow
}


#
# port stuff
#

.c bind ports <Enter> {
	global style
	%W itemconfig current -fill $style(plugin-port-active)
	puts "bbox is [%W bbox current]"
}

.c bind ports <Leave> {
	global style
	%W itemconfig current -fill $style(plugin-port-inactive)
}



.c bind ports <1> {
	global x y x0 y0
	set x %X
	set y %Y
	%W addtag startport withtag current
	lassign [itemGetCenter %W current] x0 y0
	%W create line $x0 $y0 $x0 $y0 -tag newline -fill yellow -width 3
	puts "start line $x0 $y0"
	break
}

.c bind ports <B1-Motion> {
	global x y x0 y0
	set dx [expr %X-$x]
	set dy [expr %Y-$y]
	set x0 [expr $x0+%X-$x]
	set y0 [expr $y0+%Y-$y]
	%W imove newline 0 $x0 $y0
	#puts "moving line to $x0 $y0 d=$dx $dy"
	set x %X
	set y %Y
	break
}
.c bind ports <B1-ButtonRelease> {
	global x y x0 y0
	# check if the line ends up on something interesting
	puts "yo xy=$x $y, xy0=$x0 $y0"
	#%W addtag endport closest $x0 $y0 1 newline
	set it [%W find overlapp $x0 $y0 $x0 $y0]
	#%W itemconfigure endport -fill blue
	puts "got [llength $it] : $it"
	foreach i $it {
		puts "item $i has tags [%W gettags $i]"
		set t [%W gettags $i]
		if { [lsearch $t queue] >= 0 } {
			# found the queue
			# move the endpoint to the center of the queue
			%W imove newline 1 {*}[itemGetCenter %W $i]
			# raise the port, and the queue
			%W raise startport
			set togroup [lindex $t [lsearch $t group*]]
			%W raise $togroup

			set t1 [%W gettags startport]
			set fromgroup [lindex $t1 [lsearch $t1 group*]]
			set fromport [lindex $t1 [lsearch $t1 prt*]]
			%W addtag line withtag newline
			%W addtag $fromport withtag newline
			%W addtag from-$fromgroup withtag newline
			%W addtag to-$togroup withtag newline
			puts "line is [%W gettags newline]"
			%W dtag newline newline
			break
		}
	}
	%W dtag startport startport
	%W delete newline
}




#
# Un plugin
# on a un item de base (qui sert pour le mouvement) et le reste suit
#
proc plugin {c x y txtName txtType ins outs} {
	global nextGroup style nextPort
	set g group$nextGroup
	incr nextGroup

	set tid0 [$c create text $x $y -text $txtName -anchor nw -font $style(boldfont) -fill $style(plugin-txt-color) -tags "mobile $g"]
	set tid1 [$c create text $x $y -text $txtType -anchor nw -font $style(font) -fill $style(plugin-txt-color) -tags "mobile $g"]

	itemYSequence $c $tid0 $tid1
	set allid "$tid0 $tid1"

	# the IN ports
	set allin {}
	set prev $tid1
	foreach p $ins {
		set t prt$nextPort
		incr nextPort

		set pid0 [$c create oval $x $y [expr $x+10] [expr $y+10] -fill $style(plugin-port-inactive) -tags "ports $g $t" -outline white]
		set pid1 [$c create text $x $y -text $p -anchor nw -font $style(font) -fill white -tags "mobile $g"]
		itemXSequence $c $pid0 $pid1 5
		itemVCenter $c $pid0 $pid1
		itemYSequence $c $prev "$pid0 $pid1" 5
		set prev "$pid0 $pid1"
		lappend allid $pid0 $pid1
		lappend allin $pid0 $pid1
	}
	# the OUT ports
	set allout {}
	set prev $tid1
	foreach p $outs {
		set t prt$nextPort
		incr nextPort

		set pid0 [$c create oval $x $y [expr $x+10] [expr $y+10] -fill $style(plugin-port-inactive) -tags "ports $g $t" -outline white]
		set pid1 [$c create text $x $y -text $p -anchor nw -font $style(font) -fill white -tags "mobile $g"]
		itemXSequence $c $pid1 $pid0 5
		itemVCenter $c $pid0 $pid1
		itemYSequence $c $prev "$pid0 $pid1" 5
		set prev "$pid0 $pid1"
		lappend allid $pid0 $pid1
		lappend allout $pid0 $pid1
		itemXAlign $c $allout "$pid0 $pid1" right
	}
	itemXSequence $c $allin $allout 10

	# the rectangle under all this
	set bbox [itemCover $c $allid 5]
	puts "bbox is $bbox"
	set id [$c create rectangle {*}$bbox -tags "mobile plugin $g" -fill $style(plugin-bg-color)]
	$c lower $id

}


#
# Un plugin
# on a un item de base (qui sert pour le mouvement) et le reste suit
#
proc queue {c x y name} {
	global nextGroup style
	set g group$nextGroup
	incr nextGroup

	set tid0 [$c create text $x $y -text $name -anchor nw -font $style(boldfont) -fill $style(queue-text-color) -tags "mobile $g"]
	set bbox [itemCover $c $tid0 10]
	set pid0 [$c create oval {*}$bbox -fill $style(queue-bg) -tags "mobile queue $g" -outline white]
	$c lower $pid0
}



plugin .c 50 50 "N" "Net" {in cmd} {out}
plugin .c 180 50 "V" "ViewerGLFW" {in cmd} {out}
queue .c  50 200 "recycle"
queue .c  50 250 "display"





