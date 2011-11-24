#include "cache.h"
#include "credential.h"
#include "string-list.h"
#include "parse-options.h"

static struct lock_file credential_lock;

static void parse_credential_file(const char *fn,
				  struct credential *c,
				  void (*match_cb)(struct credential *),
				  void (*other_cb)(struct strbuf *))
{
	FILE *fh;
	struct strbuf line = STRBUF_INIT;
	struct credential entry = CREDENTIAL_INIT;

	fh = fopen(fn, "r");
	if (!fh) {
		if (errno != ENOENT)
			die_errno("unable to open %s", fn);
		return;
	}

	while (strbuf_getline(&line, fh, '\n') != EOF) {
		credential_from_url(&entry, line.buf);
		if (entry.username && entry.password &&
		    credential_match(c, &entry)) {
			if (match_cb) {
				match_cb(&entry);
				break;
			}
		}
		else if (other_cb)
			other_cb(&line);
	}

	credential_clear(&entry);
	strbuf_release(&line);
	fclose(fh);
}

static void print_entry(struct credential *c)
{
	printf("username=%s\n", c->username);
	printf("password=%s\n", c->password);
}

static void print_line(struct strbuf *buf)
{
	strbuf_addch(buf, '\n');
	write_or_die(credential_lock.fd, buf->buf, buf->len);
}

static void rewrite_credential_file(const char *fn, struct credential *c,
				    struct strbuf *extra)
{
	umask(077);
	if (hold_lock_file_for_update(&credential_lock, fn, 0) < 0)
		die_errno("unable to get credential storage lock");
	parse_credential_file(fn, c, NULL, print_line);
	if (extra)
		print_line(extra);
	if (commit_lock_file(&credential_lock) < 0)
		die_errno("unable to commit credential store");
}

static void store_credential(const char *fn, struct credential *c)
{
	struct strbuf buf = STRBUF_INIT;

	if (!c->protocol || !(c->host || c->path) ||
	    !c->username || !c->password)
		return;

	strbuf_addf(&buf, "%s://", c->protocol);
	strbuf_addstr_urlencode(&buf, c->username, 1);
	strbuf_addch(&buf, ':');
	strbuf_addstr_urlencode(&buf, c->password, 1);
	strbuf_addch(&buf, '@');
	if (c->host)
		strbuf_addstr_urlencode(&buf, c->host, 1);
	if (c->path) {
		strbuf_addch(&buf, '/');
		strbuf_addstr_urlencode(&buf, c->path, 0);
	}

	rewrite_credential_file(fn, c, &buf);
	strbuf_release(&buf);
}

static void remove_credential(const char *fn, struct credential *c)
{
	if (!c->protocol || !(c->host || c->path))
		return;
	rewrite_credential_file(fn, c, NULL);
}

static int lookup_credential(const char *fn, struct credential *c)
{
	if (!c->protocol || !(c->host || c->path))
		return 0;
	parse_credential_file(fn, c, print_entry, NULL);
	return c->username && c->password;
}

int main(int argc, const char **argv)
{
	const char * const usage[] = {
		"git credential-store [options] <action>",
		NULL
	};
	const char *op;
	struct credential c = CREDENTIAL_INIT;
	char *store = NULL;
	struct option options[] = {
		OPT_STRING_LIST(0, "store", &store, "file",
				"fetch and store credentials in <file>"),
		OPT_END()
	};

	argc = parse_options(argc, argv, NULL, options, usage, 0);
	if (argc != 1)
		usage_with_options(usage, options);
	op = argv[0];

	if (!store)
		store = expand_user_path("~/.git-credentials");
	if (!store)
		die("unable to set up default store; use --store");

	if (credential_read(&c, stdin) < 0)
		die("unable to read credential");

	if (!strcmp(op, "get"))
		lookup_credential(store, &c);
	else if (!strcmp(op, "erase"))
		remove_credential(store, &c);
	else if (!strcmp(op, "store"))
		store_credential(store, &c);
	else
		die("unknown operation: %s", op);

	return 0;
}
