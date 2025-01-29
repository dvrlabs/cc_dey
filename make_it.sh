# installing the toolchain:
# (ConnectCore 6+ QP SBC)
# wget https://ftp1.digi.com/support/digiembeddedyocto/3.2/r3/sdk/ccimx6qpsbc/xwayland/dey-glibc-x86_64-dey-image-qt-xwayland-cortexa9t2hf-neon-ccimx6qpsbc-toolchain-3.2-r3.sh
# chmod +x dey-glibc-x86_64-dey-image-qt-xwayland-cortexa9t2hf-neon-ccimx6qpsbc-toolchain-3.2-r3.sh
# ./dey-glibc-x86_64-dey-image-qt-xwayland-cortexa9t2hf-neon-ccimx6qpsbc-toolchain-3.2-r3.sh



rm app/cloud-connector 
rm library/libcloudconnector.a
source /opt/dey/3.2-r3/ccimx6qpsbc/environment-setup-cortexa9t2hf-neon-dey-linux-gnueabi
make clean
make

