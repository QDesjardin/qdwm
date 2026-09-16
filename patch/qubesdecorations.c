/* Qubes OS decorations: _QUBES_LABEL / _QUBES_VMNAME
 * Port of dwm-qubesdecorations-6.3 + dwm-qubesrules-6.3 for flexipatch.
 * Label 0 = dom0 (no property). Labels 1-8 match Qubes GUI colours.
 */

int
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
	if (label < 0 || label > 8)
		label = 0;
	return label;
}

int
qubesscheme(Client *c)
{
	return SchemeQubes0 + c->label;
}

void
updatequbestitle(Client *c)
{
	if (!gettextprop(c->win, qubesatom[QubesVMName], c->vmname, sizeof c->vmname))
		strcpy(c->vmname, qubesdom0);
	c->label = getlabel(c);
	snprintf(c->dispname, sizeof c->dispname, "[%s] %s", c->vmname, c->name);
}
