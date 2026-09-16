void
toggleallowkill(const Arg *arg)
{
	if (!selmon->sel)
		return;
	selmon->sel->allowkill = !selmon->sel->allowkill;
}
