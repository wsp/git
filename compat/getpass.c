#include "../git-compat-util.h"

#ifdef HAVE_DEV_TTY

char *getpass_echo(const char *prompt)
{
	static char buf[1024];
	FILE *fh;
	int i;

	fh = fopen("/dev/tty", "w+");
	if (!fh)
		return NULL;

	fputs(prompt, fh);
	fflush(fh);
	for (i = 0; i < sizeof(buf) - 1; i++) {
		int ch = getc(fh);
		if (ch == EOF || ch == '\n')
			break;
		buf[i] = ch;
	}
	buf[i] = '\0';

	if (ferror(fh)) {
		fclose(fh);
		return NULL;
	}

	fclose(fh);
	return buf;
}

#else

char *getpass_echo(const char *prompt)
{
	return getpass(prompt);
}

#endif
