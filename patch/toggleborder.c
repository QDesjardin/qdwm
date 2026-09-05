void
toggleborder(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!c)
		return;
	c->bw = (c->bw == borderpx ? 0 : borderpx);
	arrange(selmon);
}
