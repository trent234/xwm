# version
VERSION = 0.1

# paths
PREFIX = ${HOME}/.local
COMMON = ../common
WM = src/wm
MENU = src/menu
KB = src/kb

# x11
X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# freetype
FREETYPEINC = /usr/include/freetype2
FREETYPELIBS = -lfontconfig -lXft

# includes 
INCS = -I${X11INC} -I${FREETYPEINC} -I${COMMON} -I${WM} -I${MENU}

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L -DVERSION=\"$(VERSION)\"
CFLAGS   = -std=c99 -g -pedantic -Wall -Wno-deprecated-declarations -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -L${X11LIB} -lX11 ${FREETYPELIBS} -lXtst

# compiler and linker
CC = cc
