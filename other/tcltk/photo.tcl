#!/usr/bin/wish8.6


image create photo toto -file "linux.png"

label .a -image toto
pack .a

toto put {{#ff0000 #ff0000 #ff0000} {#00ff00 #00ffff #ff0000}} -to 50 50 52 51

#puts -nonewline [toto data -format png]

puts [toto get 100 100]


