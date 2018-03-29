#!/usr/bin/wish

source osc.tcl

cd ~/mapperProjects

#
# ajuste les fonts a la taille de l'ecran
#
# (ne fonctionne pas avec tree)
#foreach font [font names] { font configure $font {*}[font actual $font] }

#source osc.tcl

# la musique...
::osc::initOut pdextended 127.0.0.1 9990

::osc::initIn 10000
#::osc::initOut mapper sugo3.local 12345
::osc::initOut mapper 127.0.0.1 12345
::osc::debug 0

#::osc::send mapper /map/allo ii 1 2


#
# Projecteurs  |  Sources |  PreSets
#  proj-  |    src-  |    mat-    |  set-
#

button .q -text quit -command { ::osc::send mapper /map/quit; after 500 destroy . }
ttk::sizegrip .sz

wm geometry . 1200x600
#pack .b .s1 .q


ttk::panedwindow .p -orient horizontal


proc validateProjecteur {} {
	global form
	return true;
}
proc ajouteProjecteur {} {
	puts "ajoute projecteur"
	global form

	set d {}
	foreach i {Nom Largeur Hauteur {Offset X} {Offset Y}} {
		dict set d $i $form(.proj:$i)
	}
	projecteur {*}$d
	return true
}

proc elimineProjecteur {} {
	global projsWindow
	set t $projsWindow.tree
	puts "elimine projecteur [$t selection]"
	# on conserve seulement les tags qui on projs comme parent
	set d {}
	foreach i [$t selection] {
		if { [$t parent $i]=="projs" } {
			puts "removing $i"
			lappend d $i
		}
	}

	foreach dd $d {
		::osc::send mapper /map/elimine/projecteur s [lindex [$t item $dd -values] 0]
	}

	$t delete $d
}

proc validateProjQuad {} {
	return true;
}
proc ajouteProjQuad {} {
	global form projsWindow quadCount
	set t $projsWindow.tree
	puts "ajoute proj quad [$t focus]"
	# le parent doit etre projs
	set id [$t focus]
	while { $id!="" && $id!="projs" && [$t parent $id]!="projs" } { set id [$t parent $id] }
	if { $id!="" && $id!="projs" } {
		# proj-2-q-1 
		quad [$t item $id-n -value] $form(.proj:Nom) {0 0} {1 0} {1 1} {0 1} $form(.proj:Shader)
	}
	return true
}

proc elimineProjQuad {} {
	puts "elimine quad"
}

##
##
##

proc activatePresetInterface {} {
	global presetsWindow
	set t $presetsWindow.tree
	# demarrer la musique...
	#::osc::send pdextended /start
	puts "activate preset: selection=[$t selection]"
	foreach i [$t selection] {
		if { [$t parent $i]=="presets" } {
			set prnom [lindex [$t item $i -values] 0]
			puts "activate preset: $prnom"
			::osc::send mapper /map/preset/activate s $prnom
		}
	}
}
# utile dans les scripts
proc activatePreset {nomPreset} {
	::osc::send mapper /map/preset/activate s $nomPreset
}

proc deactivatePresetInterface {} {
	global presetsWindow
	set t $presetsWindow.tree
	# arreter la musique...
	#::osc::send pdextended /stop
	puts "deactivate preset: [$t selection]"
	foreach i [$t selection] {
		if { [$t parent $i]=="presets" } {
			set prnom [lindex [$t item $i -values] 0]
			puts "deactivate preset: $prnom"
			::osc::send mapper /map/preset/deactivate s $prnom
		}
	}
}
# utile dans les scripts
proc deactivatePreset {nomPreset} {
	::osc::send mapper /map/preset/deactivate s $nomPreset
}

#
#
#

proc ajouteSource {} {
	puts "ajoute source"
	global form

	set d {}
	foreach i {Nom Fichier} {
		dict set d $i $form(.src:$i)
	}
	src {*}$d
	return true
}

proc elimineSource {} {
	global sourcesWindow
	set t $sourcesWindow.tree
	puts "elimine source [$t selection]"
	# on conserve seulement les tags qui on projs comme parent
	set d {}
	foreach i [$t selection] {
		if { [$t parent $i]=="sources" } {
			puts "removing $i"
			lappend d $i
		}
	}
	foreach dd $d {
		::osc::send mapper /map/elimine/source s [lindex [$t item $dd -values] 0]
	}
	$t delete $d
}

