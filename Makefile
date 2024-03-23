# Fork of dwm and menu 
# See LICENSE file for copyright and license details.

include config.mk

COMMON_SRC = src/common/drw.c src/common/util.c
WM_SRC = src/wm/wm.c $(COMMON_SRC)
MENU_SRC = src/menu/menu.c $(COMMON_SRC)
SRC = $(WM_SRC) $(MENU_SRC) 
OBJ = ${SRC:.c=.o}

all: wm menu

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ): config.mk

wm: $(WM_SRC:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

menu: $(MENU_SRC:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) wm menu

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f wm menu ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/wm
	chmod 755 $(DESTDIR)$(PREFIX)/bin/menu
	cp -f src/scripts/launch_app src/scripts/switch_app ${DESTDIR}${PREFIX}/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/launch_app
	chmod 755 $(DESTDIR)$(PREFIX)/bin/switch_app

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/wm\
		${DESTDIR}${PREFIX}/bin/menu\
		${DESTDIR}${PREFIX}/bin/launch_app\
		${DESTDIR}${PREFIX}/bin/switch_app

.PHONY: all clean dist install uninstall
