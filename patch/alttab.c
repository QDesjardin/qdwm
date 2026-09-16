int alttabn;          /* move that many clients forward */
int ntabs;            /* number of active clients in tag */
int isalt;
Client **altsnext;    /* array of all clients in the tag */
Window alttabwin;

enum { AlttabGrabMs = 8000 };

static int
mod1keycodes(KeyCode *out, int max)
{
	XModifierKeymap *modmap;
	int i, j, per, n = 0, dup;

	if (tabmodkey && n < max)
		out[n++] = (KeyCode)tabmodkey;

	modmap = XGetModifierMapping(dpy);
	if (!modmap)
		return n;
	per = modmap->max_keypermod;
	for (i = 0; i < per; i++) {
		KeyCode m = modmap->modifiermap[Mod1MapIndex * per + i];
		if (!m)
			continue;
		dup = 0;
		for (j = 0; j < n; j++)
			if (out[j] == m)
				dup = 1;
		if (!dup && n < max)
			out[n++] = m;
	}
	XFreeModifiermap(modmap);
	return n;
}

static int
keycodein(KeyCode kc, KeyCode *set, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (set[i] == kc)
			return 1;
	return 0;
}

static int
mod1held(KeyCode *set, int n)
{
	char keys[32];
	int i;

	XQueryKeymap(dpy, keys);
	for (i = 0; i < n; i++) {
		KeyCode m = set[i];
		if (m && (keys[m / 8] & (1 << (m % 8))))
			return 1;
	}
	return 0;
}

static int
isxmodifier(KeySym ks)
{
	return ks >= XK_Shift_L && ks <= XK_Hyper_R;
}

void
alttab()
{
	Monitor *m = selmon;

	/* move to next window */
	if (m->sel && m->sel->snext) {
		alttabn++;
		if (alttabn >= ntabs)
			alttabn = 0; /* reset alttabn */

		focus(altsnext[alttabn]);
		restack(m);
	}

	/* redraw tab */
	XRaiseWindow(dpy, alttabwin);
	drawalttab(ntabs, 0, m);
}

void
alttabend()
{
	Monitor *m = selmon;
	Client *buff;
	int i;

	if (!isalt)
		return;

	/* Move all clients between first and choosen position,
	 * one down in stack and put choosen client to the first position
	 * so they remain in right order for the next time that alt-tab is used
	 */
	if (ntabs > 1) {
		if (alttabn != 0) { /* if user picked original client do nothing */
			buff = altsnext[alttabn];
			if (alttabn > 1)
				for (i = alttabn; i > 0; i--)
					altsnext[i] = altsnext[i - 1];
			else /* swap them if there are just 2 clients */
				altsnext[alttabn] = altsnext[0];
			altsnext[0] = buff;
		}

		/* restack clients */
		for (i = ntabs - 1; i >= 0; i--) {
		    focus(altsnext[i]);
		    restack(m);
		}

	}

	isalt = 0;
	ntabs = 0;
	if (altsnext) {
		free(altsnext);
		altsnext = NULL;
	}
	if (alttabwin) {
		XUnmapWindow(dpy, alttabwin);
		XDestroyWindow(dpy, alttabwin);
		alttabwin = 0;
	}
}

