# -*- mode: ruby -*-


require_relative "vagrant-common"


Vagrant.configure(2) do |config|

  config.vm.provider :virtualbox do |provider|
    provider.name = "CArrayIntrospection on Fedora 23"
  end

  config.vm.provision "user setup",
                      type: "shell",
                      privileged: false,
                      inline: "scons -C /vagrant IIGLUE="

end
