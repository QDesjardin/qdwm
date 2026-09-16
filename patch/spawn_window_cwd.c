static int
getwincwd(char *buf, size_t n)
{
	pid_t pid = 0, child = 0, last = 0;
	char path[64], tmp[PATH_MAX];
	FILE *fp;
	ssize_t len;

	if (!selmon->sel)
		return -1;
	#if SWALLOW_PATCH
	pid = selmon->sel->pid;
	#endif // SWALLOW_PATCH
	if (pid <= 0)
		return -1;

	snprintf(path, sizeof path, "/proc/%d/task/%d/children", pid, pid);
	fp = fopen(path, "r");
	if (fp) {
		while (fscanf(fp, "%d", &child) == 1)
			last = child;
		fclose(fp);
	}
	if (last > 0)
		pid = last;

	snprintf(path, sizeof path, "/proc/%d/cwd", pid);
	len = readlink(path, tmp, sizeof tmp - 1);
	if (len < 0)
		return -1;
	tmp[len] = '\0';
	strncpy(buf, tmp, n);
	buf[n - 1] = '\0';
	return 0;
}