proc hidecontrol {val} {
	global sourcesWindow
	set t $sourcesWindow.tree
	set p [$t selection]
	set wname [lindex [$t item $p -values] 0]
	puts "hide $wname = $val"
	::osc::send mapper /display/hide/$wname $val
}

proc videocontrolPause {val} {
	global sourcesWindow
	set t $sourcesWindow.tree
	set p [$t selection]
	set wname [lindex [$t item $p -values] 0]
	eval {::osc::send mapper /ffmpeg-recycle-$wname/pause $val}
	if { $val=="F" } {
		# flush before restart
		eval {::osc::send mapper /drip-in-$wname/flush}
	}
	eval {::osc::send mapper /drip-in-$wname/pausenow $val}
}
proc videocontrolRewind {} {
	global sourcesWindow
	set t $sourcesWindow.tree
	set p [$t selection]
	set wname [lindex [$t item $p -values] 0]
	eval {::osc::send mapper /ffmpeg-recycle-$wname/rewind}
}


proc ajouteSourceQuad {} {
	global form sourcesWindow quadCount
	set t $sourcesWindow.tree
	puts "ajoute source quad [$t focus]"
	# le parent doit etre sources
	set id [$t focus]
	while { $id!="" && $id!="sources" && [$t parent $id]!="sources" } { set id [$t parent $id] }
	if { $id!="" && $id!="sources" } {
		# source-2-q-1 
		# pas de shader!
		quad [$t item $id-n -value] $form(.src:Nom) {0 0} {1 0} {1 1} {0 1}
	}
	return true
}

#
# Prests
#

proc ajoutePreset {} {
	puts "ajoute preset"
	global form

	set d {}
	foreach i {Nom} {
		dict set d $i $form(.preset:$i)
	}
	preset {*}$d
	return true
}

proc defaultfont {} {
    set f [[button ._] cget -font]
    destroy ._
    return $f
}


proc ajouteConnexion {} {
	global presetsWindow quadCount
	global menuprojquads menusourcequads
	set t $presetsWindow.tree
	set id [$t focus]
	if { [$t parent $id]!="presets" } {
		puts "choisir un preset"
		return
	}

	connect [$t item $id-n -values] $menuprojquads $menusourcequads
}



#
#
#

proc makeProjs {} {
	global projCount projsWindow shaders
	set projCount 0

	set w $projsWindow
	set t $projsWindow.tree

	frame $w
	frame $w.bp
	button $w.bp.b1 -text "+ Proj" -bg yellow
	button $w.bp.b2 -text "- Proj" -bg pink
	frame $w.bq
	button $w.bq.b1 -text "+ Quad" -bg yellow
	button $w.bq.b2 -text "- Quad" -bg pink

	ttk::treeview $t -columns info

	pack $w.bp.b1 $w.bp.b2 -side left -fill both -expand 1
	pack $w.bq.b1 $w.bq.b2 -side left -fill both -expand 1

	pack $t -fill both -expand 1
	pack $w.bp -fill both
	pack $w.bq -fill both

	$t column info -width 90 -anchor e

	$t insert {} end -id projs -text "Projecteurs"
	$t item projs -open true

	#$t insert projs end -id projui -text "+ projecteur" -tags {ui ajouteproj}
	$w.bp.b1 config -command {
		makeForm .proj "Nouveau projecteur" ajouteProjecteur {t Nom P1} {t Largeur 1280} {t Hauteur 720} {t "Offset X" 1920} {t "Offset Y" 0}
	}
	$w.bp.b2 config -command elimineProjecteur

	$w.bq.b1 config -command [list makeForm .proj "Nouveau Quad" ajouteProjQuad {t Nom "Boite 1A"} [list s Shader [dict keys $shaders]]]
	$w.bq.b2 config -command elimineProjQuad


	$t tag configure editable -background lightgrey
	$t tag bind editable <KeyRelease> {puts "edit %W with [%W focus]"}
}