void
drawalttab(int nwins, int first, Monitor *m)
{
	Client *c;
	int i, h;
	int y = 0;
	int px = m->mx;
	int py = m->my;

	if (first) {
		XSetWindowAttributes wa = {
			.override_redirect = True,
			#if BAR_ALPHA_PATCH
			.background_pixel = 0,
			.border_pixel = 0,
			.colormap = cmap,
			#else
			.background_pixmap = ParentRelative,
			#endif // BAR_ALPHA_PATCH
			.event_mask = ButtonPressMask|ExposureMask
		};

		/* decide position of tabwin */
		if (tabposx == 1)
			px = m->mx + (m->mw / 2) - (maxwtab / 2);
		else if (tabposx == 2)
			px = m->mx + m->mw - maxwtab;

		if (tabposy == 1)
			py = m->my + (m->mh / 2) - (maxhtab / 2);
		else if (tabposy == 2)
			py = m->my + m->mh - maxhtab;

		h = maxhtab;

		#if BAR_ALPHA_PATCH
		alttabwin = XCreateWindow(dpy, root, px, py, maxwtab, maxhtab, 2, depth,
		                             InputOutput, visual,
		                             CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);
		#else
		alttabwin = XCreateWindow(dpy, root, px, py, maxwtab, maxhtab, 2, DefaultDepth(dpy, screen),
		                             CopyFromParent, DefaultVisual(dpy, screen),
		                             CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		#endif // BAR_ALPHA_PATCH

		XDefineCursor(dpy, alttabwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, alttabwin);
	}

	h = maxhtab / ntabs;
	for (i = 0; i < ntabs; i++) { /* draw all clients into tabwin */
		c = altsnext[i];
		if (!ISVISIBLE(c))
			continue;
		if (HIDDEN(c))
			continue;

		drw_setscheme(drw, scheme[c == m->sel ? SchemeSel : SchemeNorm]);
		drw_text(drw, 0, y, maxwtab, h, 0, clienttitle(c), 0, 0);
		y += h;
	}

	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_map(drw, alttabwin, 0, 0, maxwtab, maxhtab);
}

void
alttabstart(const Arg *arg)
{
	Client *c;
	Monitor *m = selmon;
	int grabbed;
	int i;

	altsnext = NULL;
	if (alttabwin)
		alttabend();

	if (isalt == 1) {
		alttabend();
		return;
	}

	isalt = 1;
	alttabn = 0;
	ntabs = 0;

	for (c = m->clients; c; c = c->next) {
		if (!ISVISIBLE(c))
			continue;
		if (HIDDEN(c))
			continue;

		++ntabs;
	}

	if (!ntabs) {
		alttabend();
		return;
	}

	altsnext = (Client **) malloc(ntabs * sizeof(Client *));

	for (i = 0, c = m->stack; c; c = c->snext) {
		if (!ISVISIBLE(c))
			continue;
		if (HIDDEN(c))
			continue;

		altsnext[i] = c;
		i++;
	}

	drawalttab(ntabs, 1, m);

	struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000 };
	KeyCode alts[16];
	int nalts = mod1keycodes(alts, 16);
	struct timespec t0, tnow;
	long elapsed;
	int xfd = ConnectionNumber(dpy);

	/* grab keyboard (take all input from keyboard) */
	grabbed = 1;
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime) == GrabSuccess)
			break;
		nanosleep(&ts, NULL);
		if (i == 1000 - 1)
			grabbed = 0;
	}

	XEvent event;
	alttab();

	if (grabbed == 0) {
		alttabend();
		return;
	}

	/*
	 * The stock patch only ungrabs on keycode tabmodkey (Alt_L / 0x40).
	 * USB boards often fire Alt+Tab with Alt_R or release Alt before this
	 * loop runs, which left XGrabKeyboard held and made the laptop keyboard
	 * look dead until Alt_L was pressed. End the grab if no Mod1 key is
	 * still down, on any Mod1 release, on Escape / a typing key, or after
	 * a timeout.
	 */
	clock_gettime(CLOCK_MONOTONIC, &t0);
	if (!mod1held(alts, nalts))
		grabbed = 0;

	while (grabbed) {
		clock_gettime(CLOCK_MONOTONIC, &tnow);
		elapsed = (tnow.tv_sec - t0.tv_sec) * 1000L
			+ (tnow.tv_nsec - t0.tv_nsec) / 1000000L;
		if (elapsed > AlttabGrabMs || !mod1held(alts, nalts))
			break;

		while (!XPending(dpy)) {
			fd_set fds;
			struct timeval tv;

			clock_gettime(CLOCK_MONOTONIC, &tnow);
			elapsed = (tnow.tv_sec - t0.tv_sec) * 1000L
				+ (tnow.tv_nsec - t0.tv_nsec) / 1000000L;
			if (elapsed > AlttabGrabMs || !mod1held(alts, nalts)) {
				grabbed = 0;
				break;
			}
			FD_ZERO(&fds);
			FD_SET(xfd, &fds);
			tv.tv_sec = 0;
			tv.tv_usec = 100000;
			if (select(xfd + 1, &fds, NULL, NULL, &tv) < 0)
				break;
		}
		if (!grabbed || !XPending(dpy))
			continue;

		XNextEvent(dpy, &event);
		if (event.type != KeyPress && event.type != KeyRelease)
			continue;
		if (event.type == KeyRelease && keycodein(event.xkey.keycode, alts, nalts))
			break;
		if (event.type != KeyPress)
			continue;
		if (event.xkey.keycode == tabcyclekey) {
			alttab();
			clock_gettime(CLOCK_MONOTONIC, &t0);
			continue;
		}
		{
			KeySym ks = XLookupKeysym(&event.xkey, 0);
			if (ks == XK_Escape || !isxmodifier(ks))
				break;
		}
	}

	c = m->sel;
	alttabend();

	XUngrabKeyboard(dpy, CurrentTime);
	focus(c);
	restack(m);
}
