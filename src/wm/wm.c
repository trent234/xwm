/* See LICENSE file for copyright and license details. */

#include <errno.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "../common/drw.h"
#include "../common/util.h"

/* macros */
#define BUTTONMASK		(ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)		(mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define LENGTH(X)		(sizeof X / sizeof X[0])
#define MOUSEMASK		(BUTTONMASK|PointerMotionMask)
#define WIDTH(X)		((X)->w + 2 * (X)->bw)
#define HEIGHT(X)		((X)->h + 2 * (X)->bw)
#define TEXTW(X)		(drw_fontset_getwidth(drw, (X)) + lrpad)
#define SOCKET_PATH 		"/tmp/xwm"
#define DELIMITER 		"\n"

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,NetActiveWindow, NetWMWindowType,
	   NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkStatusText, ClkWinTitle, ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
enum {GetClients, SelectClient, StateDump, Quit}; /* socket commands */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, hintsvalid;
	int bw, oldbw;
	int isfixed, isfloating, isurgent, neverfocus;
	Client *next;
	Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

struct Monitor {
	int by;			/* bar geometry */
	int mx, my, mw, mh;   	/* screen size */
	int wx, wy, ww, wh;   	/* window area  */
	int showbar;
	Client *clients;
	Window barwin;
};

/* function declarations */
static void arrange(void);
static void attach(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void createbar(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void dispatchsocketevent();
static void drawbar(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static Atom getatomprop(Client *c, Atom prop);
static char* getclients(char *unused); 
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void movemouse(const Arg *arg);
static void propertynotify(XEvent *e);
static char* quit(char* unused);
static void resize(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(void);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static char* selectclient(char *body);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setup(void);
static void setupsocket(void);
static void seturgent(Client *c, int urg);
static void spawn(const Arg *arg);
static char* statedump(char *unused);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static Client *wintoclient(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);

/* variables */
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;		   /* X display screen geometry width, height */
static int bh;			   /* bar height */
static int lrpad;			/* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static char* (*shandler[4]) (char *) = {
	[GetClients] = getclients,
	[SelectClient] = selectclient, 
	[StateDump] = statedump,
	[Quit] = quit
};
static void (*xhandler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static int sockfd;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor mon;
static Window root, wmcheckwin;

/* configuration, allows nested code to access above variables */
#include "wm.h"

/*
 * arrange() - This function orchestrates the layout updates for client windows.
 * It is a critical part of the window manager's main event loop, ensuring that
 * the arrangement of windows reflects both the current screen geometry and client
 * states. This design prioritizes responsiveness by keeping the layout recalculations
 * lightweight, which is essential for smooth user interactions.
 */
void
arrange()
{
	Client *c, *next;
	Client *tiled = NULL, *tlast = NULL;
	Client *floating = NULL, *flast = NULL;
	XEvent ev;

	drawbar();
	if (!mon.clients)
		return;
	
	/* Partition clients into tiled and floating lists */
	for (c = mon.clients; c; c = next) {
		next = c->next;
		c->next = NULL;
		if (c->isfloating) {
			if (!floating) {
				floating = c;
				flast = c;
			} else {
				flast->next = c;
				flast = c;
			}
		} else {
			if (!tiled) {
				tiled = c;
				tlast = c;
			} else {
				tlast->next = c;
				tlast = c;
			}
		}
	}
	/* Merge lists: floating clients first, then tiled clients */
	if (floating) {
		mon.clients = floating;
		flast->next = tiled;
	} else {
		mon.clients = tiled;
	}

	restack();

	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
attach(Client *c)
{
    // For floating clients, add them to the head of the list.
    if (c->isfloating) {
        c->next = mon.clients;
        mon.clients = c;
        return;
    }

    // For tiled clients and no other clients, add to the head of the list too. 
    if (!mon.clients) {
        mon.clients = c;
        c->next = NULL;
        return;
    }

    Client *lastFloatingClient = NULL;
    Client *current = mon.clients;

    // Traverse the list to find the last floating client.
    while (current && current->isfloating) {
        lastFloatingClient = current;
        current = current->next;
    }

    if (lastFloatingClient) {
        // Insert new tiled client after the last floating client.
        c->next = lastFloatingClient->next;
        lastFloatingClient->next = c;
    } else {
        // No floating clients found, add new client at the beginning.
        c->next = mon.clients;
        mon.clients = c;
    }
}

/*
 * buttonpress() - Centralizes the dispatching of mouse button events.
 */
void
buttonpress(XEvent *e)
{
	unsigned int i, x, click;
	Client *c;
	XButtonPressedEvent *ev = &e->xbutton;
	fprintf(stderr, "ButtonPress captured: button=%d, state=%u, x=%d, y=%d\n", ev->button, ev->state, ev->x, ev->y);
	
	click = ClkRootWin;
	if (ev->window == mon.barwin) {
		i = x = 0;
		if (ev->x > mon.ww - (int)TEXTW(stext))
			click = ClkStatusText;
		else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		arrange();
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
		for (i = 0; i < LENGTH(buttons); i++) {
		if (click == buttons[i].click 
			&& buttons[i].func 
			&& buttons[i].button == ev->button
			&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state)) 
				buttons[i].func(&buttons[i].arg);
	}
}

/* checkotherwm() - Ensures exclusive control over the X server. */
void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	size_t i;

	close(sockfd);
	unlink(SOCKET_PATH);
	while (mon.clients)
		unmanage(mon.clients, 0);
	focus(NULL);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XUnmapWindow(dpy, mon.barwin);
	XDestroyWindow(dpy, mon.barwin);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

/*
 * clientmessage() - Dispatches client messages to update WM state.
 * Processes activation and urgency hints for consistent behavior.
 */
void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetActiveWindow] && c != mon.clients && !c->isurgent)
		seturgent(c, 1);
}

/*
 * configure() - Syncs client's state by sending a ConfigureNotify event.
 * Centralizes state updates for consistent window management.
 */
void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

/*
 * configurenotify() - Handles configuration notifications from the X server.
 * This function is critical as it dynamically adjusts the window manager's
 * geometry and layout in response to changes in the root window's size.
 */
void
configurenotify(XEvent *e)
{
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			createbar();
			XMoveResizeWindow(dpy, mon.barwin, mon.wx, mon.by, mon.ww, bh);
			focus(NULL);
			arrange();
		}
	}
}

