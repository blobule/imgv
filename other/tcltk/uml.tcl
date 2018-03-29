#!/usr/bin/wish8.6

proc defaultfont {} {
    set f [[button ._] cget -font]
    destroy ._
    set f
 }
 proc ladd {_list what} {
    upvar $_list list
    if {![info exists list] || [lsearch $list $what] == -1} {
        lappend list $what
    }
 }
 namespace eval UML {
    variable a
    #set a(font)     [defaultfont]
    #set a(boldfont) [concat $a(font) bold]
    #  the following two are suggestions as more platform independent
    set a(font)     [eval font create fontNormal [font actual [defaultfont]]]
    set a(boldfont) [eval font create fontBold   [font actual [defaultfont]] -weight bold]

    proc box {c x y class {fill white}} {
        variable a
        foreach {name atts meths} $class break
        if {"$atts$meths"==""} {append name \n} ;# don't make box too low
        set id0 [$c create text $x $y -text $name -anchor nw -font $a(boldfont)]
        $c itemconfig $id0 -tag [set id node$id0]
        set y1 [lindex [$c bbox $id] 3]
        if {$atts!=""} {
            $c create text $x $y1 -text [join $atts \n] -anchor nw\
                -font $a(font) -tag $id
        }
        set y2 [lindex [$c bbox $id] 3]
        if {$meths!=""} {
            $c create text $x $y2 -text [join $meths ()\n]() -anchor nw\
                -font $a(font) -tag $id
        }
        foreach {x0 y0 x3 y3} [$c bbox $id] break
        set x0 [expr {$x0-2}]
        set x3 [expr {$x3+2}]
        if {$meths!=""} {set y3 [expr {$y3+2}]}
        set id1 [$c create rect $x0 $y0 $x3 $y3 -tag $id \
            -fill grey -outline grey]
        $c move $id1 4 4
        set id2 [$c create rect $x0 $y0 $x3 $y3 -tag $id -fill $fill]
        $c lower $id2; $c lower $id1
        $c create line $x0 $y1 $x3 $y1 -tag $id
        $c create line $x0 $y2 $x3 $y2 -tag $id
        foreach {x0 y0 x1 y1} [$c bbox $id] break
        $c move $id [expr {($x0-$x1)/2}] [expr {($y0-$y1)/2}]
        set a(edges,$id) {}

        # lines to add saving by jima
        set a(TyS,$id) box
        set a(NaS,$id) $name
        set a(ClS,$id) $class
        set a(FiS,$id) $fill
        set a(XS,$id) $x
        set a(YS,$id) $y
        return $id
    }
    proc oval {c x y text {fill white}} {
        variable a
        set id0 [$c create oval [expr {$x-20}] [expr {$y-20}] \
            [expr {$x+20}] $y -fill grey -outline grey]
        $c move $id0 4 4
        set id1 [$c create oval [expr {$x-20}] [expr {$y-20}] \
            [expr {$x+20}] $y -fill $fill]
        $c itemconfig $id1 -tag [set id node$id1]
        $c itemconfig $id0 -tag $id
        $c create text $x [expr {$y+5}] -text $text \
            -font $a(font) -tag $id -anchor n
        set a(edges,$id) {}
        return $id
    }
    proc actor {c x y {text ""}} {
        variable a
        set id0 [$c create oval [expr {$x-5}] [expr {$y-5}] \
            [expr {$x+5}] [expr {$y+5}] -fill white]
        $c itemconfig $id0 -tag [set id node$id0]
        $c create line [expr {$x-15}] [expr {$y+7}] \
            [expr {$x+15}] [expr {$y+7}] -tag $id
        $c create line $x [expr {$y+5}] $x [expr {$y+15}] -tag $id
        $c create line $x [expr {$y+15}] [expr {$x-10}] [expr {$y+25}] -tag $id
        $c create line $x [expr {$y+15}] [expr {$x+10}] [expr {$y+25}] -tag $id
        $c create text $x [expr {$y+25}] -text $text \
            -font $a(font) -tag $id -anchor n
        set a(edges,$id) {}
        return $id
    }
    proc rawEdge {c from to {dash ""}} {
        foreach {x0 y0 x2 y2} [$c bbox $from] break
        set x1 [expr {($x0+$x2)/2.}]
        set y1 [expr {($y0+$y2)/2.}]
        if {$from==$to} {
            set x3 [expr {$x2+5}]
            set y4 [set y3 [expr {$y2+5}]]
            set x4 $x2
            $c create line $x2 $y1 $x3 $y1 $x3 $y3 $x1 $y3 $x1 $y2 \
                -tag edge -dash $dash
        } else {
            foreach {x3 y3 x5 y5} [$c bbox $to] break
            set x4 [expr {($x3+$x5)/2.}]
            set y4 [expr {($y3+$y5)/2.}]
            if {$x1<$x2 && $x4>$x2} {set x1 $x2} ;# crop coordinates
            if {$x4>$x3 && $x1<$x3} {set x4 $x3}
            if {$y1<$y2 && $y4>$y2} {set y1 $y2}
            if {$y4>$y3 && $y1<$y3} {set y4 $y3}
            if {$x1>$x0 && $x4<$x0} {set x1 $x0}
            if {$x4<$x5 && $x1>$x5} {set x4 $x5}
            if {$y1>$y0 && $y4<$y0} {set y1 $y0}
            if {$y4<$y5 && $y1>$y5} {set y4 $y5}
            $c create line $x1 $y1 $x4 $y4 -tag edge -dash $dash
        }
        decorationPoints $x1 $y1 $x4 $y4
    }
    proc decorationPoints {x1 y1 x4 y4 {r 12}} {
        set a [expr {atan2($x4-$x1,$y4-$y1)}]
        set a1 [expr {$a-atan(1.)}]
        set x2 [expr {round($x4-cos($a1)*$r)}]
        set y2 [expr {round($y4+sin($a1)*$r)}]
        set a2 [expr {$a+atan(1.)}]
        set x3 [expr {round($x4+cos($a2)*$r)}]
        set y3 [expr {round($y4-sin($a2)*$r)}]
        set a3 [expr {$a+2*atan(1)}]
        set r2 [expr {$r*sqrt(2.)}]
        set x5 [expr {round($x4+cos($a3)*$r2)}]
        set y5 [expr {round($y4-sin($a3)*$r2)}]
        list $x2 $y2 $x4 $y4 $x3 $y3 $x5 $y5 ;# use 6 for a triangle
    }
    proc edge {c type from to} {
        variable a
        ladd a(edges) [list $type $from $to]
        set dash [expr {$type=="depend"||$type=="dotted"? ".": ""}]
        set deco [rawEdge $c $from $to $dash]
        switch -- $type {
          aggreg   {$c create poly $deco -fill white -outline black -tag edge}
          assoc    {}
          compose  {$c create poly $deco -fill black -outline black -tag edge}
          uniAssoc -
          depend   {$c create line [lrange $deco 0 5] -tag edge}
          dotted   {}
          inherit  {$c create poly [lrange $deco 0 5] -fill white \
            -outline black -tag edge}
        }
    }
    proc makeMovable c {
        variable a
        foreach i [$c find all] {
            if [regexp {node[1-9]} [$c itemcget $i -tags]] {
                $c addtag mv withtag $i
            }
        }
        $c bind mv <1> {
            set tag ""
            foreach i [%W itemcget current -tags] {
                if [regexp node $i] {set tag $i; break}
            }
            if {$tag!=""} {
                set ::UML::a(tag) $tag
                set ::UML::a(x) [%W canvasx %X]
                set ::UML::a(y) [%W canvasx %Y]
                #jima added lines.
                set ::UML::a(XS,$tag) [%W canvasx %X]
                set ::UML::a(YS,$tag) [%W canvasx %Y]
            }
        }
        $c bind mv <B1-Motion> {
            set x [%W canvasx %X]
            set y [%W canvasx %Y]
            %W move $::UML::a(tag) \
                [expr {$x-$::UML::a(x)}] [expr {$y-$::UML::a(y)}]
            set ::UML::a(x) $x
            set ::UML::a(y) $y
        }
        $c bind mv <B1-ButtonRelease> {
          %W delete withtag edge
          if {[array names ::UML::a -exact edges] != ""} {
            foreach i $::UML::a(edges) {
              eval UML::edge %W $i
            }
          }
        }

    }
 }

 proc save c {
   variable a
   set ThRes {}
   append ThRes "\n"
   append ThRes "#Setting canvas...\n"
   append ThRes "$c configure -width [$c cget -width] -height [$c cget -height]\n"
   append ThRes "\n"
   if {[array names ::UML::a -regexp {ClS,node[1-9]}] != ""} {
     foreach i [array names ::UML::a -regexp {NaS,node[1-9]}] {
       set ThNode [string range $i 4 end]
       set ThTy $UML::a(TyS,$ThNode)
       set ThNa $UML::a(NaS,$ThNode)
       set ThCl $UML::a(ClS,$ThNode)
       set ThFi $UML::a(FiS,$ThNode)
       set ThX $UML::a(XS,$ThNode)
       set ThY $UML::a(YS,$ThNode)

       append ThRes "\n"
       append ThRes "#Setting node ($ThTy) $ThNa\n"
       append ThRes "set $ThNa \[UML::$ThTy $ThX $ThY $ThCl $ThFi\]\n"
       append ThRes "\n"
     }
   }
   if {[array names ::UML::a -exact edges] != ""} {
     foreach i $::UML::a(edges) {
       set ThTy [lindex $i 0]
       set ThA $::UML::a(NaS,[lindex $i 1])
       set ThB $::UML::a(NaS,[lindex $i 2])
       append ThRes "\n"
       append ThRes "#Setting edge ($ThTy) $ThA $ThB\n"
       append ThRes "UML::edge $c $ThTy $ThA $ThB\n"
       append ThRes "\n"
     }
   }

   return $ThRes
 }

#------- self-test code, and demo
 if {[file tail [info script]]==[file tail $argv0]} {
   set c [canvas .c -bg white]
   pack .c -fill both -expand 1
   set b1 [UML::box   $c  50  50 {Class {att0 att1 att2} {method1 method2}}]
   set b2 [UML::box   $c 200  50 {AnotherClass {attrib} {method1 method2 method3}}]
   set o1 [UML::oval  $c 350  50 "Use a CASE tool"]
   set a1 [UML::actor $c 350 200 badActor]
   set b3 [UML::box   $c  50 200 {NoAttributes {} {butHas methods}} yellow]
   set b4 [UML::box   $c 200 200 {NoMethods {butHas attributes} {}}]
   set b5 [UML::box   $c 200 125 {{UML demo}} green]
   UML::edge $c aggreg   $b1 $b2
   UML::edge $c uniAssoc $b1 $b3
   UML::edge $c depend   $b1 $b4
   UML::edge $c dotted   $b4 $o1
   UML::edge $c assoc    $o1 $a1
   UML::edge $c assoc    $b2 $a1
   UML::edge $c inherit  $b3 $b4
   UML::edge $c compose  $b2 $b3
   UML::edge $c assoc    $b4 $b4
   UML::makeMovable $c
 }

