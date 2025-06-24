# Fork of dwm and dmenu, combined in one project
# See LICENSE file for copyright and license details.

include config.mk

# === Source Configuration ===

COMMON_SRC = src/common/drw.c src/common/util.c
WM_SRC     = src/wm/wm.c $(COMMON_SRC)
MENU_SRC   = src/menu/menu.c $(COMMON_SRC)
SRC        = $(WM_SRC) $(MENU_SRC)
OBJ        = ${SRC:.c=.o}

# === Build Targets ===

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
	chmod 755 ${DESTDIR}${PREFIX}/bin/menu
	cp -f src/scripts/launch_app src/scripts/switch_app ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/launch_app
	chmod 755 ${DESTDIR}${PREFIX}/bin/switch_app

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/wm \
	      ${DESTDIR}${PREFIX}/bin/menu \
	      ${DESTDIR}${PREFIX}/bin/launch_app \
	      ${DESTDIR}${PREFIX}/bin/switch_app

# === Remote Development & Debugging ===

# VM Configuration
REMOTE_IP := $(shell vagrant ssh-config 2>/dev/null | awk '/HostName/ { print $$2 }')
REMOTE_USER := vagrant
REMOTE_DIR := /home/$(REMOTE_USER)/xwm
GDB_PORT := 5555

# Sync code to VM and rebuild with correct HOST_IP context
rebuild:
	vagrant rsync
	vagrant ssh -c 'bash /home/vagrant/xwm/scripts/rebuild.sh'

debug:
	@echo "Connecting to gdbserver at $(REMOTE_IP):$(GDB_PORT)..."
	@gdb -tui -ex "target remote $(REMOTE_IP):$(GDB_PORT)" -ex continue

# Tail log output from inside VM (2)  
tail-log:
	vagrant ssh -c 'touch /var/log/wm && tail -f /var/log/wm'

# View the VM GUI (3)
view:
	remote-viewer vnc://127.0.0.1:5901 &

# Composite target of 1, 2, and 3 above within tmux
start: rebuild view
	@tmux new-session -d -s xwm_dev \
		"make debug" \; \
		split-window -v "make tail-log" \; \
		select-pane -t 0 \; \
		attach-session -t xwm_dev

# === VM Lifecycle Commands ===

# Reloads the VM (useful for network or config changes)
reload:
	vagrant reload

# Gracefully halts the VM
stop:
	vagrant halt

.PHONY: all clean install uninstall deploy tail-log debug start
