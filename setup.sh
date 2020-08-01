git clone https://github.com/ronistone/Kernel_Paxos.git
cd Kernel_Paxos/
git checkout feature-persistence-workspace 
sudo apt update
sudo apt install make lmdb-utils gcc liblmdb-dev -y
make
