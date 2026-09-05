static void
killthis(Client *c)
{
	#if ISPERMANENT_PATCH
	if (c->ispermanent)
		return;
	#endif // ISPERMANENT_PATCH
	#if ALLOWKILLRULE_PATCH
	if (!c->allowkill)
		return;
	#endif // ALLOWKILLRULE_PATCH
	#if BAR_SYSTRAY_PATCH
	if (!sendevent(c->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0, 0, 0))
	#else
	if (!sendevent(c, wmatom[WMDelete]))
	#endif // BAR_SYSTRAY_PATCH
	{
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, c->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
bulkill(const Arg *arg)
{
	Client *c;

	if (!selmon->sel)
		return;

	if (!arg->ui) {
		killthis(selmon->sel);
		return;
	}

	for (c = selmon->clients; c; c = c->next) {
		if (!ISVISIBLE(c) || (arg->ui == 1 && c == selmon->sel))
			continue;
		killthis(c);
	}
}