proc makeSources {} {
	global sourceCount sourcesWindow
	set sourceCount 0

	set w $sourcesWindow
	set t $sourcesWindow.tree

	frame $w

	frame $w.bh
	button $w.bh.hide -text "hide" -bg orange
	button $w.bh.show -text "show" -bg orange

	frame $w.bv
	button $w.bv.play -text "play" -bg orange
	button $w.bv.stop -text "stop" -bg orange
	button $w.bv.rewind -text "rewind" -bg orange

	frame $w.bp
	button $w.bp.b1 -text "+ Source" -bg yellow
	button $w.bp.b2 -text "- Source" -bg pink
	frame $w.bq
	button $w.bq.b1 -text "+ Quad" -bg yellow
	button $w.bq.b2 -text "- Quad" -bg pink

	ttk::treeview $t -columns info

	pack $w.bh.hide $w.bh.show -side left -fill both -expand 1
	pack $w.bv.play $w.bv.stop $w.bv.rewind -side left -fill both -expand 1
	pack $w.bp.b1 $w.bp.b2 -side left -fill both -expand 1
	pack $w.bq.b1 $w.bq.b2 -side left -fill both -expand 1

	pack $t -fill both -expand 1
	pack $w.bh -fill both
	pack $w.bv -fill both
	pack $w.bp -fill both
	pack $w.bq -fill both

	$t column info -width 90 -anchor e

	$t insert {} end -id sources -text "Sources"
	$t item sources -open true

	$w.bp.b1 config -command {
		makeForm .src "Nouvelle source" ajouteSource {t Nom S1} {f Fichier toto.mp4}
	}
	$w.bp.b2 config -command elimineSource

	$w.bv.play config -command {videocontrolPause F}
	$w.bv.stop config -command {videocontrolPause T}
	$w.bv.rewind config -command {videocontrolRewind}

	$w.bh.hide config -command {hidecontrol T}
	$w.bh.show config -command {hidecontrol F}

	$w.bq.b1 config -command {
		makeForm .src "Nouveau Quad" ajouteSourceQuad {t Nom "Boite 1A"}
	}
	$w.bq.b2 config -command elimineSourceQuad


	$t tag configure editable -background lightgrey
	$t tag bind editable <KeyRelease> {puts "edit %W with [%W focus]"}
}


proc makePresets {} {
	global presetCount presetsWindow
	set presetCount 0

	set w $presetsWindow
	set t $presetsWindow.tree

	frame $w
	frame $w.ba
	button $w.ba.b1 -text "Activate" -bg {light green}
	button $w.ba.b2 -text "Deactivate" -bg {light blue}
	frame $w.bp
	button $w.bp.b1 -text "+ Preset" -bg yellow
	button $w.bp.b2 -text "- Preset" -bg pink -state disabled
	frame $w.bq
	button $w.bq.b1 -text "+ Connect" -bg yellow
	ttk::menubutton $w.bq.b2
	ttk::menubutton $w.bq.b3
	frame $w.bqq
	button $w.bqq.b1 -text "- Connect" -bg pink -state disabled

	ttk::treeview $t -columns info

	pack $w.ba.b1 $w.ba.b2 -side left -fill both -expand 1
	pack $w.bp.b1 $w.bp.b2 -side left -fill both -expand 1
	pack $w.bq.b1 $w.bq.b2 $w.bq.b3 -side left -fill both -expand 1
	pack $w.bqq.b1 -side left -fill both -expand 1

	pack $t -fill both -expand 1
	pack $w.ba -fill both
	pack $w.bp -fill both
	pack $w.bq -fill both
	pack $w.bqq -fill both


	$t column info -width 90 -anchor e

	$t insert {} end -id presets -text "Presets"
	$t item presets -open true

	$w.bp.b1 config -command {
		makeForm .preset "Nouveau preset" ajoutePreset {t Nom R1}
	}
	$w.bp.b2 config -command elimineSet

	$w.ba.b1 config -command activatePresetInterface
	$w.ba.b2 config -command deactivatePresetInterface

	$w.bq.b1 config -command ajouteConnexion

	# menu des projecteurs et des sources
	menu .projquads -tearoff 1
	menu .sourcequads

	$w.bq.b2 config -menu .projquads -textvariable menuprojquads
	$w.bq.b3 config -menu .sourcequads -textvariable menusourcequads
	$w.bqq.b1 config -command elimineConnexion

	set menuprojquad "(projecteur quad)"
	set menusourcequads "(source quad)"


	$t tag configure editable -background lightgrey
	$t tag bind editable <KeyRelease> {puts "edit %W with [%W focus]"}
}




