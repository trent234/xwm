/* See LICENSE file for copyright and license details. */

static const char *fonts[] = {
	"monospace:size=20"
};

static const char *colors[SchemeLast][2] = {
	/*     fg         bg       */
	[SchemeNorm] = { "#000000", "#FFFFFF" },
	[SchemeSel] = { "#FFFFFF", "#000000" },
	[SchemeOut] = { "#000000", "#00ffff" },
};

/*
 * Characters not considered part of a word while deleting words
 * for example: " /?\"&[]"
 */
static const char worddelimiters[] = " ";
