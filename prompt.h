#ifndef PROMPT_H
#define PROMPT_H

#define PROMPT_ASKPASS (1<<0)

char *git_prompt(const char *prompt, const char *name, int flags);
char *git_getpass(const char *prompt);

#endif /* PROMPT_H */
