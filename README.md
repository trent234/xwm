# xwm - eXperimental window manager

xwm is an experiment that started as a fork of dwm and dmenu. I've been interested in hacking on a wm and instead of starting from scratch thought it'd be more fun to start with a fork of the wm that I've been using for years and have come to appreciate. Kind of like learning how a car works by slowly taking it apart, I'm using dwm to learn about xlib.

One potential goal is to boil this wm into an even simpler version that will be useful on touch devices like the reMarkable 1 and/or the PinePhone. The idea is to use dwm as the start point for the xlib wm components and dmenu for the app launcher which instead of being typing centric will list out gui apps and be scrollable and clickable. Also will be able to list open processes in that launcher as well as programs, and modifying the classic layout to instead have one main client, and all others are hidden. One exception will need to be the onscreen keyboard which will involve learning from the team behind the sxmo-dwm project and use a dock patch where svkb declares itself as a special type of client that gets handled by dwm specially.

## Current Project Status & Notes

Not currently in a working state.

Generally speaking these are the changes so far
- removed: multi-monitor support
- removed: tags
- removed: tile layout. Default behavior is monocle, while still permitting floating windows.
- added: socket server which both serves a requested client name and accepts changes in client selection

I'm removing features partly as an exercise, but it has the benefit of reducing complexity, and certain features are incompatible with the new (simpler) program architecture. 

## Requirements

In order to build dwm you need the Xlib header files.

## Installation

Edit `config.mk` to match your local setup (dwm is installed into the /usr/local namespace by default).

Afterwards enter the following command to build and install dwm (if necessary as root):

```
make clean install
```

## Running dwm

Add the following line to your `.xinitrc` to start dwm using `startx`:

```
exec dwm
```

In order to display status info in the bar, you can do something like this in your `.xinitrc`:

```shell
while xsetroot -name "`date` `uptime | sed 's/.*,//'`"
do
	sleep 1
done &
exec dwm
```
