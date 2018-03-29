#!/usr/bin/wish

source osc.tcl

::osc::initIn 10000
::osc::initOut blub localhost 12345
::osc::debug 1

button .b -text go -command {::osc::send blub /allo ii 1 2}
ttk::scale .s1 -orient horizontal -command {::osc::send blub /pos f } -from 0 -to 100
button .q -text quit -command {::osc::uninitOut blub;destroy .}

#pack .b .s1 .q

#::osc::uninitOut first;


proc mylistbox {base} {
	tk::listbox $base.lb -yscrollcommand "$base.sb set" -height 5
	ttk::scrollbar $base.sb -command "$base.lb yview" -orient vertical
	ttk::label $base.stat -text "Status message here" -anchor w
	ttk::sizegrip $base.sz

	grid $base.lb -column 0 -row 0 -sticky nwes
	grid $base.sb -column 1 -row 0 -sticky ns
	grid $base.stat -column 0 -row 1 -sticky we
	grid $base.sz -column 1 -row 1 -sticky se

	grid columnconfigure $base 0 -weight 1;
	grid rowconfigure $base 0 -weight 1

	for {set i 0} {$i<100} {incr i} {
	   $base.lb insert end "Line $i of 100"
	}
}

button .b1 -text "un bouton"
ttk::panedwindow .p -orient horizontal
frame .p.f
frame .p.g
mylistbox .p.f
mylistbox .p.g
#ttk::treeview .p.tv -columns info
ttk::treeview .p.tv -columns { info ui }
.p.tv column info -width 50 -anchor e
.p.tv column ui -width 50 -anchor e

ttk::sizegrip .sz
.p add .p.f
.p add .p.g
.p add .p.tv
grid .p -column 0 -row 0  -sticky nwes
grid .b1 -column 0 -row 1  -columnspan 2 -sticky nwes
grid .sz -column 999 -row 999  -sticky ws
grid columnconfigure  . 0 -weight 1
grid rowconfigure  . 0 -weight 1



.p.tv insert {} end -id ecrans -text "Ecrans"
.p.tv insert ecrans end -id projui -text "ajoute projecteur" -tags ppui
.p.tv insert ecrans end -id proj-1 -text "projecteur 1" -tags pp1 -values { {} blub }
.p.tv insert ecrans end -id proj-2 -text "projecteur 2"
.p.tv insert ecrans end -id proj-3 -text "projecteur 3"
.p.tv insert proj-1 end -id proj-1-w -tags proj-1-w -text "Largeur"
.p.tv insert proj-1 end -id proj-1-h -text "Hauteur" -values { 1080 blub }
.p.tv insert proj-1 end -id proj-1-ox -text "Offset X" -values 1920
.p.tv insert proj-1 end -id proj-1-oy -text "Offset Y" -values 0
.p.tv insert {} end -text "Go!" -tags yo
.p.tv tag configure yo -background yellow
.p.tv tag bind yo <1> {puts "click on [.p.tv focus]"}
.p.tv tag bind yo <<TreeviewSelect>> { puts "select" }
.p.tv tag bind yo  <<TreeviewOpen>> { puts "open" }
.p.tv tag bind yo <<TreeviewClose>> { puts "close" }


.p.tv tag bind proj-1-w <KeyRelease> { puts "KEY %K!" }

.p.tv tag configure ppui -background yellow
.p.tv tag bind ppui <1> {puts "ajoute projecteur"}

puts [.p.tv tag configure yo]
puts [.p.tv tag names]
.p.tv item ecrans -open true

.p.tv set proj-1-w info 1920


.p.tv insert proj-1 end -id proj-1-A -text "Quad" -values A
.p.tv insert proj-1-A end -id proj-1-A-p1 -text "Point 1" -values {340,20}
.p.tv insert proj-1-A end -id proj-1-A-p2 -text "Point 2" -values {240,120}
.p.tv insert proj-1-A end -id proj-1-A-p3 -text "Point 3" -values {40,220}
.p.tv insert proj-1-A end -id proj-1-A-p4 -text "Point 4" -values {140,50}
.p.tv tag configure pp1 -background red


#first
#second
#blub
#puts [ttk::style theme names]
#puts [ttk::style theme use]
#wm withdraw .


######################################


