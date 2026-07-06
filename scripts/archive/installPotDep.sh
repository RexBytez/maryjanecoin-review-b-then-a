#!/bin/bash
cd /home/$USER/
echo "###############################"
echo "#         Dependencies        #"		
echo "###############################"
echo ""
sudo apt-get update
sudo apt-get install git
sudo apt-get install build-essential libssl-dev libdb++-dev libboost-all-dev libqrencode-dev libminiupnpc-dev libevent-dev
sudo apt-get install qt5-default qt5-qmake qtbase5-dev-tools qttools5-dev-tools build-essential libboost-dev 
sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-program-options-dev libboost-thread-dev libssl-dev
echo ""
echo "###############################"
echo "#          Compiling          #"		
echo "###############################"
echo ""
echo "Now we begin the compilation....."
echo ""
git clone https://github.com/Jahvinci/MaryJaneCoin.git
echo "\nRepository downloaded."
cd /home/$USER/MaryJaneCoin/src/leveldb/
sh build_detect_platform build_config.mk ./
cd /home/$USER/MaryJaneCoin/src/
echo "\nMakefile.."
make -f makefile.unix
cd /home/$USER/MaryJaneCoin/
echo "Qmake..."
qmake
echo "Make.."
make
echo "Your wallet is ready."

