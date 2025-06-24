# xwm Development Guide

This document outlines the complete development and debugging workflow for `xwm`, a fork of `dwm` and `dmenu`, packaged with a remote debugging environment using Vagrant, libvirt, and gdb.

---

## Project Overview

This repo contains:
- `wm`: the main window manager
- `menu`: the app launcher menu
- Shared drawing and utility code under `src/common`

Builds are managed with `make`. Debugging is done remotely inside a VM that mirrors the host project structure for seamless GDB integration.

---

## Required packages 
```
sudo apt install gcc make gdb vagrant rsync libvirt-daemon-system libvirt-clients qemu-kvm bridge-utils remote-viewer
```

## First-Time Setup

Before using the `Makefile` targets for building or debugging, the development VM must be created and started with Vagrant.

### 1. Start the VM

Use the following command to initialize the virtual machine using the Libvirt provider:

```
vagrant up --provider=libvirt
```
See the lifecycle targets below for modifying the state of this VM after its been created.  


##  Makefile Targets

The `Makefile` provides a complete build, deploy, debug, and VM lifecycle workflow.

###  Local Build Targets

| Target          | Description                                             |
|------------------|---------------------------------------------------------|
| `make`           | Builds both `wm` and `menu` binaries                    |
| `make clean`     | Removes all build artifacts (`*.o`, `wm`, `menu`)       |
| `make install`   | Installs binaries and scripts to `${PREFIX}/bin`        |
| `make uninstall` | Removes installed files from `${PREFIX}/bin`            |

---

###  VM Development Targets

The typical test iteration involves running these four commands in the sequence 
seen below. The user will end with a VM running up to date code from the current repo state 
as well as a terminal running gdb and a terminal tailing all relevant logs.  There will also be 
a VNC graphical window. To redeploy an updated version, run make rebuild. 
Quit gdb which will terminate xwm and re-run `make debug` to restart the wm 
with the latest version.  

| Target          | Description                                                                 |
|------------------|-----------------------------------------------------------------------------|
| `make rebuild`   | Syncs code to the VM and triggers `vagrant provision` to rebuild `wm`       |
| `make tail-log`  | Tails runtime log output from the VM (`/var/log/wm`) using `vagrant ssh`    |
| `make view`      | Launches `remote-viewer` to view the VM's GUI 				|
| `make debug`     | Launches `gdb` in TUI mode and attaches to the remote `gdbserver` running `wm` |

---

###  VM Lifecycle Targets

If the VM gets in an odd state, run `make reload` to reset state and start fresh.  

| Target         | Description                                            |
|----------------|--------------------------------------------------------|
| `make reload`  | Reloads the VM (e.g. after config or networking changes) |
| `make stop`    | Halts the VM   |