#
# trouve les quads des proj/sources et fabrique les menus
#
proc updateMenusQuad {} {
	#
	# projs
	#
	global projsWindow shaders
	set t $projsWindow.tree

	.projquads delete 0 end
	set projs [$t children projs]
	set labs {}
	foreach p $projs {
		set pnom [lindex [$t item $p-n -values] 0]
		set allq [lsearch -all -inline [$t children $p] "$p-q-*"]
		puts "les quads de $p sont $allq"
		foreach q $allq {
			#set val [list [lindex [$t item $q -values] 0]]
			#lappend labs $val
			set nom [lindex [$t item $q-n -values] 0]
			set shader [lindex [$t item $q-shader -values] 0]
			puts "le nom de $q est $q-n est $nom"
			puts "le shader de $q est $q-shader est $shader"
			#set nom [lindex [$t item $q-shader -values] 0]
			foreach {e v} [dict get $shaders $shader] {
				puts "uniforme $e -- $v"
				if { $v!="texture" } continue
				set nome "$pnom:$nom:$e"
				.projquads add command -label $nome -command [list set menuprojquads $nome]
			}
		}
	}

	#
	# sources
	#
	global sourcesWindow
	set t $sourcesWindow.tree

	.sourcequads delete 0 end
	set sources [$t children sources]
	foreach p $sources {
		set allq [lsearch -all -inline [$t children $p] "$p-q-*"]
		puts "les quads de $p sont $allq"
		set nom [lindex [$t item $p -values] 0]
		foreach q $allq {
			set val [list [lindex [$t item $q -values] 0]]
			set b "$nom:[lindex $val 0]"
			.sourcequads add command -label $b -command [list set menusourcequads $b]
		}
	}
}

# trouve un item avec le texte txt et retourne la valeur
proc findItem {t txt q} {
	set [$t item $q -text]
}


#
# Si le nom est Fichier, on utilise un dialogue de selection de fichier
#
#proc makeForm {t title fields values callback} {
#	puts "--- makeform $title ---"
#	catch {destroy $t}
#	toplevel $t
#	wm title $t $title
#	global form
#	for {set i 0} {$i<[llength $fields]} {incr i} {
#		set form([lindex $fields $i]) [lindex $values $i]
#	}
#	
#	foreach field $fields {
#		label $t.lab$field -text $field
#		if { $field!="FichierXXX" } {
#			entry $t.ent$field -textvariable form($field)
#		} else {
#			button $t.ent$field -text "choisir un fichier" -command [list askFile Fichier $t.ent$field]
#		}
#		grid $t.lab$field $t.ent$field -sticky news
#		bind $t.ent$field <Return> $callback
#		}
#	button $t.upd -text Cancel -command "destroy $t"
#	button $t.exi -text OK -command "$callback; destroy $t"
#	grid $t.upd $t.exi -sticky news
#}



#
# args: {type field vals val}
# types: t, s f
# {t noma a}
# {s nomc {aa bb cc dd ee ff} cc}
# {f nomd {}}
#
# set array gloabl form($t:$field)
# appelle callback sur ok
#
# callback will be called, must return true to destroy form
#


proc makeForm {t title callback args} {
        global form
        puts "--- makeform $title ---"
        catch {destroy $t}
        toplevel $t
        wm title $t $title

        foreach f $args {
                lassign $f type field vals val
                puts "type $type, name $field, vals $vals, val $val"

                # t=text, s=select
                switch $type {
                        t {
                                label $t.lab$field -text $field
                                entry $t.ent$field -textvariable form($t:$field)
                                set form($t:$field) $vals
                        }
                        s {
                                label $t.lab$field -text $field
                                ttk::menubutton $t.ent$field
                                $t.ent$field config -menu $t.blub -textvariable form($t:$field)
                                set form($t:$field) $val
                                menu $t.blub -tearoff 0
                                $t.blub delete 0 end
                                foreach j [lsort $vals] {
                                        $t.blub add command -label $j -command "set form($t:$field) $j"
                                }
                        }
                        f {
                                button $t.lab$field -text $field -command [list askFile $t:$field]
                                entry $t.ent$field -textvariable form($t:$field)
                                set form($t:$field) $vals
                        }
                }
         
                grid $t.lab$field $t.ent$field -sticky news
        }
        
        button $t.upd -text Cancel -command "destroy $t"
        button $t.exi -text OK -command "finishForm $t $callback"
        grid $t.upd $t.exi -sticky news

}

proc finishForm {t callback} {
        if { [$callback] } { destroy $t }
}

proc askFile {v} {
        global form
        set types {
                  {{Video}       {.mp4}      }     
                  {{Images}       {.jpg .png}      }
                  {{All Files}        *             }
                  }
        set form($v) [tk_getOpenFile -filetypes $types]
}



# $quadCount(proj-1) -> nb de quads dans ce proj
# les tags des quad sont proj-1-q-1 ... et aussi source-1-q...
array set quadCount {}
array set quadToProj {}

set projsWindow .p.projs
set sourcesWindow .p.sources
set presetsWindow .p.presets

source shaders.smap

makeProjs
makeSources
makePresets