/*
 * configurerequest() - Handles client configuration requests.
 * This function plays a critical role in the dynamic adjustment of window layouts.
 * The decision to process these requests immediately here ensures that client
 * modifications adhere to WM constraints, preserving consistent state and responsiveness
 * across the system’s event loop.
 */
void
configurerequest(XEvent *e)
{
	Client *c;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (c->isfloating) {
			int x = c->x, y = c->y, w = c->w, h = c->h;

			if (ev->value_mask & CWX)       x = mon.mx + ev->x;
			if (ev->value_mask & CWY)       y = mon.my + ev->y;
			if (ev->value_mask & CWWidth)   w = ev->width;
			if (ev->value_mask & CWHeight)  h = ev->height;
	
			resize(c, x, y, w, h);
		}
	} else { /* Rer ICCCM unclaimed windows should not be blocked, so forward it. */
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

void
createbar(void)
{
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"wm", "wm"};
	if (!mon.barwin){
		mon.barwin = XCreateWindow(dpy, root, mon.wx, mon.by, mon.ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, mon.barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, mon.barwin);
		XSetClassHint(dpy, mon.barwin, &ch);
	}
}

/*
 * destroynotify() - Handles window destruction events.
 * Removes a client promptly to prevent stale states.
 */
void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
}

/*
 * detach() - Removes the specified client from the monitor's client list.
 * This function is a critical utility that maintains the internal ordering of
 * clients by unlinking the given client from the list. 
 */
