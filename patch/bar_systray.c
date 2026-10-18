static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
static int refresh_systray_icons = 0;

static int isdwmsystray(Window w);
static void adoptorphansystray(void);

int
width_systray(Bar *bar, BarArg *a)
{
	unsigned int w = 0;
	Client *i;
	if (!systray)
		return 1;
	if (showsystray) {
		for (i = systray->icons; i; w += i->w + systrayspacing, i = i->next);
		if (!w)
			XMoveWindow(dpy, systray->win, -systray->h, bar->by);
	}
	return w ? w + lrpad - systrayspacing : 0;
}

int
draw_systray(Bar *bar, BarArg *a)
{
	if (!showsystray)
		return 0;

	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	unsigned int w;

	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));

		wa.override_redirect = True;
		wa.event_mask = ButtonPressMask|ExposureMask;
		wa.border_pixel = 0;
		systray->h = MIN(a->h, drw->fonts->h);
		#if BAR_ALPHA_PATCH
		wa.background_pixel = 0;
		wa.colormap = cmap;
		systray->win = XCreateWindow(dpy, root, bar->bx + a->x + lrpad / 2, -systray->h, MAX(a->w + 40, 1), systray->h, 0, depth,
						InputOutput, visual,
						CWOverrideRedirect|CWBorderPixel|CWBackPixel|CWColormap|CWEventMask, &wa); // CWBackPixmap
		#else
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		systray->win = XCreateSimpleWindow(dpy, root, bar->bx + a->x + lrpad / 2, -systray->h, MIN(a->w, 1), systray->h, 0, 0, scheme[SchemeNorm][ColBg].pixel);
		XChangeWindowAttributes(dpy, systray->win, CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &wa);
		#endif // BAR_ALPHA_PATCH

		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&systrayorientation, 1);
		#if BAR_ALPHA_PATCH
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
				PropModeReplace, (unsigned char *)&visual->visualid, 1);
		#endif // BAR_ALPHA_PATCH
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&netatom[NetWMWindowTypeDock], 1);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
			/* Icons ejected by an older build sit mapped at the root origin. */
			adoptorphansystray();
		} else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return 0;
		}
	}

	systray->bar = bar;

	wc.stack_mode = Above;
	wc.sibling = bar->win;
	XConfigureWindow(dpy, systray->win, CWSibling|CWStackMode, &wc);

	drw_setscheme(drw, scheme[SchemeNorm]);
	for (w = 0, i = systray->icons; i; i = i->next) {
		#if BAR_ALPHA_PATCH
		wa.background_pixel = 0;
		#else
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		#endif // BAR_ALPHA_PATCH
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		if (refresh_systray_icons)
			XClearArea(dpy, i->win, 0, 0, 0, 0, True);
		w += i->w;
		if (i->next)
			w += systrayspacing;
		if (i->mon != bar->mon)
			i->mon = bar->mon;
	}
	refresh_systray_icons = 0;

	#if !BAR_ALPHA_PATCH
	wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
	XChangeWindowAttributes(dpy, systray->win, CWBackPixel, &wa);
	XClearWindow(dpy, systray->win);
	#endif // BAR_ALPHA_PATCH

	XMoveResizeWindow(dpy, systray->win, bar->bx + a->x + lrpad / 2, (w ? bar->by + a->y + (a->h - systray->h) / 2: -systray->h), MAX(w, 1), systray->h);
	return w;
}

int
click_systray(Bar *bar, Arg *arg, BarArg *a)
{
	return -1;
}

static int
isdwmsystray(Window w)
{
	XClassHint ch = { NULL, NULL };
	int yes;

	if (!XGetClassHint(dpy, w, &ch))
		return 0;
	yes = ch.res_class && !strcmp(ch.res_class, "dwmsystray");
	if (ch.res_name)
		XFree(ch.res_name);
	if (ch.res_class)
		XFree(ch.res_class);
	return yes;
}

static void
addsystrayicon(Window win)
{
	Client *c;
	XWindowAttributes wa;
	XSetWindowAttributes swa;

	if (!showsystray || !systray || !win || wintosystrayicon(win))
		return;
	if (!XGetWindowAttributes(dpy, win, &wa))
		return;
	if (!(c = (Client *)calloc(1, sizeof(Client))))
		die("fatal: could not malloc() %u bytes\n", sizeof(Client));

	c->win = win;
	c->mon = selmon;
	c->next = systray->icons;
	systray->icons = c;
	c->x = c->oldx = c->y = c->oldy = 0;
	c->w = c->oldw = wa.width;
	c->h = c->oldh = wa.height;
	c->oldbw = wa.border_width;
	c->bw = 0;
	c->isfloating = True;
	/* reuse tags field as mapped status */
	c->tags = 1;
	updatesizehints(c);
	updatesystrayicongeom(c, wa.width, wa.height);
	XAddToSaveSet(dpy, c->win);
	XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
	XClassHint ch = {"dwmsystray", "dwmsystray"};
	XSetClassHint(dpy, c->win, &ch);
	XReparentWindow(dpy, c->win, systray->win, 0, 0);
	swa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
	XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
	sendevent(c->win, xatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0, systray->win, XEMBED_EMBEDDED_VERSION);
	XSync(dpy, False);
	setclientstate(c, NormalState);
}

static void
adoptorphansystray(void)
{
	unsigned int i, n;
	Window r, p, *ch = NULL;
	XWindowAttributes wa;

	if (!systray || !XQueryTree(dpy, root, &r, &p, &ch, &n))
		return;
	for (i = 0; i < n; i++) {
		/* Withdrawn icons parked off-screen stay unmapped. Only visible
		 * leftovers (the ones covering the bar) go back into the tray. */
		if (!XGetWindowAttributes(dpy, ch[i], &wa) || wa.map_state != IsViewable)
			continue;
		if (isdwmsystray(ch[i]))
			addsystrayicon(ch[i]);
	}
	if (ch)
		XFree(ch);
}

void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!showsystray || !i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (*ii)
		*ii = i->next;
	refresh_systray_icons = 1;
	/* Unmap first. Reparenting a mapped icon to (0, 0) paints it on the bar. */
	XUnmapWindow(dpy, i->win);
	XReparentWindow(dpy, i->win, root, -1, -1);
	free(i);
	drawbarwin(systray->bar);
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		drawbarwin(systray->bar);
	}
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	if (!systray)
		return;

	int icon_height = systray->h;
	if (i) {
		i->h = icon_height;
		if (w == h)
			i->w = icon_height;
		else if (h == icon_height)
			i->w = w;
		else
			i->w = (int) ((float)icon_height * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > icon_height) {
			if (i->w == i->h)
				i->w = icon_height;
			else
				i->w = (int) ((float)icon_height * ((float)i->w / (float)i->h));
			i->h = icon_height;
		}
		if (i->w > 2 * icon_height)
			i->w = icon_height;
	}
}

void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!showsystray || !systray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo], xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

Client *
wintosystrayicon(Window w)
{
	if (!systray)
		return NULL;
	Client *i = NULL;
	if (!showsystray || !w)
		return i;
	for (i = systray->icons; i && i->win != w; i = i->next);
	return i;
}