.p add .p.projs
.p add .p.sources
.p add .p.presets

grid .p -column 0 -row 0  -sticky nwes
grid .q -column 0 -row 1  -columnspan 4 -sticky nwes
grid .sz -column 999 -row 999  -sticky ws
grid columnconfigure  . 0 -weight 1
grid rowconfigure  . 0 -weight 1



proc makeMenu {} {
	menu .mbar
	. configure -menu .mbar

	menu .mbar.fl -tearoff 0
	.mbar.fl add command -label Open -command { menuOpen } -underline 0
	.mbar.fl add command -label Save -command { menuSave PSX } -underline 0
	.mbar.fl add command -label "Save Projecteurs" -command { menuSave P } -underline 0
	.mbar.fl add command -label "Save Sources" -command { menuSave S} -underline 0
	.mbar.fl add command -label "Save Presets" -command { menuSave X} -underline 0
	.mbar.fl add command -label Exit -command { menuExit } -underline 0

	# trouve tout ce qui s'appelle .smap
	menu .mbar.fs -tearoff 0
	foreach s [glob -nocomplain *.smap] {
		.mbar.fs add command -label $s -command  "openFile $s" -underline 0
	}

	# barre de menu
	.mbar add cascade -menu .mbar.fl -label File -underline 0
	.mbar add cascade -menu .mbar.fs -label Scripts -underline 0
	
}

proc menuOpen {} {
	puts "menu open"
	set types {
		  {{LightMapper}       {.lmap}      }
    		  {{All Files}        *             }
		  }
	set filename [tk_getOpenFile -filetypes $types]
	puts "file is $filename"
	openFile $filename
}

#
# Ouvre et execute tout ce qui est dans un fichier
#
proc openFile {fname} {
	if { [catch {set f [open $fname r]}] } {
		puts "unable to open file $fname"
		return
	}
	# get file, line by line, and execute
	set lineNumber 0
	while {[gets $f line] >= 0} { eval $line }
	close $f
}

proc menuSave {selection} {
	global projsWindow sourcesWindow presetsWindow
	puts "menu save"
	set types {
		  {{LightMapper}       {.lmap}      }
    		  {{All Files}        *             }
		  }
	set filename [tk_getSaveFile -filetypes $types]
	puts "file is $filename"

	set saveProj   [string match *P* $selection]
	set saveSrc    [string match *S* $selection]
	set savePreset [string match *X* $selection]

	set f [open $filename w]

	if { $saveProj } {
		puts $f "videProjecteurs"
	}
	if { $saveSrc } {
		puts $f "videSources"
	}
	if { $savePreset } {
		puts $f "videPresets"
	}

	#puts $f "# position de la fenetre principale"
	#puts $f "wm geometry . [wm geometry .]"

	#
	# save the projs...
	#

	if { $saveProj } {

	set t $projsWindow.tree
	set allp [lsearch -all -inline [$t children projs] "proj-*"]
	#puts "got $p"

	foreach p $allp {
		puts "projecteur $p"
		set allk [$t children $p]
		puts $allk
		set d {}
		foreach j $allk {
			puts "item $j : [$t item $j -text] : [$t item $j -value]"
			set n [$t item $j -text]
			set v [$t item $j -value]
			if { ![string match $p-q-* $j] } {
				dict set d $n [lindex $v 0]
			}
		}
		puts $d
		puts $f "projecteur $d"
		# les quads
		# quad nomProj nom x1 y1 x2 y2 x3 y3 x4 y4
		foreach j $allk {
			if { [string match $p-q-* $j] } {
				puts "quads $j"
				puts $f "quad [dict get $d Nom] [tmp $t $j-n] [tmp $t $j-p0] [tmp $t $j-p1] [tmp $t $j-p2] [tmp $t $j-p3] [tmp $t $j-shader]"
			}
		}

	}

	}

	#
	# save les source
	#

	if { $saveSrc } {

	set t $sourcesWindow.tree
	puts "all sources [$t children sources]"
	set allp [lsearch -all -inline [$t children sources] "source-*"]

	foreach p $allp {
		puts "source $p"
		set allk [$t children $p]
		puts $allk
		set d {}
		foreach j $allk {
			puts "item $j : [$t item $j -text] : [$t item $j -value]"
			set n [$t item $j -text]
			set v [$t item $j -value]
			if { ![string match $p-q-* $j] } {
				dict set d $n [lindex $v 0]
			}
		}
		puts $d
		puts $f "src $d"
		# les quads
		# quad nomProj nom x1 y1 x2 y2 x3 y3 x4 y4
		foreach j $allk {
			if { [string match $p-q-* $j] } {
				puts "quads $j"
				puts $f "quad [dict get $d Nom] [tmp $t $j-n] [tmp $t $j-p0] [tmp $t $j-p1] [tmp $t $j-p2] [tmp $t $j-p3]"
			}
		}

	}

	}

	#
	# save les presets
	#

	if { $savePreset } {

	set t $presetsWindow.tree
	puts "all presets [$t children presets]"
	set allp [lsearch -all -inline [$t children presets] "preset-*"]

	foreach p $allp {
		puts "preset $p"
		set allk [$t children $p]
		puts $allk
		set d {}
		foreach j $allk {
			puts "item $j : [$t item $j -text] : [$t item $j -value]"
			set n [$t item $j -text]
			set v [$t item $j -value]
			if { ![string match $p-c-* $j] } {
				dict set d $n [lindex $v 0]
			}
		}
		puts $d
		puts $f "preset $d"
		# les connexion
		# connect presetid from to

		foreach j $allk {
			if { [string match $p-c-* $j] } {
				puts "connexion $j"
				puts $f "connect [list [dict get $d Nom]] [list [$t item $j -text]] [tmp $t $j]"
			}
		}

	}

	}

	close $f
}

