#!/usr/bin/wish


#
# args: {type field vals val}
# types: t, s f
# {t noma a}
# {s nomc {aa bb cc dd ee ff} cc}
# {f nomd {}}
#
# set array gloabl form($t:$field)
# appelle validate (si recoit true, ok sinon erreur)
# appelle callback sur ok
#


# {t name val} ...
proc makeForm {t title validate callback args} {
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
				$t.ent$field config -menu .blub -textvariable form($t:$field)
				set form($t:$field) $val
				menu .blub -tearoff 0
				.blub delete 0 end
				foreach j [lsort $vals] {
					.blub add command -label $j -command "set form($t:$field) $j"
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
        button $t.exi -text OK -command "finish $t $callback"
        grid $t.upd $t.exi -sticky news

}

proc finish {t callback} {
	puts "finish t=$t cb=$callback"
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

proc valide {} {
	global form
	puts "valide"
	if { $form(.toto:nomd)=={} } {
		puts "empty file"
		tk_messageBox -message "I know you like this application!" -type ok
		return false
	}
	return true;
}


proc fini {} {
	global form
	
	if { $form(.toto:nomd)=={} } {
		puts "empty file"
		tk_messageBox -message "I know you like this application!" -type ok
		return false
	}
	puts "got noma $form(.toto:noma)"
	puts "got nomb $form(.toto:nomb)"
	puts "got nomc $form(.toto:nomc)"
	puts "got nomd $form(.toto:nomd)"
	return true
}


makeForm .toto "un titre" valide fini {t noma a} {t nomb b} {s nomc {aa bb cc dd ee ff} cc} {f nomd {}}


wm iconify .



