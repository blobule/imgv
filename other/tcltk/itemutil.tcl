
#
# practical stuff with canvas items
#


#
# move items around...
# put item b vertically aligned with a
#
proc itemVCenter {c a b {pad 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        lassign [$c bbox {*}$b] bx1 by1 bx2 by2
	# ay1-ay2 by1=by2
	# (ay1+ay2)/2 - (by1+by2)/2
        set dy [expr ($ay1+$ay2-$by1-$by2)/2]
        foreach t $b { $c move $t 0 $dy }
}

#
# move items around...
# put item b aligned on the bbox of a (left,right,center)
#
proc itemXAlign {c a b anchor {pad 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        lassign [$c bbox {*}$b] bx1 by1 bx2 by2
	switch $anchor {
	  left		{set dx [expr $ax1-$bx1]}
	  center	{set dx [expr ($ax1+$ax2-$bx1-$bx2)/2]}
	  right		{set dx [expr $ax2-$bx2]}
	}
        foreach t $b { $c move $t $dx 0 }
}


#
# move items around...
# put item b after a horizontally, with some padding
#
proc itemXSequence {c a b {pad 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        lassign [$c bbox {*}$b] bx1 by1 bx2 by2
        set dx [expr $ax2+$pad-$bx1]
        foreach t $b { $c move $t $dx 0 }
}

#
# move items around...
# put item b under item a with some padding
#
proc itemYSequence {c a b {pad 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        lassign [$c bbox {*}$b] bx1 by1 bx2 by2
        set dy [expr $ay2+$pad-$by1]
        foreach t $b { $c move $t 0 $dy }
}


#
# return the bbox of the set of tags a, plus a margin
proc itemCover {c a {margin 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        return [list [expr $ax1-$margin] [expr $ay1-$margin] [expr $ax2+$margin] [expr $ay2+$margin]]
}

proc itemGetCenter {c a {margin 0}} {
        lassign [$c bbox {*}$a] ax1 ay1 ax2 ay2
        return [list [expr ($ax1+$ax2)/2] [expr ($ay1+$ay2)/2]]
}



