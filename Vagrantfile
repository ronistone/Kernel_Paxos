# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configureMAX_SIZE
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
#

VM_NAME = 'Kernel_Paxos'

Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.box = "centos/7"

  config.vm.synced_folder ".", "/vagrant", type: 'virtualbox'

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
#   config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
#   config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder "../data", "/vagrant_data"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  config.vm.provider "virtualbox" do |vb|
    # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  #
  #   # Customize the amount of memory on the VM:
#     vb.name = VM_NAME
    vb.memory = "2048"
  end

  config.vm.define "paxos_1" do |paxos_1|
       paxos_1.vm.hostname = "paxos1"
       paxos_1.vm.network "private_network", ip: "192.168.33.10"
  end
  config.vm.define "paxos_2" do |paxos_2|
       paxos_2.vm.hostname = "paxos2"
       paxos_2.vm.network "private_network", ip: "192.168.33.11"
  end
  config.vm.define "paxos_3" do |paxos_3|
       paxos_3.vm.hostname = "paxos3"
       paxos_3.vm.network "private_network", ip: "192.168.33.12"
  end
  #
  # View the documentation for the provider you are using for more
  # information on available options.
  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
   config.vm.provision "shell", inline: <<-SHELL
     sudo yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
     sudo yum -y group install "Development Tools"
     sudo yum -y install nano wget gcc bzip2 make kernel-devel kernel-headers dkms gflags-devel glog-devel lmdb-devel lmdb tree kexec-tools
     sudo sh -c 'mkdir /var/crash || echo -e "\n\nkernel.core_pattern = /var/crash/core.%t.%p\nkernel.panic=10\nkernel.unknown_nmi_panic=1" >> /etc/sysctl.conf '
   SHELL
end