proc tmp {t id} {
	return [list [lindex [$t item $id -values] 0 ]]
}

proc menuExit {} {
	destroy .
}




makeMenu

#
# remove all projecteurs
#
proc videProjecteurs {} {
	global projCount projsWindow
	set projCount 0

	set allp [lsearch -all -inline [$projsWindow.tree children projs] "proj-*"]

	$projsWindow.tree delete $allp
}

#
# remove all sources
#
proc videSources {} {
	global sourceCount sourcesWindow
	set sourceCount 0

	set alls [lsearch -all -inline [$sourcesWindow.tree children sources] "src-*"]

	$sourcesWindow.tree delete $alls
}

#
# remove all presets
#
proc videPresets {} {
	global presetCount presetsWindow
	set presetCount 0

	set allp [lsearch -all -inline [$presetsWindow.tree children presets] "preset-*"]

	$presetsWindow.tree delete $allp
}


#
# reading from file
# projecteur nom P1 Largeur 1280 Hauteur 720 {Offset X} 1920 {Offset Y} 0
#
proc projecteur {args} {
	global projCount projsWindow

	puts "reading projecteur: $args"	
	set d $args
	incr projCount
	set id proj-$projCount
	set t $projsWindow.tree

	puts "keys are [dict keys $d]"
	puts "Nom is [dict get $d Nom]"
	$t insert projs end -id $id -text "projecteur $projCount" -tags proj -values [list [dict get $d Nom]]
	$t item $id -open true

	set ids [dict create Nom n Largeur w Hauteur h "Offset X" ox "Offset Y" oy]

	foreach {key iid} $ids {
		set val ""
		catch {set val [dict get $d $key]}
		$t insert $id end -id $id-$iid -text $key -tags editable -values [list $val]
	}

	::osc::send mapper /map/ajoute/projecteur siiii [dict get $d Nom] [dict get $d Largeur] [dict get $d Hauteur] [dict get $d {Offset X}] [dict get $d {Offset Y}]
}

#
# reading from file
# source nom S1 fichier toto.mp4
#
proc src {args} {
	global sourceCount sourcesWindow

	puts "reading source: $args"	
	set d $args
	incr sourceCount
	set id source-$sourceCount
	set t $sourcesWindow.tree

	puts "keys are [dict keys $d]"
	puts "Nom is [dict get $d Nom]"
	$t insert sources end -id $id -text "source $sourceCount" -tags source -values [list [dict get $d Nom]]
	#$t item $id -open true

	set ids [dict create Nom n Fichier f]

	foreach {key iid} $ids {
		set val ""
		catch {set val [dict get $d $key]}
		$t insert $id end -id $id-$iid -text $key -tags editable -values [list $val]
	}

	::osc::send mapper /map/ajoute/source ss [dict get $d Nom] [dict get $d Fichier]
	# cache la fenetre... elle sera invisible
	after 500 ::osc::send mapper /display/hide/[dict get $d Nom] T
}

