void
resetnmaster(const Arg *arg)
{
	selmon->nmaster = 1;
	#if PERTAG_PATCH
	selmon->pertag->nmasters[selmon->pertag->curtag] = 1;
	#endif // PERTAG_PATCH
	arrange(selmon);
}
