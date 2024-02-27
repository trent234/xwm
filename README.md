# xwm - eXperimental window manager

xwm is an experiment that started as a fork of dwm and dmenu. I've been interested in hacking on a wm and instead of starting from scratch thought it'd be more fun to start with a fork of the wm that I've been using for years and have come to appreciate. Kind of like learning how a car works by slowly taking it apart, I'm using dwm to learn about xlib.

One potential goal is to boil this wm into an even simpler version that will be useful on touch devices like the reMarkable 1 and/or the PinePhone. The idea is to use dwm as the start point for the xlib wm components and dmenu for the app launcher which instead of being typing centric will list out gui apps and be scrollable and clickable. Final considerations will be listing open processes in that launcher as well as programs, and modifying the classic layout to instead have one main client, and all others are hidden. One exception will need to be the onscreen keyboard. I will investigate what the sxmo project did with sxmo-svkbd for guidance.

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