#
# cherche dans les proejcteurs et les sources
#
proc quad {nomProjOuSrc nom xy1 xy2 xy3 xy4 {shader normal}} {
	global projsWindow sourcesWindow quadCount quadToProj
	puts "ajoute un quad au projecteur/source $nomProjOuSrc shader=$shader"

	# trouve le projecteur avec ce nom
	set t $projsWindow.tree
	foreach pid [$t children projs] {
		set n [$t item $pid-n -values]
		puts "got : $n"
		if { $n==$nomProjOuSrc } {
			puts "adding quad to proj $n"
			incr quadCount($pid)
			set lettre [format %c [expr $quadCount($pid)+65-1]]
			set qid $pid-q-$quadCount($pid)
			$t insert $pid end -id $qid -text "Quad $lettre" -values [list $nom]
			$t insert $qid end -id $qid-l -text Lettre -values [list $lettre]
			$t insert $qid end -id $qid-n -text Nom -values [list $nom]
			$t insert $qid end -id $qid-shader -text Shader -values [list $shader]
			$t insert $qid end -id $qid-p0 -text "Point 0" -values [list $xy1]
			$t insert $qid end -id $qid-p1 -text "Point 1" -values [list $xy2]
			$t insert $qid end -id $qid-p2 -text "Point 2" -values [list $xy3]
			$t insert $qid end -id $qid-p3 -text "Point 3" -values [list $xy4]
			eval ::osc::send mapper /map/projecteur/ajoute/quad scssffffffff {$nomProjOuSrc} {$lettre} {$nom} {$shader} $xy1 $xy2 $xy3 $xy4
			set quadToProj($nom) $nomProjOuSrc
		}
	}

	# trouve la source avec ce nom
	# on affiche pas le shader, qui devrait etre normal
	set t $sourcesWindow.tree
	foreach pid [$t children sources] {
		set n [$t item $pid-n -values]
		puts "got : $n"
		if { $n==$nomProjOuSrc } {
			puts "adding quad to source $n"
			incr quadCount($pid)
			set lettre [format %c [expr $quadCount($pid)+65-1]]
			set qid $pid-q-$quadCount($pid)
			$t insert $pid end -id $qid -text "Quad $lettre" -values [list $nom]
			$t insert $qid end -id $qid-l -text Lettre -values [list $lettre]
			$t insert $qid end -id $qid-n -text Nom -values [list $nom]
			$t insert $qid end -id $qid-p0 -text "Point 0" -values [list $xy1]
			$t insert $qid end -id $qid-p1 -text "Point 1" -values [list $xy2]
			$t insert $qid end -id $qid-p2 -text "Point 2" -values [list $xy3]
			$t insert $qid end -id $qid-p3 -text "Point 3" -values [list $xy4]
			eval ::osc::send mapper /map/source/ajoute/quad scsffffffff {$nomProjOuSrc} {$lettre} {$nom} $xy1 $xy2 $xy3 $xy4
		}
	}

	updateMenusQuad

}

proc preset {args} {
	global presetCount presetsWindow

	puts "reading preset: $args"	
	set d $args
	incr presetCount
	set id preset-$presetCount
	set t $presetsWindow.tree

	puts "keys are [dict keys $d]"
	puts "Nom is [dict get $d Nom]"
	$t insert presets end -id $id -text "preset $presetCount" -tags preset -values [list [dict get $d Nom]]
	#$t item $id -open true

	set ids [dict create Nom n]

	foreach {key iid} $ids {
		set val ""
		catch {set val [dict get $d $key]}
		$t insert $id end -id $id-$iid -text $key -tags editable -values [list $val]
	}

	::osc::send mapper /map/ajoute/preset s [dict get $d Nom]
}

proc connect {nom from to} {
	global presetsWindow quadCount quadToProj
	set t $presetsWindow.tree

	foreach id [$t children presets] {
		set n [lindex [$t item $id-n -values] 0]
		if {$n==$nom} {
			puts "ajoute connexion $id:  $from -> $to"
			if { [$t parent $id]!="presets" } {
				return
			}
			incr quadCount($id)
			set nid $id-c-$quadCount($id)
			$t insert $id end -id $nid -text $from -values [list $to]
			break
		}
	}
	# trouve le quad 'from' dans les projecteurs
	#set nomProj $quadToProj($from)
	lassign [split $from :] nomProj nomPQuad nomUni
	puts "je test le $from -> $nomProj, $nomPQuad, $nomUni"
	#  nom du preset / nom proj / nom du quad projecteur / nom uniform / nom source / nom du quad source
	lassign [split $to :] nomSource nomQuad
	::osc::send mapper /map/preset/ajoute/connexion ssssss $nom $nomProj $nomPQuad $nomUni $nomSource $nomQuad

	#
	# si on connecte proj1/A avec src1/ecran
	# on doit ajouter une matrix-cmd a src/ecran vers proj1/A et declancher (go src1/A proj1/A)
	# pour mettre a jour immediatement.
	# si on enleve une connexion, il faudra enleve la matrix-cmd du tag src1/A vers proj1/A
	# /set/matrix-cmd proj1-A /mat3/src1-A/homog
	#
}

