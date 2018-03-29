# Imgv

## Introduction

Imgv is a multithreaded computer vision library.

## Before installing

There are a few dependencies that must be checked before compiling imgv.
Some dependencies are packages, some are actual code you must compile.
See the instructions for dependencies in the file other/README.

-> for glfw and oscpack, and for installing on Raspberry Pi
	1- Follow REAME file under "other" directory

-> for Prosilica plugin: 
	1- you need to download vimba from http://www.alliedvisiontec.com/us/products/software/vimba-sdk.html
	2- you extract your package and you get into VimbaC/DynamicLib/x86_64bit/
	3- you copy libVimbaC.so to /usr/local/lib
	4- you get into VimbaC/Include and you copy *.h files
	5- you get into /usr/local/include then you mkdir VimbaC and you get into it and you paste your .h files
	6- finally you get into AVTGigETL and you launch Install.sh

For a super quick regular install on Linux Mint (or equivalent), see QUICK_INSTALL_LINUX

## Compiling imgv

Imgv uses the [CMake](http://www.cmake.org/) build system.

From inside the main directory,

mkdir build
cd build
cmake ..
(then optionally "ccmake .." to tune options)

make
sudo make install

That's it!

To test, just execute:

playImage

The indian head should show up. Move the mouse over the image and press 1,2,3,4 to distort the image.

The documentation will be installed here:

file:///usr/local/share/imgv/docs/html/index.html

## Using imgv externaly

When using the imgv library, simply include the library with:

#include <imgv/imgv.hpp>
#include <imgv/qpmanager.hpp>   (if you are using qpmanager)

To link properly with the library, simply use this (on linux, adapt to your situation):

LIBS= -L/usr/local/lib -limgv -loscpack -lpthread -lopencv_objdetect -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_calib3d -lopencv_features2d -lpthread -lX11 -lXinerama -lX11 -lXinerama -ludev -lopencv_flann





Enjoy!
