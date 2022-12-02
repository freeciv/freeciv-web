# coding: utf-8
# -*- mode: ruby -*-
# vi: set ft=ruby :

# Freeciv-web Vagrant Vagrantfile
# 2014-02-17 - Andreas Røsdal
#
# Run 'vagrant up' in this directory, which will create a VirtualBox image
# and install Freeciv-web for you.
# Then point your browser to http://localhost/ on your host OS.
#

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "ubuntu-jammy"

  config.vm.provider "virtualbox"

  config.vm.box_url = "https://cloud-images.ubuntu.com/jammy/current/jammy-server-cloudimg-amd64-vagrant.box"

  if Vagrant::Util::Platform.windows?
	  config.vm.network :forwarded_port, guest: 80, host: 80, host_ip: "127.0.0.1"
	  config.vm.network :forwarded_port, guest: 443, host: 443, host_ip: "127.0.0.1"
  else 
	  config.vm.network :forwarded_port, guest: 80, host: 8080, host_ip: "127.0.0.1"
	  config.vm.network :forwarded_port, guest: 443, host: 8443, host_ip: "127.0.0.1"
  end

  config.vm.provider "virtualbox" do |v|
    v.cpus = "1"
    host = RbConfig::CONFIG['host_os']

    # Give VM 50% of system memory 
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

    mem = mem / 1024 / 2
    v.customize ["modifyvm", :id, "--memory", mem]

    # Fix so that symlinks can be created in Windows.
    v.customize ['setextradata', :id, "VBoxInternal2/SharedFoldersEnableSymlinksCreate//vagrant", '1']

    # Disable serial port. It is unnecessary, and may cause error on Win10
    #   https://github.com/joelhandwell/ubuntu_vagrant_boxes/issues/1#issuecomment-292370353
    v.customize ["modifyvm", :id, "--uartmode1", "disconnected"]
  end

  config.vm.synced_folder "./", "/vagrant"

  # There are problems with the default configuration of a DNS stub in some
  # versions of systemd-resolved
  config.vm.provision "systemd-resolved workaround", type: "shell", inline: "ln -sf /run/systemd/resolve/resolv.conf /etc/resolv.conf"
  # run the Freeciv bootstrap script on machine creation
  config.vm.provision "bootstrap", type: "shell", inline: "/vagrant/scripts/install/install.sh --mode=TEST", privileged: false
  # run the Freeciv start script on startup
  config.vm.provision "startup", type: "shell", inline: "/vagrant/scripts/start-freeciv-web.sh", run: "always", privileged: false

end
