# -*- mode: ruby -*-
# vi: set ft=ruby :

# Freeciv-web Vagrant Vagrantfile - play.freeciv.org 
# 2014-02-17 - Andreas Røsdal
#
# Run 'vagrant up' in this directory, which will create a VirtualBox image,
# and run scripts/freeciv-web-bootstrap.sh to install Freeciv-web for you.
# Then point your browser to http://localhost/ on your host OS.
#

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "saucy64"

  # The url from where the 'config.vm.box' box will be fetched if it
  # doesn't already exist on the user's system.
  config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/saucy/current/saucy-server-cloudimg-amd64-vagrant-disk1.box"

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # on Windows accessing "localhost:80" will access port 80 on the guest 
  # machine, and on Linux and OS X accessing "localhost:8080" will access 
  # port 80 on the guest machine. Se README for creating a SSH tunnel for
  # Linux and OS X on port 80.
  if Vagrant::Util::Platform.windows?
    config.vm.network :forwarded_port, guest: 80, host: 80
  else 
    config.vm.network :forwarded_port, guest: 80, host: 8080
  end

  # WebSocket connections use ports 1337 - 1340
  config.vm.network :forwarded_port, guest: 1337, host: 1337
  config.vm.network :forwarded_port, guest: 1338, host: 1338
  config.vm.network :forwarded_port, guest: 1339, host: 1339
  config.vm.network :forwarded_port, guest: 1340, host: 1340

 
   config.vm.provider "virtualbox" do |v|
    v.memory = 4000
   end

  # run the Freeciv bootstrap script on startup
  config.vm.provision :shell, :path => "scripts/freeciv-web-vagrant.sh"


end
