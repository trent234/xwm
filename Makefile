# Fork of dwm and dmenu, combined in one project
# See LICENSE file for copyright and license details.

include config.mk

# === Source Configuration ===

COMMON_SRC = src/common/drw.c src/common/util.c
WM_SRC     = src/wm/wm.c $(COMMON_SRC)
MENU_SRC   = src/menu/menu.c $(COMMON_SRC)
KB_SRC     = $(KB)/kb.c $(COMMON_SRC)
SRC        = $(WM_SRC) $(MENU_SRC)
OBJ        = ${SRC:.c=.o}

# === Build Targets ===
BIN_DIR = bin
all: wm menu kb

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJ): config.mk

wm: $(WM_SRC:.c=.o)
	$(CC) -o $(BIN_DIR)/wm $^ $(LDFLAGS)

menu: $(MENU_SRC:.c=.o)
	$(CC) -o $(BIN_DIR)/menu $^ $(LDFLAGS)

kb: $(KB_SRC:.c=.o)
	$(CC) -o $(BIN_DIR)/kb $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN_DIR)/wm $(BIN_DIR)/menu $(BIN_DIR)/kb

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f $(BIN_DIR)/* src/scripts/* ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/*

INSTALL_BINS = $(notdir $(wildcard $(BIN_DIR)/*))
INSTALL_SCRIPTS = $(notdir $(wildcard src/scripts/*))

uninstall:
	rm -f $(addprefix ${DESTDIR}${PREFIX}/bin/, $(INSTALL_BINS) $(INSTALL_SCRIPTS))

# === Remote Development & Debugging ===

# VM Configuration
REMOTE_IP := $(shell vagrant ssh-config 2>/dev/null | awk '/HostName/ { print $$2 }')
REMOTE_USER := vagrant
REMOTE_DIR := /home/$(REMOTE_USER)/xwm
GDB_PORT := 5555

# Sync code to VM and rebuild, restart X and thus gdbserver (0)
rebuild:
	vagrant rsync
	vagrant ssh -c 'bash /home/vagrant/xwm/scripts/rebuild.sh'

# Tail log output from inside VM (2)  
debug:
	gdb -ex "target remote $(REMOTE_IP):$(GDB_PORT)" 

# Tail log output from inside VM (2)  
tail-log:
	vagrant ssh -c 'touch /var/log/wm && tail -f /var/log/wm'

# View the VM GUI (3)
view:
	remote-viewer vnc://127.0.0.1:5901 &

# Composite target of 0-3 above within tmux for one command dev setup
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
