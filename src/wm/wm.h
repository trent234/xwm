/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const char *fonts[]          = { "monospace:size=10" };
static const char dmenufont[]       = "monospace:size=10";
static const char col_black[] 		= "#000000";
static const char col_white[] 		= "#FFFFFF";

static const char *colors[][3] = {
    /*               fg         bg         border   */
    [SchemeNorm] = { col_black, col_white, col_black },
    [SchemeSel]  = { col_white, col_black, col_white },
};

static const Rule rules[] = {
	/* class      instance    title       tags mask     isfloating  */
	{ "Gimp",     NULL,       NULL,       0,            1 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0 },
};

/* layout(s) */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */

/* commands */
static const char *launch_app[] = { "launch_app", "-fn", dmenufont, "-nb", col_white, "-nf", col_black, "-sb", col_white, "-sf", col_black, NULL };
static const char *switch_app[] = { "switch_app", "-fn", dmenufont, "-nb", col_white, "-nf", col_black, "-sb", col_white, "-sf", col_black, NULL };
static const char *termcmd[]  = { "uxterm", NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ Mod1Mask,                     XK_f,      spawn,          {.v = launch_app } },
	{ Mod1Mask,                     XK_j,      spawn,          {.v = switch_app } },
	{ Mod1Mask,			            XK_Return, spawn,          {.v = termcmd } },
	{ Mod1Mask,                     XK_b,      togglebar,      {0} },
	{ Mod1Mask|ShiftMask,           XK_c,      killclient,     {0} },
	{ Mod1Mask|ShiftMask,           XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         Mod1Mask,       Button1,        movemouse,      {0} },
	{ ClkClientWin,         Mod1Mask,       Button2,        togglefloating, {0} },
	{ ClkClientWin,         Mod1Mask,       Button3,        resizemouse,    {0} },
};

