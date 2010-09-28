#include "exec_cmd.h"
#include <locale.h>
#include <libintl.h>
#ifdef HAVE_LIBCHARSET_H
#include <libcharset.h>
#else
#include <langinfo.h>
#endif
#include <stdlib.h>

extern void git_setup_gettext(void) {
	char *podir;
	char *envdir = getenv("GIT_TEXTDOMAINDIR");
	const char *charset;

	if (envdir) {
		(void)bindtextdomain("git", envdir);
	} else {
		podir = (char *)system_path("share/locale");
		if (!podir) return;
		(void)bindtextdomain("git", podir);
		free(podir);
	}

	(void)setlocale(LC_MESSAGES, "");
	(void)setlocale(LC_CTYPE, "");
#ifdef HAVE_LIBCHARSET_H
	charset = locale_charset();
#else
	charset = nl_langinfo(CODESET);
#endif
	(void)bind_textdomain_codeset("git", charset);
	(void)setlocale(LC_CTYPE, "C");
	(void)textdomain("git");
}
