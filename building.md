git clone --recurse-submodules

Linux

GLFW 
https://www.glfw.org/docs/latest/compile_guide.html
sudo apt install xorg-dev

The project requires c++23 support so you need to install a lest gcc 12
sudo apt install gcc-12
sudo apt install g++-12

And then update the simlink to the default compiler
https://askubuntu.com/questions/26498/how-to-choose-the-default-gcc-and-g-version

rm /usr/bin/gcc
ln -s /usr/bin/gcc-12 /usr/bin/gcc
rm /usr/bin/g++
ln -s /usr/bin/g++-12 /usr/bin/g++

yojimbo
https://github.com/networkprotocol/yojimbo/blob/master/BUILDING.md
sudo apt install libsodium-dev
sudo apt install libmbedtls-dev
