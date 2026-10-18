/* Qubes OS decorations: _QUBES_LABEL / _QUBES_VMNAME / _QUBES_LABEL_COLOR
 * Port of dwm-qubesdecorations-6.3 + dwm-qubesrules-6.3 for flexipatch.
 *
 * The label index is only for rules. The colour is _QUBES_LABEL_COLOR,
 * which guid sets to that qube's label colour (custom labels included).
 * Dom0 windows carry neither property and are drawn white when
 * selected, grey when unfocused (same dimming as the title tab).
 */

#define QUBES_DOM0_COLOR 0xffffff
#define QUBES_SCHEME_SLOTS 24

static struct {
	int used;
	unsigned int rgb;
	Clr *scm;
} qubeschemes[QUBES_SCHEME_SLOTS];

static const char *
qubesfg(unsigned int rgb)
{
	unsigned r = (rgb >> 16) & 0xff;
	unsigned g = (rgb >> 8) & 0xff;
	unsigned b = rgb & 0xff;

	/* BT.601. Light labels (white, yellow, orange, green) take ink. */
	if (r * 299 + g * 587 + b * 114 > 140000)
		return "#070605";
	return "#F8E0D4";
}

/* Quieter shade of the same colour, so an unfocused title and
 * window outline stay identifiable. Dom0 white becomes grey. */
static unsigned int
qubesdim(unsigned int rgb)
{
	unsigned r = ((rgb >> 16) & 0xff) * 3 / 5;
	unsigned g = ((rgb >> 8) & 0xff) * 3 / 5;
	unsigned b = (rgb & 0xff) * 3 / 5;

	return (r << 16) | (g << 8) | b;
}

static Clr *
qubes_scheme_for(unsigned int rgb)
{
	int i, slot = -1;
	char bg[16];
	char *names[ColCount];
	Clr *scm;

	rgb &= 0xffffff;
	for (i = 0; i < QUBES_SCHEME_SLOTS; i++) {
		if (qubeschemes[i].used && qubeschemes[i].rgb == rgb)
			return qubeschemes[i].scm;
		if (!qubeschemes[i].used && slot < 0)
			slot = i;
	}
	if (slot < 0) {
		Visual *visual;
		Colormap cmap;

		slot = 0;
		#if BAR_ALPHA_PATCH
		visual = drw->visual;
		cmap = drw->cmap;
		#else
		visual = DefaultVisual(drw->dpy, drw->screen);
		cmap = DefaultColormap(drw->dpy, drw->screen);
		#endif // BAR_ALPHA_PATCH
		if (qubeschemes[slot].scm) {
			for (i = 0; i < ColCount; i++)
				XftColorFree(drw->dpy, visual, cmap, &qubeschemes[slot].scm[i]);
			free(qubeschemes[slot].scm);
		}
		qubeschemes[slot].scm = NULL;
		qubeschemes[slot].used = 0;
	}

	snprintf(bg, sizeof bg, "#%06x", rgb);
	names[ColFg] = (char *)qubesfg(rgb);
	names[ColBg] = bg;
	names[ColBorder] = bg;
	names[ColFloat] = bg;
	#if BAR_ALPHA_PATCH
	{
		unsigned int a[ColCount];

		a[ColFg] = OPAQUE;
		a[ColBg] = baralpha;
		a[ColBorder] = borderalpha;
		a[ColFloat] = borderalpha;
		scm = drw_scm_create(drw, names, a, ColCount);
	}
	#else
	scm = drw_scm_create(drw, names, ColCount);
	#endif // BAR_ALPHA_PATCH
	qubeschemes[slot].used = 1;
	qubeschemes[slot].rgb = rgb;
	qubeschemes[slot].scm = scm;
	return scm;
}

static int
getlabel(Client *c)
{
	int format;
	unsigned long nitems, extra;
	unsigned char *p = NULL;
	Atom actual;
	int label = 0;

	if (XGetWindowProperty(dpy, c->win, qubesatom[QubesLabel], 0L, 1L, False,
			XA_CARDINAL, &actual, &format, &nitems, &extra, &p) == Success && p) {
		if (nitems)
			label = (int)*(unsigned long *)p;
		XFree(p);
	}
	if (label < 0)
		label = 0;
	return label;
}

static void
updatequbescolor(Client *c)
{
	int format;
	unsigned long nitems, extra;
	unsigned char *p = NULL;
	Atom actual;

	c->hasqubescolor = 0;
	c->qubescolor = QUBES_DOM0_COLOR;
	if (XGetWindowProperty(dpy, c->win, qubesatom[QubesLabelColor], 0L, 1L, False,
			XA_CARDINAL, &actual, &format, &nitems, &extra, &p) == Success && p) {
		if (nitems && format == 32) {
			c->qubescolor = (unsigned int)(*(unsigned long *)p & 0xffffff);
			c->hasqubescolor = 1;
		}
		XFree(p);
	}
}

static Clr *
qubesscheme(Client *c, int dim)
{
	unsigned int rgb = QUBES_DOM0_COLOR;

	if (c && c->hasqubescolor)
		rgb = c->qubescolor;
	if (dim)
		rgb = qubesdim(rgb);
	return qubes_scheme_for(rgb);
}

static void
qubesborder(Client *c)
{
	if (!c)
		return;
	XSetWindowBorder(dpy, c->win,
		qubesscheme(c, c != selmon->sel)[c->isfloating ? ColFloat : ColBorder].pixel);
}

static void
updatequbestitle(Client *c)
{
	if (!gettextprop(c->win, qubesatom[QubesVMName], c->vmname, sizeof c->vmname))
		strcpy(c->vmname, qubesdom0);
	c->label = getlabel(c);
	updatequbescolor(c);
	snprintf(c->dispname, sizeof c->dispname, "[%s] %s", c->vmname, c->name);
}
