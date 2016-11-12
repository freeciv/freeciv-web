# coding: iso-8859-1
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
  config.vm.box = "ubuntu-yakkety"

  # The url from where the 'config.vm.box' box will be fetched if it
  # doesn't already exist on the user's system.
  config.vm.box_url = "https://cloud-images.ubuntu.com/yakkety/current/yakkety-server-cloudimg-amd64-vagrant.box"

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # on Windows accessing "localhost:80" will access port 80 on the guest 
  # machine, and on Linux and OS X accessing "localhost:8080" will access 
  # port 80 on the guest machine. Se README for creating a SSH tunnel for
  # Linux and OS X on port 80.
  if Vagrant::Util::Platform.windows?
    config.vm.network :forwarded_port, guest: 80, host: 80
    config.vm.network :forwarded_port, guest: 443, host: 443
  else 
    config.vm.network :forwarded_port, guest: 80, host: 8080
  end

  config.vm.provider "virtualbox" do |v|
    v.cpus = "1"
    host = RbConfig::CONFIG['host_os']

    # Give VM 2/3 system memory 
    if host =~ /darwin/
      # sysctl returns Bytes and we need to convert to MB
      mem = `sysctl -n hw.memsize`.to_i / 1024
    elsif host =~ /linux/
      # meminfo shows KB and we need to convert to MB
      mem = `grep 'MemTotal' /proc/meminfo | sed -e 's/MemTotal://' -e 's/ kB//'`.to_i 
    elsif host =~ /mswin|mingw|cygwin/
      # Windows code via https://github.com/rdsubhas/vagrant-faster
      mem = `wmic computersystem Get TotalPhysicalMemory`.split[1].to_i / 1024
    end

    mem = mem / 1024 * 2 / 3
    v.customize ["modifyvm", :id, "--memory", mem]
  end

  config.vm.synced_folder "./", "/vagrant"

  # run the Freeciv bootstrap script on startup
  config.vm.provision :shell, :path => "scripts/vagrant-build.sh", run: "always"


end
