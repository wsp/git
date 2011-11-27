#include "cache.h"
#include "run-command.h"
#include "strbuf.h"
#include "prompt.h"

static char *do_askpass(const char *cmd, const char *prompt, const char *name)
{
	struct child_process pass;
	const char *args[3];
	static struct strbuf buffer = STRBUF_INIT;

	args[0] = cmd;
	args[1]	= prompt;
	args[2] = NULL;

	memset(&pass, 0, sizeof(pass));
	pass.argv = args;
	pass.out = -1;

	if (start_command(&pass))
		exit(1);

	strbuf_reset(&buffer);
	if (strbuf_read(&buffer, pass.out, 20) < 0)
		die("failed to read %s from %s\n", name, cmd);

	close(pass.out);

	if (finish_command(&pass))
		exit(1);

	strbuf_setlen(&buffer, strcspn(buffer.buf, "\r\n"));

	return buffer.buf;
}

char *git_prompt(const char *prompt, const char *name, int flags)
{
	if (flags & PROMPT_ASKPASS) {
		const char *askpass;

		askpass = getenv("GIT_ASKPASS");
		if (!askpass)
			askpass = askpass_program;
		if (!askpass)
			askpass = getenv("SSH_ASKPASS");
		if (askpass && *askpass)
			return do_askpass(askpass, prompt, name);
	}

	return getpass(prompt);
}

char *git_getpass(const char *prompt)
{
	return git_prompt(prompt, "password", PROMPT_ASKPASS);
}
