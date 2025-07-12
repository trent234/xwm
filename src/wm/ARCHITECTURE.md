---

title: "WM Architecture Guide" last\_updated: 2025-06-23 audience:

- Human developers onboarding to **wm**
- LLM coding/analysis agents (tool‑aware) purpose: > Provide a concise, contract‑oriented map of **wm** so that both people and AI agents can extend or refactor the codebase with confidence. language: "C17" process\_model: "Single‑threaded event loop + poll() multiplexing" ipc\_mechanism: "UNIX‑domain socket / tmp/wm.sock" layout\_variants: ["fullscreen-stack", "floating"]

---

# WM Architecture (Contract‑First Edition)

> **TL;DR** – **wm** is a \~2 k‑line X11 window manager in one C file. It runs a single fullscreen‑stack layout (all tiled clients are maximised and stacked beneath the bar) plus optional per‑window floating. It boots, initialises resources, then spins in a `poll()` loop that alternates between X events and IPC messages.

---

## Table of Contents

1. [Context Snapshot](#1-context-snapshot)
2. [Runtime Contracts](#2-runtime-contracts)
   1. [Process & Threading Model](#21-process--threading-model)
   2. [Event Loop](#22-event-loop-contract)
   3. [Client Lifecycle](#23-client-lifecycle-contract)
3. [Module‑Level Contracts](#3-module-level-contracts)
4. [Key Sequences](#4-key-sequences)
5. [Glossary](#5-glossary)
6. [Design Notes vs dwm](#6-design-notes-vs-dwm)

---

## 1. Context Snapshot

| Artifact     | Responsibility                   | Notes                      |
| ------------ | -------------------------------- | -------------------------- |
| **wm.c**     | Implements *all* runtime logic   | \~2 k LOC, C17             |
| **wm.h**     | Public API & compile‑time config | Keybindings, colors, rules |
| **Makefile** | Build & install targets          | `make clean install`       |

---

## 2. Runtime Contracts

### 2.1 Process & Threading Model

| Aspect  | Guarantee                                                   |
| ------- | ----------------------------------------------------------- |
| Threads | **Exactly one** (main). No pthreads.                        |
| FDs     | X connection FD + IPC socket FD are always in `poll()` set. |
| Signals | `SIGCHLD` ignored (clients handle children).                |

### 2.2 Event Loop Contract

```c
while (running) {
    poll(fds, nfds, -1);              // Block until an event
    if (xAvailable) handleX();        // drain all X events
    if (sockReady)  handleSock();     // read one full line cmd
}
```

| Field          | Contract                                                                    |
| -------------- | --------------------------------------------------------------------------- |
| **Input**      | X11 events (`XEvent.type < LASTEvent`), newline‑delimited IPC strings.      |
| **Output**     | Updated window tree + bar; optional socket response.                        |
| **Latency**    | Must return to `poll()` within **<50 ms** per iteration to stay responsive. |
| **Invariants** | Global lists `clients`, `monitors` stay acyclic & null‑terminated.          |

### 2.3 Client Lifecycle Contract

| Phase          | Pre‑conditions                                     | Post‑conditions                                                                                   | Side‑effects                                     |
| -------------- | -------------------------------------------------- | ------------------------------------------------------------------------------------------------- | ------------------------------------------------ |
| **manage()**   | X window is viewable & override\_redirect == False | New `Client` struct linked into `clients`; layout refreshed.                                      | `XSelectInput` masks set; EWMH props updated.    |
| **unmanage()** | `Client` exists in list                            | Client removed; focus restored or passed.                                                         | Bar redraw scheduled.                            |
| **arrange()**  | At least one `Monitor` present                     | All **tiled** `Client`s are resized to fullscreen & stacked; topmost floating (if any) is raised. | Emits `ConfigureNotify` for each resized client. |

---

## 3. Module‑Level Contracts

Below, **Require** / **Ensure** / **Side‑effects** describe each top‑level symbol a coding agent might call.

### 3.1 `main(int argc, char **argv)`

- **Require**: `$DISPLAY` set; no conflicting WM running.
- **Ensure**: Returns exit code 0 on graceful quit.
- **Side‑effects**: Registers X error handlers; calls `setup()`, `scan()`, `run()`, `cleanup()` in that order.

### 3.2 `setup(Display *dpy)`

- **Require**: `dpy` non‑null; connection to X server open.
- **Ensure**: Global atoms, fonts, colors, bar, and key grabs are initialised.
- **Side‑effects**: Creates bar window; opens IPC socket.

### 3.3 `run(void)`

- **Require**: `setup()` completed successfully.
- **Ensure**: Blocks until `running` becomes 0, then returns.
- **Side‑effects**: Continuous screen updates; invokes handlers via dispatch tables.

### 3.4 `arrange(void)`

- **Require**: `mon` (global active monitor) non‑null.
- **Ensure**: For each non‑floating client, `resize()` sets geometry to `(mon.wx, mon.wy, mon.ww, mon.wh)` minus borders. Clients are stacked below bar. If the first client is floating, it is raised instead.
- **Side‑effects**: Generates `XConfigureWindow` and `XRaiseWindow` calls; bar is redrawn.

### 3.5 `togglefloating(const Arg *arg)`

- **Require**: At least one client managed.
- **Ensure**: Toggles `isfloating` flag on the focused client; refreshes layout via `arrange()`.

> **Note for LLM agents**: Unlike dwm, there is **no layout strategy pointer**. `arrange()` always applies the fullscreen‑stack layout, so reasoning about window positions is deterministic and stateless beyond `isfloating`.

---

## 4. Key Sequences

### 4.1 Window MapRequest

```mermaid
graph LR
    MapRequest --> maprequest() --> manage() --> attach() --> arrange() --> XConfigureWindow
```

### 4.2 Focus Follows Mouse

```mermaid
graph LR
    EnterNotify --> enternotify() --> focus() --> XSetInputFocus
```

---

## 5. Glossary

| Term                 | Meaning                                                              |
| -------------------- | -------------------------------------------------------------------- |
| **Client**           | In‑memory struct representing one X window.                          |
| **Monitor**          | Represents one physical screen; holds geometry & bar.                |
| **Fullscreen‑stack** | All tiled windows take full monitor area and are stacked.            |
| **Floating**         | Window bypasses layout; user‑draggable, raised on toggle.            |
| **IPC**              | Simple text protocol over UNIX socket; commands map to `shandler[]`. |

---

## 6. Design Notes vs dwm

| Aspect         | dwm                                          | **wm** (this project)                               |
| -------------- | -------------------------------------------- | --------------------------------------------------- |
| Layouts        | Multiple (`tile`, `monocle`, `floating`…)    | Single built‑in fullscreen‑stack + per‑window float |
| Layout pointer | `void (*layout)(Monitor*)` global            | **None**                                            |
| Tags           | Yes (bitmask per client, up to 9 by default) | **Absent** – single workspace                       |
| LOC            | \~2.4 k                                      | \~2 k (simpler feature set)                         |

This simplification removes runtime layout switches, reducing branching complexity at the cost of flexibility. For many kiosk‑style or minimal workflows, the trade‑off is acceptable; for others, adding a strategy pointer back would be a natural extension.