#
# time management
#
proc start {} {
	global startTime
	set startTime [clock milliseconds]
}

#
# attends qu'on soit a la bonne heure depuis start
# le temps est 5  -> 5 secondes
#	5.500 -> 5 + 500 millis
#	1:10 -> i minute + 10 secondes
#	1:10.3 -> 1 min + 10 sec + 300 millis
#
proc at {abstime args} {
	global startTime
	lassign [lreverse [split $abstime ":"]] sec min
	if { $min=={} } { set min 0 }
	set t [expr ($min*60+$sec)*1000]
	set delta [expr int($t-([clock milliseconds]-$startTime))]
	puts "delta=$delta"
	if { $delta>0 } {
		after $delta $args
	} else {
		eval $args
	}
}

#
# len is number of steps
# si len =5, from=0, to=5, on va avoir 0 1 2 3 4 5
#
proc animate {from to len nbstep args} {
	global startTime
	#puts "animate $from $to $len $nbstep"
	eval $args f $from
	#set now [expr [clock milliseconds]-$startTime]
	incr nbstep -1
	if { $nbstep>0 } {
		set sleep [expr int($len*1000/$nbstep)]
		set len [expr $len-$sleep/1000.0]
		set from [expr $from+($to-$from+0.0)/$nbstep]
		after $sleep animate $from $to $len $nbstep $args
	}
}


#
# command line arguments
#
for {set i 0} {$i<[llength $argv]} {incr i} {
	switch [lindex $argv $i] {
		-h	{ puts {superMapper -- [-h] [-open toto.lmap]}; destroy . }
		-open	{ incr i;openFile [lindex $argv $i] }
		default { puts "unkonwn option: [lindex $argv $i]"; destroy . }
	}
}


##
#
# fonctions de retour du superMapper
#
##

proc /yo {tag} {
	puts "bonjour, yo, avec tag $tag"
}

# deplace les points d'un quad (proj OU src)
proc /set/quad/point {nomproj lettre nomquad p x y} {
	global projsWindow sourcesWindow
	puts "update-quad-point: $nomproj $lettre $nomquad $p $x $y"
	set qid $nomproj
	#
	# trouve le projecteur avec ce nom
	#
	set t $projsWindow.tree
	foreach pid [$t children projs] {
		set n [$t item $pid-n -values]
		if { $n==$nomproj } {
			set allq [lsearch -all -inline [$t children $pid] "$pid-q-*"]
			puts "les quads de $pid sont $allq"
			foreach q $allq {
				set nomq [lindex [$t item $q -values] 0]
				if { $nomq==$nomquad } {
					$t item $q-p$p -values [list [list $x $y]]
				}
			}
		}
	}
	#
	# trouve la source avec ce nom
	#
	set t $sourcesWindow.tree
	foreach pid [$t children sources] {
		set n [$t item $pid-n -values]
		if { $n==$nomproj } {
			set allq [lsearch -all -inline [$t children $pid] "$pid-q-*"]
			puts "les quads de $pid sont $allq"
			foreach q $allq {
				set nomq [lindex [$t item $q -values] 0]
				if { $nomq==$nomquad } {
					$t item $q-p$p -values [list [list $x $y]]
				}
			}
		}
	}
}


# pour cin6058
proc monfade {args} {
	puts "mon fade $args"
	eval ::osc::send mapper /display/set/P1/A/fade $args
	eval ::osc::send mapper /display/set/P1/B/fade $args
	eval ::osc::send mapper /display/set/P1/C/fade $args
	eval ::osc::send mapper /display/set/P1/D/fade $args
	eval ::osc::send mapper /display/set/P1/E/fade $args
	eval ::osc::send mapper /display/set/P1/F/fade $args
	eval ::osc::send mapper /display/set/P2/A/fade $args
	eval ::osc::send mapper /display/set/P2/B/fade $args
	eval ::osc::send mapper /display/set/P2/C/fade $args
	eval ::osc::send mapper /display/set/P2/D/fade $args
	eval ::osc::send mapper /display/set/P2/E/fade $args
	eval ::osc::send mapper /display/set/P2/F/fade $args
}


#source base.lmap
#source base.lmap


