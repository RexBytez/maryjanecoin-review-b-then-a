#!/bin/bash
cd /home/$USER/
echo "###############################"
echo "#         Dependencies        #"		
echo "###############################"
echo ""
sudo apt-get update
sudo apt-get install git
sudo apt-get install libjansson-dev libssl-dev automake libcurl4-openssl-dev
echo ""
echo "###############################"
echo "#          Compiling          #"		
echo "###############################"
echo ""
echo "Now we begin the compilation....."
echo ""
git clone https://github.com/tpruvot/Cpuminer-multi.git
echo "Repository downloaded."
cd /home/$USER/Cpuminer-multi/
./build.sh
./cpuminer -a x13 -o 127.0.0.1:17421 -u rpcuser -p password1




