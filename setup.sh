git clone https://github.com/ronistone/Kernel_Paxos.git
cd Kernel_Paxos/
git checkout feature-persistence-workspace 
sudo apt update
sudo apt install make lmdb-utils gcc liblmdb-dev cmake build-essential -y
make

sudo apt install cmake build-essential -y
git clone https://bitbucket.org/sciascid/libpaxos.git
cd libpaxos/
mkdir build
cd build
wget https://github.com/libevent/libevent/releases/download/release-2.1.12-stable/libevent-2.1.12-stable.tar.gz
tar -xvzf libevent-2.1.12-stable.tar.gz
cd libevent-2.1.12-stable/
./configure --disable-openssl
make
sudo make install
cd ..
git clone https://github.com/msgpack/msgpack-c.git
cd msgpack-c/
git checkout c_master
cmake .
make
sudo make install

cd ..
cmake ..