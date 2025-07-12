host_src = `pwd`.chomp

Vagrant.configure("2") do |config|
  config.vm.define "xwm-dev", primary: true do |xwm|
    xwm.vm.box = "debian/bookworm64"
    xwm.vm.hostname = "xwm-dev"
    xwm.vm.network "private_network", ip: "192.168.122.35"
    xwm.vm.synced_folder ".", "/home/vagrant/xwm", type: "rsync"
    
    xwm.vm.provision "shell", inline: <<-SHELL
  set -e

  echo "Installing dependencies..."
  apt-get update
  apt-get install -y \
    git make gcc gdb gdbserver valgrind \
    xorg libx11-dev libxft-dev libxtst-dev dbus-x11 \
    systemd netcat-openbsd

  echo "Linking guest path to match host path for GDB..."
  mkdir -p $(dirname #{host_src})
  ln -sf /home/vagrant/xwm #{host_src}

  echo "Setting X11 config for non-root user..."
  echo "allowed_users=anybody" > /etc/X11/Xwrapper.config

  echo "Building xwm..."
  cd #{host_src}
  make clean install

  echo "Preparing log file..."
  touch /var/log/wm
  chmod 666 /var/log/wm

  echo "Configuring autologin on tty1..."
  mkdir -p /etc/systemd/system/getty@tty1.service.d
  cat <<EOF >/etc/systemd/system/getty@tty1.service.d/autologin.conf
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin vagrant --noclear %I \$TERM
EOF

  echo "Enabling linger so X can run after boot..."
  loginctl enable-linger vagrant

  echo "Writing .xinitrc to start gdbserver..."
  su - vagrant -c 'cat << "EOF" > ~/.xinitrc
#!/bin/bash
source "$HOME/.xwm_env"
: "${HOST_IP:?Missing HOST_IP}" "${PORT:=5555}"

export PATH="$HOME/.local/bin:$PATH"

exec gdbserver $HOST_IP:$PORT wm > /var/log/wm 2>&1
EOF
chmod +x ~/.xinitrc'

  echo "Starting X session automatically on login..."
  su - vagrant -c 'echo "[ -z \\\"\\\$DISPLAY\\\" ] && [ \\\"\\\$(tty)\\\" = \\\"/dev/tty1\\\" ] && exec startx" > ~/.bash_profile'
    SHELL

    xwm.vm.provider :libvirt do |libvirt|
      libvirt.memory = 2048
      libvirt.cpus = 2

      # Graphics configuration (stable and predictable)
      libvirt.graphics_type = "vnc"
      libvirt.graphics_port = 5901
      libvirt.graphics_ip   = "127.0.0.1"
      libvirt.video_type    = "qxl"
      libvirt.keymap        = "en-us"
    end
  end

  config.vm.post_up_message = <<-MESSAGE
  Development environment ready for xwm.
  MESSAGE
end
