#include "../git-compat-util.h"

char *getpass_echo(const char *prompt)
{
	return getpass(prompt);
}
