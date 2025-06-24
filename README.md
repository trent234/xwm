
# xwm - eXperimental window manager

## Overview

**xwm** is a minimalist X11 window manager focused on single-application visibility with a built-in launcher and switcher for streamlined user interaction.

Inspired by `dwm` and `dmenu`, **xwm** is designed to run efficiently on constrained or touch-based environments such as the reMarkable 1 and the PinePhone.

### Key Features

- **Single Visible Client**: Only one window is shown at a time; all others are hidden.
- **Integrated App Launcher**: A scrollable and clickable launcher (built on `dmenu`) replaces traditional typing-based app search.
- **Process and Application View**: The launcher can display both installed applications and currently running processes.
- **Touch Device Support**: Designed to work with on-screen keyboards and limited input; will support special client handling (e.g., `svkbd`) using concepts from the [sxmo project](https://sxmo.org) and `dwm`'s dock patch.

This project extends the simplicity of `dwm` and `dmenu` to deliver a clean, distraction-free windowing model with touch compatibility.

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

In order to build you need the Xlib header files e.g-  
```
sudo apt install libx11-dev
```

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

## Usage

These shortcuts are modifiable in wm.h but below outlines the defaults.  
The window manager uses `Alt` (`Mod1`) as the primary modifier key:

- `` Alt + f `` — Launch application launcher (`launch_app`)
- `` Alt + j `` — Switch to a different application (`switch_app`)
- `` Alt + Enter `` — Launch terminal (`uxterm`)
- `` Alt + b `` — Toggle the visibility of the status bar
- `` Alt + Shift + c `` — Close the focused window
- `` Alt + Shift + q `` — Quit the window manager

### Screenshots
#### App Launcher
![launch_app](https://github.com/trent234/xwm/assets/22989914/1d087cd8-7fc9-4022-b0a4-4ed7c619f864)

#### App Switcher
![switch_app](https://github.com/trent234/xwm/assets/22989914/6aafe1ca-10f2-479e-a632-96db882db6ca)

#### App Selected
![app](https://github.com/trent234/xwm/assets/22989914/24fb2318-3a57-4ae2-8b0c-d4f7dbfc6cb0)