void
detach(Client *c)
{
	Client **tc;

	for (tc = &mon.clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

/*
 * dispatchsocketevent() - Centralizes the processing of incoming socket events.
 * This function is critical because it integrates external IPC requests into the
 * WM's core event loop, allowing remote commands to dynamically influence the state
 * and configuration of the window manager. 
 */
static void dispatchsocketevent(void) {
	int fd = accept(sockfd, NULL, NULL);
	char buffer[1024] = {0};
	ssize_t r_size, w_size;

	if (fd < 0)
		return;

	/* read in request */
	r_size = read(fd, buffer, sizeof(buffer) - 1);
	if (r_size <= 0) {
		close(fd);
		return;
	}
	buffer[r_size] = '\0';  

	/* socket protocol is- request \n payload */
	char *sevent_str = strtok(buffer, DELIMITER);
	char *payload = strtok(NULL, DELIMITER);

	if (sevent_str != NULL) {
		int sevent = atoi(sevent_str);
		/* check if the event is within the bounds of the handler array */
		if (sevent >= 0 && sevent < sizeof(shandler) / sizeof(shandler[0])) {
			/* call handler and write response if one exists */
			char *ret = shandler[sevent](payload);
			if (ret) {
				size_t total_written = 0, len = strlen(ret);
				while (total_written < len) {
					w_size = write(fd, ret + total_written, len - total_written);
					if (w_size < 0) {
						if (errno == EINTR || errno == EAGAIN)
							continue;
						perror("write error");
						break;
					}
					total_written += w_size;
				}
			}
		}
	}
	memset(buffer, 0, sizeof(buffer)); 
	close(fd);
}

/*
 * drawbar() - Renders the status bar providing user feedback and desktop information.
 * This function is critical because it centralizes the rendering of dynamic UI elements,
 * including the active client title and status text. Its design minimizes redraw latency,
 * ensuring a seamless visual experience as WM state changes.
 */
void
drawbar()
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;

	if (!mon.showbar)
		return;

	drw_setscheme(drw, scheme[SchemeNorm]);
	tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
	drw_text(drw, mon.ww - tw, 0, tw, bh, 0, stext, 0);

	x = 0;
	if ((w = mon.ww - tw - x) > bh) {
		if (mon.clients) {
			drw_text(drw, x, 0, w, bh, lrpad / 2, mon.clients->name, 0);
			if (mon.clients->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, mon.clients->isfixed, 0);
		} else {
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	drw_map(drw, mon.barwin, 0, 0, mon.ww, bh);
}

/*
 * enternotify() - Triggers focus updates on pointer entry events.
 * This function is critical for implementing focus-follows-mouse behavior.
 */
void
enternotify(XEvent *e)
{
	Client *c;
	XCrossingEvent *ev = &e->xcrossing;
 
	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	if (!c)
		return;
	fprintf(stderr, "Focus-follows-mouse: losing %s, gaining %s\n", 
			mon.clients ? mon.clients->name : "none", c->name);
	focus(c);
}

/*
 * expose() - Efficiently handles window expose events to refresh visual content.
 * This function is part of the critical redraw path; it ensures that the bar and other
 * window elements are re-rendered promptly when unmapped regions become visible.
 */
void
expose(XEvent *e)
{
	XExposeEvent *ev = &e->xexpose;
	
	if (ev->count == 0 )
		drawbar();
}

/*
 * focus() - Central handler for shifting input focus among client windows.
 * This function not only updates visual indicators (e.g., window borders) but also
 * reorders the client list to maintain WM invariants. The design choice to centralize
 * focus management here ensures that all focus transitions (whether triggered by mouse or
 * programmatically) consistently propagate through the system, simplifying state tracking
 * and event routing. This is key to supporting focus-follows-mouse and ensuring a smooth UX.
 */
void
focus(Client *c)
{
	c = c ? c : mon.clients; /* default to first client if c is NULL */
	if (mon.clients != c) {
		unfocus(mon.clients, 0);
		detach(c);
		attach(c);
	}
	if (c) {
		if (c->isurgent)
			seturgent(c, 0);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
		updatetitle(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	drawbar();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (mon.clients && ev->window != mon.clients->win)
		setfocus(mon.clients);
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

char* 
getclients(char *unused) {
	static char buf[4096];
	int i = 0;
	buf[0] = '\0'; 

	for (Client *c = mon.clients; c != NULL; c = c->next, ++i) {
		/* Ensure buffer doesn't overflow, account for number length, 
		name length, 2 extra chars for space and newline */
		if (strlen(buf) + strlen(c->name) + 10 > sizeof(buf)) 
			break; 
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d %s\n", i, c->name);
	}
	return buf;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

/*
 * gettextprop() - Retrieves a text property from a window.
 * This function dynamically fetching properties such as window names
 * and status indicators that drive the WM's interface updates. 
 */
int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

/*
 * grabbuttons() - Sets up mouse button event grabs for the client window.
 * This function is critical in ensuring that all mouse interactions on a client
 * window are correctly intercepted and handled according to the WM's policy.
 */
void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

/*
 * grabkeys() - Centralizes key grabbing during initialization.
 * This function is critical because it ensures that the WM intercepts all keyboard
 * inputs with the proper modifier combinations (handling variations like NumLock and LockMask).
 */
void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++)
				/* skip modifier codes, we do that ourselves */
				if (keys[i].keysym == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							 keys[i].mod | modifiers[j],
							 root, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

/*
 * keypress() - Dispatches key events to their bound actions.
 * Maps user key inputs to functions.
 */
void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg)
{
	if (!mon.clients)
		return;
	if (!sendevent(mon.clients, wmatom[WMDelete])) {
		detach(mon.clients);
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, mon.clients->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

int
logXErrors(Display *dpy, XErrorEvent *e) {
    char errorMsg[256];
    XGetErrorText(dpy, e->error_code, errorMsg, sizeof(errorMsg));
    fprintf(stderr, "X Error: %s (Code: %d, Request: %d, Minor: %d)\n", 
		errorMsg, e->error_code, e->request_code, e->minor_code);
    return 0; 
}

/*
 * manage() - Integrates a new client window into the window manager.
 * This function is critical because it centralizes client initialization,
 * and setting state for proper integration into the WM's layout
 * and event management. 
 */
void
manage(Window w, XWindowAttributes *wa)
{
	Client *c = NULL;

	/* allocate and initialize client */
	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->bw = c->oldbw = borderpx;

	/* update client state and properties */
	updatetitle(c);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	updatewindowtype(c);
	updatewmhints(c);

	/* setup input and event handling */
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	c->isfloating = c->isfixed;

	/* EWMH and X11 integration */
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);

	setclientstate(c, NormalState);

	/* attach, arrange, resize, notify, render, and focus */
	attach(c);
	arrange();
	resize(c, c->x, c->y, c->w, c->h);
	configure(c); 
	XMapWindow(dpy, c->win);
	focus(NULL);
}

/*
 * mappingnotify() - Refreshes keyboard mapping and re-establishes key bindings.
 * This function is critical in maintaining reliable keyboard input after a 
 * keyboard mapping update (for example, due to hardware changes or locale adjustments).
 */
void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

/*
 * maprequest() - Handles mapping requests for new windows.
 * This function is critical as it immediately integrates new windows
 * into the window manager by checking for override redirects and delegating
 * client management when necessary. 
 */
void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
movemouse(const Arg *arg)
{
	int x, y;
	Client *c;
	XEvent ev;

	if (!(c = mon.clients))
		return;
	if (!c->isfloating)
		return;
	arrange();
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	int ix = x, iy = y;
	int ox = c->x, oy = c->y;
	unsigned long lastMotionTime = 0;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			xhandler[ev.type](&ev);
			break;
		case MotionNotify:
		{
			XMotionEvent *me = &ev.xmotion;
			if ((me->time - lastMotionTime) < 17)
				continue;
			lastMotionTime = me->time;
			int nx = ox + (me->x_root - ix);
			int ny = oy + (me->y_root - iy);
			if (abs(nx - mon.wx) < snap)
				nx = mon.wx;
			if (abs((nx + WIDTH(c)) - (mon.wx + mon.ww)) < snap)
				nx = mon.wx + mon.ww - WIDTH(c);
			if (abs(ny - mon.wy) < snap)
				ny = mon.wy;
			if (abs((ny + HEIGHT(c)) - (mon.wy + mon.wh)) < snap)
				ny = mon.wy + mon.wh - HEIGHT(c);
			resize(c, nx, ny, c->w, c->h);
		}
		break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
}

/*
 * propertynotify() - Integrates property change events into WM state updates.
 * This function is critical as it processes dynamic updates from the X server,
 * including title changes, WM hints, and window type transitions. By centralizing
 * these property notifications, the WM ensures that client state remains consistently
 * synchronized with their visual representation, which is vital for responsive window management.
 */
void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange();
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbar();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == mon.clients)
				drawbar();
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

char*
quit(char* unused)
{
	running = 0;
	return NULL;
}

/*
 * resize() - Centralizes the handling of client window resizing.
 * It applies size hints and then directly updates the client's geometry.
 */
void
resize(Client *c, int x, int y, int w, int h)
{
	if (c->isfixed)
		return;
	
	/* Interactive resizing: adjust position if window would be off-screen */
	if (c->isfloating) {
		if (x > sw) /* off right? */
			x = sw - WIDTH(c); 
		if (y > sh) /* off bottom? */
			y = sh - HEIGHT(c);
		if (x + w + 2 * c->bw < 0) /* off left? */
			x = 0;
		if (y + h + 2 * c->bw < 0) /* off top? */
			y = 0;
		/* Set minimum dimensions: must be greater than bar height */
		if (h < bh)
			h = bh;
		if (w < bh)
			w = bh;
	} else {
		x = mon.wx;
		y = mon.wy;
		w = mon.ww;
		h = mon.wh;
	}
	
	/* If any geometry has changed, update the client window configuration */
	if (x != c->x || y != c->y || w != c->w || h != c->h) {
		XWindowChanges wc;
		c->oldx = c->x; c->x = wc.x = x;
		c->oldy = c->y; c->y = wc.y = y;
		c->oldw = c->w; c->w = wc.width = w;
		c->oldh = c->h; c->h = wc.height = h;
		wc.border_width = c->bw;
		XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
		configure(c);
		XSync(dpy, False);
	}
}

/*
 * resizemouse() - Provides interactive window resizing using mouse events.
 * This function enters a dedicated event loop to capture all resize-related mouse
 * events, ensuring that the window manager can update window dimensions in real time.
 */
void
resizemouse(const Arg *arg)
{
	Client *c;
	XEvent ev;
	int ocx, ocy;
	unsigned long lastMotionTime = 0;
		
	if (!(c = mon.clients))
		return;
	if (!c->isfloating)
		return;
	if (!getrootptr(&ocx, &ocy))
		return;
	int origw = c->w, origh = c->h;
	
	arrange();
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			xhandler[ev.type](&ev);
			break;
		case MotionNotify:
		{
			XMotionEvent *me = &ev.xmotion;
			if ((me->time - lastMotionTime) < 17)
				continue;
			lastMotionTime = me->time;
			int nw = MAX(origw + (me->x_root - ocx), 1);
			int nh = MAX(origh + (me->y_root - ocy), 1);
			if (c->x + nw > mon.wx + mon.ww)
				nw = mon.wx + mon.ww - c->x;
			if (c->y + nh > mon.wy + mon.wh)
				nh = mon.wy + mon.wh - c->y;
			resize(c, c->x, c->y, nw, nh);
		}
		break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
restack(void)
{
	Client *c;
	XWindowChanges wc;

	wc.stack_mode = Below;
	wc.sibling = mon.barwin;
	/* Iterate over sorted client list, configuring windows in order */
	for (c = mon.clients; c; c = c->next) {
		XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
		wc.sibling = c->win;
	}
	XSync(dpy, False);
}

/*
 * run() - Main event loop integrating X events and socket events.
 * This function polls both the X connection and a local socket so that
 * it can handle user interactions (like window events) as well as interprocess
 * commands. Single poll-based loop helps keep things fast.
 */
void run(void) {
	XEvent ev;
	int xfd, timeout;
	struct pollfd fds[2];

	xfd = ConnectionNumber(dpy);
	timeout = 100; // Poll timeout in milliseconds

   	/* Initialize pollfd structure */
	fds[0].fd = xfd;
	fds[0].events = POLLIN; // Check for data to read from X server
	fds[1].fd = sockfd;
	fds[1].events = POLLIN; // Check for data to read from socket

	/* main event loop */
	XSync(dpy, False);
	while (running) {
		/* Poll the set of file descriptors */
		int num_ready = poll(fds, 2, timeout);
		if (num_ready > 0) {
			/* Check for X events without blocking */
			while (XPending(dpy)) {
				XNextEvent(dpy, &ev);
				if (xhandler[ev.type]) {
					xhandler[ev.type](&ev);
				}
			}
			/* Handle socket event */
			if (fds[1].revents & POLLIN) {
				dispatchsocketevent();
			}
		}
	}
}

/*
 * scan() - Integrates pre-existing client windows into the window manager.
 */
void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

char*
selectclient(char *body)
{
	int index, i = 0;

	if (body == NULL || (index = atoi(body)) < 0) 
		return NULL;

	/* Iterate through clients to find the requested one */
	for (Client *c = mon.clients; c != NULL; c = c->next, ++i) {
		if (i == index) {
			focus(c);
			arrange();
			return c->name;
		}
	}
	return NULL;
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

/*
 * sendevent() - Dispatches a client message event if the specified protocol is supported.
 * This function is critical for allowing the WM to communicate with client windows
 * by sending protocol events (e.g., WM_DELETE_WINDOW) that inform them about state changes.
 * Centralizing event dispatch here ensures consistent inter-client communication as per ICCCM.
 */
int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

/*
 * setfocus() - Directs the input focus to a client window while ensuring WM state synchronization.
 * It is critical for maintaining consistent interactive state in the WM event loop.
 * The WM sends a WM_TAKE_FOCUS event after setting focus, allowing applications to adjust behavior
 * based on the active window context.
 */
void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

/*
 * setup() - Initializes the window manager's core state and resources.
 * This function is critical because it establishes the necessary X connections,
 * configures signal handling, and loads essential resources (such as fonts and colors)
 * that define the WM's overall behavior. This centralized initialization ensures that
 * all subsequent components operate within a consistent environment, simplifying
 * system-wide event handling and resource management.
 */
void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	XSetErrorHandler(logXErrors);

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	/* clean up any zombies (inherited from .xinitrc etc) immediately */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	mon.showbar = 1;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	/* init bar */
	createbar();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "wm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
	/* init stack socket */
	setupsocket();
}

/*
 * setupsocket() - Initializes the IPC socket interface.
 * This function is critical because it establishes a non-blocking Unix domain socket for inter-process communication.
 * By enabling external processes to send commands without blocking the main event loop,
 * it enhances the window manager's responsiveness and supports extensibility.
 */
void
setupsocket(void) {
	struct sockaddr_un addr;

	/* Create a socket */
	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
		die("socket failed");

	/* Set socket to non-blocking */
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0) 
		die("fcntl(F_GETFL) failed");
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)
		die("fcntl(F_SETFL) failed");

	/* Remove the socket file if it already exists */
	unlink(SOCKET_PATH);

	/* Set the socket address */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	/* Bind the socket to the address */
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
		die("bind failed");

	/* Listen for incoming connections */
	if (listen(sockfd, SOMAXCONN) < 0) 
		die("listen failed");
	if (chmod(SOCKET_PATH, 0666) < 0)
		die("chmod failed");
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

/*
 * spawn() - Forks a process to execute a command.
 * Decouples execution from the event loop to ensure responsiveness.
 */
void
spawn(const Arg *arg)
{
	struct sigaction sa;

	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("wm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

static char* statedump(char *unused) {
    static char dump[8192];
    int len = 0;
    len += snprintf(dump + len, sizeof(dump) - len,
                    "{\n\"bar_visible\": %s,\n\"screen\": { \"w\": %d, \"h\": %d },\n",
                    (mon.showbar ? "true" : "false"), sw, sh);
    if (mon.clients)
        len += snprintf(dump + len, sizeof(dump) - len,
                        "\"active_client\": \"%s\",\n", mon.clients->name);
    else
        len += snprintf(dump + len, sizeof(dump) - len,
                        "\"active_client\": null,\n");
    len += snprintf(dump + len, sizeof(dump) - len, "\"clients\": [\n");
    int i = 0;
    for (Client *c = mon.clients; c; c = c->next, i++) {
         len += snprintf(dump + len, sizeof(dump) - len,
                         "  { \"id\": %d, \"name\": \"%s\", \"geometry\": { \"x\": %d, \"y\": %d, \"w\": %d, \"h\": %d }, \"state\": \"%s\", \"isfixed\": %d }%s\n",
                         i, c->name, c->x, c->y, c->w, c->h,
                         (c->isfloating ? "Floating" : "Tiled"),
                         c->isfixed,
                         (c->next ? "," : ""));
    }
    len += snprintf(dump + len, sizeof(dump) - len, "]\n}\n");
    return dump;
}

void
togglebar(const Arg *arg)
{
	mon.showbar = !mon.showbar;
	updatebarpos();
	XMoveResizeWindow(dpy, mon.barwin, mon.wx, mon.by, mon.ww, bh);
	for (Client *c = mon.clients; c; c = c->next) {
		if (!c->isfloating)
			resize(c, mon.wx, mon.wy, mon.ww, mon.wh);
	}
	arrange();
}

void
togglefloating(const Arg *arg)
{

	Client *c;
	Window focusedwin;
	int revert;

	XGetInputFocus(dpy, &focusedwin, &revert);
	if (!(c = wintoclient(focusedwin)) || c->isfixed)
		return;
	c->isfloating = !c->isfloating;
	if (c->isfloating)
		resize(c, c->oldx, c->oldy, c->oldw, c->oldh);
	else
		resize(c, mon.wx, mon.wy, mon.ww, mon.wh);
	arrange();
}

/*
 * unfocus() - Handles the removal of focus styling from a client window.
 */
void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

/*
 * unmanage() - Removes a client from the window management stack.
 * This function is critical because it consolidates all cleanup and removal operations
 * when a client window is unmapped or destroyed. 
 */
void
unmanage(Client *c, int destroyed)
{
	XWindowChanges wc;

	detach(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	arrange();
	updateclientlist();
}

/*
 * unmapnotify() - Reacts to window unmapping events to maintain WM state consistency.
 * This function is critical because it distinguishes between client-generated unmap events
 * and those initiated by the X server when focus shifts occur. By updating the client state
 * appropriately (either marking it as withdrawn or fully unmanaging it), the WM prevents orphaned
 * window references and maintains a coherent internal state.
 */
void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
}

/*
 * updatebarpos() - Calculates and updates the bar's position and available window area.
 * This function is critical for adjusting the layout when the bar is visible or hidden.
 * By centralizing the calculation of workspace dimensions based on bar visibility,
 * it ensures that layout changes propagate consistently throughout the window manager.
 */
void
updatebarpos()
{
	mon.wy = mon.my;
	mon.wh = mon.mh;
	if (mon.showbar) {
		mon.wh -= bh;
		mon.by = mon.wy;
		mon.wy = mon.wy + bh; 
	} else
		mon.by = -bh;
}

/*
 * updateclientlist() - Synchronizes the WM's client list with the EWMH _NET_CLIENT_LIST property.
 * This function is critical because it ensures that external desktop components and
 * standards-compliant tools accurately reflect the current set of managed windows.
 */
void
updateclientlist()
{
	Client *c;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (c = mon.clients; c; c = c->next)
		XChangeProperty(dpy, root, netatom[NetClientList],
			XA_WINDOW, 32, PropModeAppend,
			(unsigned char *) &(c->win), 1);
}

/*
 * updategeom() - Updates monitor geometry in response to screen size changes.
 */
int
updategeom(void)
{
	int dirty = 0;
	if (mon.mw != sw || mon.mh != sh) {
		dirty = 1;
		mon.mw = mon.ww = sw;
		mon.mh = mon.wh = sh;
		updatebarpos();
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}


void
updatestatus(void)
{
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "wm");
	drawbar();
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == mon.clients && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

Client *
wintoclient(Window w)
{
	Client *c;

	for (c = mon.clients; c; c = c->next)
		if (c->win == w)
			return c;
	return NULL;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "wm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("wm: another window manager is already running");
	return -1;
}

int
main(int argc, char *argv[])
{
	if (argc != 1)
		die("usage: wm");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("wm: cannot open display");
	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
