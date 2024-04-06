
# xwm - eXperimental window manager

xwm is an experiment that started as a fork of dwm and dmenu. I've been interested in hacking on a wm and instead of starting from scratch thought it'd be more fun to start with a fork of the wm that I've been using for years and have come to appreciate. Kind of like learning how a car works by slowly taking it apart, I'm using dwm to learn about xlib.

One potential goal is to boil this wm into an even simpler version that will be useful on touch devices like the reMarkable 1 and/or the PinePhone. The idea is to use dwm as the start point for the xlib wm components and dmenu for the app launcher which instead of being typing centric will list out gui apps and be scrollable and clickable. Also will be able to list open processes in that launcher as well as programs, and modifying the classic layout to instead have one main client, and all others are hidden. One exception will need to be the onscreen keyboard which will involve learning from the team behind the sxmo-dwm project and use a dock patch where svkb declares itself as a special type of client that gets handled by dwm specially.

## Current Project Status & Notes
The next todo is to make the menu clickable, then add the keyboard, then gesture support.  

Generally speaking these are the changes so far to dwm: 
* Added: 
	* Visibility for non-floating windows is defined as being the top client in the stack
	* Socket server 
		* serves the client stack 
		* accepts request for a change in client selection
		* these endpoints are used to enable app switching with dmenu
	* Default behavior is monocle, while still permitting floating windows
	* menu is now a window, list is size of window
* Removed: 
	* Multi-monitor support
	* Tags
	* Tile layout
	* Fullscreen (technically. in practice monocle is essentially fullscreen.)
	* Layout indicator in bar

I'm removing features partly as an exercise, but it has the benefit of reducing complexity, and certain features are incompatible with the new (simpler) program architecture.  

dwm 2165 / wm 1582 SLOC  
dmenu 796 / menu 665 SLOC 

### Screenshots
#### App Launcher
![launch_app](https://github.com/trent234/xwm/assets/22989914/1d087cd8-7fc9-4022-b0a4-4ed7c619f864)

#### App Switcher
![switch_app](https://github.com/trent234/xwm/assets/22989914/6aafe1ca-10f2-479e-a632-96db882db6ca)

#### App Selected
![app](https://github.com/trent234/xwm/assets/22989914/24fb2318-3a57-4ae2-8b0c-d4f7dbfc6cb0)


#### wm

 This is a dynamic window manager is designed like any other X client as well. It is
 driven through handling X events (and socket events). In contrast to other X clients, a window
 manager selects for SubstructureRedirectMask on the root window, to receive
 events about window (dis-)appearance. Only one X connection at a time is
 allowed to select for this event mask.  

 The event handlers of wm are organized in an array which is accessed
 whenever a new event has been fetched. This allows event dispatching
 in O(1) time.  

 Each child of the root window is called a client, except windows which have
 set the override_redirect flag. Clients are organized in a linked client list.  

 Keys rules are organized as arrays and defined in config.h.  

## Requirements

In order to build you need the Xlib header files.

## Installation

Edit `config.mk` to match your local setup (files are installed into the /usr/local namespace by default).

Afterwards enter the following command to build and install (if necessary as root):

```
make clean install
```

## Running wm

Add the following line to your `.xinitrc` to start the window manager using `startx`:

```
exec wm
```

In order to display status info in the bar, you can do something like this in your `.xinitrc`:

```shell
while xsetroot -name "`date` `uptime | sed 's/.*,//'`"
do
	sleep 1
done &
exec wm
```


## Developing & debugging

I use virtualbox with a virtual NAT as the integral component of the build-run cycle. Create a debian 12 VM and note its IP. 
From the host, within the repo parent run-  
```
scp -r xwm user@192.168.122.35:/tmp
```
Then I can SSH to the VM like so-  
```
ssh user@192.168.122.35  
```
For a new VM certain packages will be required- 
* To pull
  * git
* Project dependencies
  * libx11-dev
  * libxft-dev
* To build
  * make
  * gcc
* To run
  * xorg
* To debug
  * gdb
  * gdbserver
  * valgrind
* To populate many apps to test with
  * gnome
* Missing deps for some gnome apps
  * dbus-launch  

SSH'd into the VM, move the source that was copied over to the same dir path as on the host (required for gdb)- 
```
mv /tmp/xwm /path/to/host/repo/dir
```
Create an ~/.xinitrc with these contents-
```
vi ~/.xinitrc
exec gdbserver 192.168.122.35:5555 wm > /var/log/wm 2>&1
```
And in a separate terminal SSH'd in to the VM run- 
```
touch /var/log/wm
tail -f /var/log/wm
```
Then, in a separate terminal SSH'd into the VM, start the wm with-
```
startx
```
Then on the host, from the repo root dir-
```
gdb -tui
target remote 192.168.122.35:5555
continue
```
At this point you'll have three essential windows-
* A VM running the wm
* A terminal tailing the logs for the wm and its child processes
* A terminal with gdb remotely attached to wm
And that's all you need to debug. Be sure the host repo root has the same version of build output as on remote. That is needed for gdb.  