/* Qubes OS decorations: _QUBES_LABEL / _QUBES_VMNAME
 * https://dwm.suckless.org/patches/qubesdecorations/
 * https://dwm.suckless.org/patches/qubesrules/
 */

#if QUBES_RULES_PATCH && !QUBES_DECORATIONS_PATCH
#error QUBES_RULES_PATCH requires QUBES_DECORATIONS_PATCH
#endif

enum { QubesLabel, QubesVMName, QubesLast };

static inline const char *
clienttitle(const Client *c)
{
#if QUBES_DECORATIONS_PATCH
	return c->dispname[0] ? c->dispname : c->name;
#else
	return c->name;
#endif // QUBES_DECORATIONS_PATCH
}

#if QUBES_DECORATIONS_PATCH
static Atom qubesatom[QubesLast];
static const char qubesdom0[] = "dom0";

static int getlabel(Client *c);
static int qubesscheme(Client *c);
static void updatequbestitle(Client *c);
#endif // QUBES_DECORATIONS_PATCH
