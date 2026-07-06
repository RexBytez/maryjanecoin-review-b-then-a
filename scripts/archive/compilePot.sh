#!/bin/bash
echo ""
echo ""
echo "###############################"
echo "#          Compiling          #"		
echo "###############################"
echo ""
echo "Now we begin the compilation....."
echo ""
cd /home/$USER/MaryJaneCoin/src/leveldb/
sh build_detect_platform build_config.mk ./
cd /home/$USER/MaryJaneCoin/src/
echo "Makefile.."
make -f makefile.unix
cd /home/$USER/MaryJaneCoin/
echo "Qmake..."
qmake
echo "Make.."
make
echo "Your wallet is ready."
