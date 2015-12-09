# -*- mode: ruby -*-

Vagrant.configure(2) do |config|

  config.vm.box = "fedora/23-cloud-base"

  config.vm.provider :libvirt do |provider|
    provider.memory = 1024
  end

  config.vm.provider :virtualbox do |provider|
    provider.customize ["modifyvm", :id, "--ostype", "Fedora_64"]
    provider.memory = 1024
    provider.name = "CArrayIntrospection on Fedora 23"
    #config.vm.synced_folder ".", "/vagrant-live"
  end

  config.vm.synced_folder ".", "/vagrant",
                          rsync__args: [
                            "--archive",
                            "--compress",
                            "--delete",
                            "--filter=:- .gitignore",
                            "--verbose",
                          ]

  config.vm.provision "system package updates and additions",
                      type: "shell",
                      inline: <<-SHELL
    dnf --assumeyes update
    dnf --assumeyes install \
      boost-devel \
      clang \
      git \
      llvm-devel \
      qepcad-B \
      sagemath \
      scons
  SHELL
  config.vm.provision :reload
  config.vbguest.auto_reboot = false

  config.vm.provision "user setup",
                      type: "shell",
                      privileged: false,
                      inline: "scons -C /vagrant IIGLUE="

end
