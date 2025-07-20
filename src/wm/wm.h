/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx = 1;           /* border pixel of windows */
static const unsigned int snap     = 32;          /* snap pixel */
static const int showbar           = 1;          /* 0 means no bar */
static const char *fonts[]         = { "monospace:size=10" };
static const char dmenufont[]      = "monospace:size=10";
static const char col_black[]      = "#000000";
static const char col_white[]      = "#FFFFFF";

static const char *colors[][3] = {
    /*               fg         bg         border */
    [SchemeNorm] = { col_white, col_black, col_white },
    [SchemeSel]  = { col_black, col_white, col_black },
};

/* commands */
static const char *launch_app[] = { "launch_app", NULL };
static const char *switch_app[] = { "switch_app", NULL };
static const char *toggle_kb[] 	= { "toggle_kb", NULL };
static const char *quit_cmd[]  	= { "quit_wm", "-p", NULL };

static const Key keys[] = {
    /* modifier            key          function           argument */
    { Mod1Mask,            XK_f,        spawn,             { .v = launch_app } },
    { Mod1Mask,            XK_j,        spawn,             { .v = switch_app } },
    { Mod1Mask,            XK_k,        spawn,             { .v = toggle_kb } },
    { Mod1Mask,            XK_d,        togglebar,         { 0 } },
    { Mod1Mask|ShiftMask,  XK_j,        killclient,        { 0 } },
    { Mod1Mask|ShiftMask,  XK_f,        spawn,             { .v = quit_cmd } },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
    /* click          event mask     button      function         argument */
    { ClkClientWin,   Mod1Mask,      Button1,   movemouse,       { 0 } },
    { ClkClientWin,   Mod1Mask,      Button2,   togglefloating,  { 0 } },
    { ClkClientWin,   Mod1Mask,      Button3,   resizemouse,     { 0 } },
};

