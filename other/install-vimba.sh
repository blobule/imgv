#!/bin/bash
echo "The source for Vimba: http://www.alliedvisiontec.com/us/products/software/vimba-sdk.html"


if [[ `arch` = "x86_64" ]]; then
	echo "64-bit installation"
	bits=64
else
	echo "32-bit installation"
	bits=32
fi

echo "Architecture is `arch`, instalaling $bits bits"

#
# http://www.alliedvisiontec.com/us/products/software/vimba-sdk.html
#
if [[ ! -d ~/src/vimba ]]; then
	mkdir -p ~/src/vimba
fi
cd ~/src/vimba

#
# get vimba
#
if [[ ! -d Vimba_1_3 ]]; then
	if [[ ! -f AVTVimba_v1.3_Linux.tgz ]]; then
                wget http://www.alliedvision.com/fileadmin/content/software/software/Vimba/Vimba_v1.3_Linux.tgz
		#wget http://www.alliedvisiontec.com/fileadmin/content/PDF/Software/AVT_software/zip_files/VIMBA/AVTVimba_v1.3_Linux.tgz
	fi
	tar xvfz Vimba_v1.3_Linux.tgz
	#tar xvfz AVTVimba_v1.3_Linux.tgz
fi

cd Vimba_1_3


sudo cp VimbaCPP/DynamicLib/x86_${bits}bit/libVimbaC.so /usr/local/lib/
sudo cp VimbaCPP/DynamicLib/x86_${bits}bit/libVimbaCPP.so /usr/local/lib/
sudo cp AVTImageTransform/DynamicLib/x86_${bits}bit/libAVTImageTransform.so /usr/local/lib


sudo mkdir -p /usr/local/include/VimbaC/Include
sudo cp VimbaC/Include/* /usr/local/include/VimbaC/Include

sudo mkdir -p /usr/local/include/VimbaCPP/Include
sudo cp VimbaCPP/Include/* /usr/local/include/VimbaCPP/Include

sudo mkdir -p /usr/local/include/AVTImageTransform/Include
sudo cp AVTImageTransform/Include/* /usr/local/include/AVTImageTransform/Include

sudo cp Tools/Viewer/Bin/x86_${bits}bit/VimbaViewer /usr/local/bin

#
# Update .bashrc
#

echo "" >>${HOME}/.bashrc
echo "#" >>${HOME}/.bashrc
echo "# Vimba" >>${HOME}/.bashrc
echo "#" >>${HOME}/.bashrc

echo "export GENICAM_GENTL64_PATH=:${HOME}/src/vimba/Vimba_1_3/AVTGigETL/CTI/x86_64bit" >> ${HOME}/.bashrc
echo "export GENICAM_GENTL32_PATH=:${HOME}/src/vimba/Vimba_1_3/AVTGigETL/CTI/x86_32bit" >> ${HOME}/.bashrc

echo "export LD_LIBRARY_PATH=\${LD_LIBRARY_PATH}:/usr/local/lib" >> ${HOME}/.bashrc
echo "export PATH=\${PATH}:/usr/local/bin" >> ${HOME}/.bashrc
echo "" >>${HOME}/.bashrc
echo "" >>${HOME}/.bashrc

echo "Demarrer un nouveau terminal pour activer la configuration."


