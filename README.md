
# xwm - eXperimental window manager

## Overview

**xwm** is a minimalist X11 window manager focused on single-application visibility with a built-in launcher and switcher for streamlined user interaction.

Inspired by `dwm` and `dmenu`, **xwm** is designed to run efficiently on constrained or touch-based environments such as the reMarkable 1 and the PinePhone.

### Key Features

- **Single Visible Client**: Only one window is shown at a time; all others are hidden (Except for floating/dialog windows which are on top).
- **Integrated App Launcher**: A scrollable and clickable launcher (built on `dmenu`) replaces traditional typing-based app search.
- **Process and Application View**: The launcher can display both installed applications and currently running processes.
- **Touch Device Support**: Designed to work with on-screen keyboards and limited input

This project extends the simplicity of `dwm`, `dmenu`, and `svkbd` to deliver a clean, distraction-free windowing model with touch compatibility.

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

These controls are modifiable in wm.h but below outlines the defaults.  
By default the window manager uses `Alt` (`Mod1`) as the primary modifier key.  

#### Keyboard Shortcuts

- `` Alt + F `` → Launch app
- `` Alt + J `` → Switch app
- `` Alt + D `` → Toggle the status bar
- `` Alt + K `` → Toggle the keyboard
- `` Alt + Shift + J `` → Quit the focused window
- `` Alt + Shift + F `` → Quit the window manager

#### Mouse Buttons (while holding Alt):  
Moving and resizing windows is only valid on floating windows.  

- ``[ Left Click  ]`` → Move window  
- ``[ Middle Click]`` → Toggle floating  
- ``[ Right Click ]`` → Resize window  

## Screenshots
#### App Launcher
![launch_app](https://github.com/user-attachments/assets/4b31b612-f2c9-45d1-ac1f-593541082104)

#### App Switcher
![switch_app](https://github.com/user-attachments/assets/78c21374-0cd4-4579-abf4-ffe142058032)

#### App Selected
![app](https://github.com/user-attachments/assets/f670eb39-35e0-4201-8c05-4d0a5b6c3236)

#### Quit WM Confirmation
![quit qm](https://github.com/user-attachments/assets/78875f92-0681-43d5-86f4-d5411969cd8d)
