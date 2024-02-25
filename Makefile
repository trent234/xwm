# Fork of dwm and dmenu, combined
# See LICENSE file for copyright and license details.

include config.mk

DEPS_SRC = deps/drw.c deps/util.c
DWM_SRC = wm/dwm.c $(DEPS_SRC)
DMENU_SRC = menu/dmenu.c $(DEPS_SRC)
SRC = $(DWM_SRC) $(DMENU_SRC)
OBJ = ${SRC:.c=.o}

all: dwm dmenu

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ): config.mk

dwm: $(DWM_SRC:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

dmenu: $(DMENU_SRC:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) dwm dmenu

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm dmenu ./menu/dmenu_run ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu
	chmod 755 $(DESTDIR)$(PREFIX)/bin/dmenu_run

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${PREFIX}/bin/dmenu\
		${DESTDIR}${PREFIX}/bin/dmenu_run

.PHONY: all clean dist install uninstall
