void
movetoedge(const Arg *arg)
{
	Client *c = selmon->sel;
	int x, y, nx, ny;

	if (!c || !arg)
		return;
	if (selmon->lt[selmon->sellt]->arrange && !c->isfloating)
		return;
	if (sscanf((char *)arg->v, "%d %d", &x, &y) != 2)
		return;

	if (x == 0)
		nx = selmon->wx + (selmon->ww - WIDTH(c)) / 2;
	else if (x == -1)
		nx = selmon->wx;
	else if (x == 1)
		nx = selmon->wx + selmon->ww - WIDTH(c);
	else
		nx = c->x;

	if (y == 0)
		ny = selmon->wy + (selmon->wh - HEIGHT(c)) / 2;
	else if (y == -1)
		ny = selmon->wy;
	else if (y == 1)
		ny = selmon->wy + selmon->wh - HEIGHT(c);
	else
		ny = c->y;

	XRaiseWindow(dpy, c->win);
	resize(c, nx, ny, c->w, c->h, True);
}
