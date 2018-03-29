#!/usr/bin/wish8.6

##+##########################################################################
#
# Manhole.tcl
# 
# Draws N-sided manhole covers
# by Keith Vetter
#
# Revisions:
# KPV Mar 22, 1996 - initial revision
# KPV Sep 22, 2002 - cleaned up for 8+
#

package require Tk

proc Init {} {
    global sz

    set sz(n) 3                                 ;# Number of sides
    set sz(s) 400                               ;# Size of canvas
    set sz(cx) [expr {$sz(s) / 2}]              ;# Canvas center point
    set sz(cy) $sz(cx)
    set sz(r) [expr {$sz(cx) * 3 / 4}]          ;# Radius
    set sz(rot) 0                               ;# How much to rotate by
    set sz(anim) 0                              ;# No animation yet
    set sz(after) ""                            ;# No after yet
    set sz(a) 0                                 ;# Interior angle to fill in
    set sz(colored) 1                           ;# Colored or solid

    set colors "cyan green magenta blue deepskyblue hotpink aquamarine "
    append colors $colors
    for {set i 0} {$i < 13} {incr i} {
        set sz($i) [lindex $colors $i]
    }

    canvas .c -width $sz(s) -height $sz(s) -bd 2 -relief raised
    .c config -bg black
    .c create oval [expr {$sz(cx)-$sz(r)}] [expr {$sz(cy)-$sz(r)}] \
        [expr {$sz(cx)+$sz(r)}] [expr {$sz(cy)+$sz(r)}] -tag circle \
        -fill [lindex [.c config -bg] 3]
    
    button .anim -text Animate -command {Animate 1}
    label .l -text "Sides: $sz(n)"
    scale .s -orient h -showvalue 0 -from 0 -to 5 -command MyScale

    pack .c -side top
    pack .anim -side right -expand 1
    pack .s .l -side bottom -expand 1
    wm resizable . 0 0
}
##+##########################################################################
# 
# ngon
# 
# Compute the vertices for a n-gon
# 
proc ngon {n angle} {
    global v sz

    catch {unset v}
    set delta [expr {2*3.14159 / $n}]           ;# Angle of vertices on circle
    set sz(delta) [expr {360.0 / $n}]
    set sz(a) [expr {180.0 / $n}]               ;# Interior angle to fill in

    set angle [expr {$angle * 3.14159 / 180}]
    for {set i 0} {$i < $n} {incr i} {
        set a [expr {$angle + ($i*$delta)}]     ;# Angle in radians
        set v($i,x) [expr {$sz(cx) + $sz(r) * cos($a)}]
        set v($i,y) [expr {$sz(cy) + $sz(r) * sin($a)}]
        set i2 [expr {$i + $n}]
        set v($i2,x) $v($i,x)
        set v($i2,y) $v($i,y)
        
        lappend vertices $v($i,x) $v($i,y)
    }

    set n2 [expr {$n/2}]                        ;# Opposite angle
    set x [expr {$v(0,x) - $v($n2,x)}]
    set y [expr {$v(0,y) - $v($n2,y)}]
    set sz(d) [expr {sqrt($x*$x + $y*$y)}]      ;# Length of opposite side


    for {set i 0} {$i < $n} {incr i} {
        set v($i,bb) [list [expr {$v($i,x)-$sz(d)}] [expr {$v($i,y)+$sz(d)}] \
                          [expr {$v($i,x)+$sz(d)}] [expr {$v($i,y)-$sz(d)}]]
        
        set i2 [expr {$i + 1}]
        set n2 [expr {($i + ($sz(n) / 2) + 1) % $sz(n)}]
        set xy [list $sz(cx) $sz(cy) $v($i,x) $v($i,y) $v($i2,x) $v($i2,y)]
        .c create poly $xy -fill $sz($n2) -outline $sz($n2) \
            -tag {poly poly_$i}
    }
    
    return $vertices
}
##+##########################################################################
# 
# DrawPie
# 
# Draws a single pie slice for vertex which.
# 
proc DrawPie {which} {
    global v sz

    if {$which == 0} {
        set n2 [expr {$which + ($sz(n) / 2) + 1}] ;# Opposite angle
        set x [expr {$v($n2,x) - $v($which,x)}]
        set y [expr {-($v($n2,y) - $v($which,y))}]
        set a [expr {atan2( $y, $x) * 180 / 3.14159}]
        set sz(atan) $a
    } else {
        set a [set sz(atan) [expr {$sz(atan) - $sz(delta)}]]
    }
    
    eval .c create arc $v($which,bb) -start $a -extent $sz(a) -style chord \
        -fill $sz($which) -outline $sz($which) -tag {{pie pie_$which}}
}
##+##########################################################################
# 
# DrawIt
# 
# Draws the n-side manhole cover w/ sz(n) sides at angle sz(rot).
# 
proc DrawIt {} {
    global sz

    .c delete pie poly
    ngon $sz(n) $sz(rot)                        ;# Get vertices for this angle
    for {set i 0} {$i < $sz(n)} {incr i} {      ;# Draw the pie slices
        DrawPie $i
    }
    update
}
##+##########################################################################
# 
# Animate
# 
# Draw the figure rotated by a small amount, then if animation is on,
# it schedules itself to be run again in the near future.
# 
proc Animate {toggle} {
    global sz

    if {$toggle} {                              ;# On/off toggle
        set sz(anim) [expr {1 - $sz(anim)}]     ;# Toggle to flag
        if {$sz(anim)} {set relief sunken} {set relief raised}
        .anim config -relief $relief
    }

    if $sz(anim) {                              ;# Are we animating???
        incr sz(rot) 3                          ;# Rotate a bit
        DrawIt                                  ;# Redraw it

        after 1 {Animate 0}                    ;# Rerun in the future
    }
}
##+##########################################################################
# 
# MyScale
# 
# Command called when scale gets a new value
# 
proc MyScale {v} {
    set ::sz(n) [expr {$v*2+3}]
    .l config -text "Sides: $::sz(n)"
    DrawIt
}
Init
DrawIt

