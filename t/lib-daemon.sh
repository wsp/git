#!/bin/sh

if test -z "$GIT_TEST_DAEMON"
then
	skip_all="Daemon testing disabled (define GIT_TEST_DAEMON to enable)"
	test_done
fi

LIB_DAEMON_PORT=${LIB_DAEMON_PORT-'8121'}

DAEMON_PID=
DAEMON_DOCUMENT_ROOT_PATH="$PWD"/repo
DAEMON_URL=git://127.0.0.1:$LIB_DAEMON_PORT

start_daemon() {
	if test -n "$DAEMON_PID"
	then
		error "start_daemon already called"
	fi

	mkdir -p "$DAEMON_DOCUMENT_ROOT_PATH"

	trap 'code=$?; stop_daemon; (exit $code); die' EXIT

	say >&3 "Starting git daemon ..."
	git daemon --listen=127.0.0.1 --port="$LIB_DAEMON_PORT" \
		--reuseaddr --verbose \
		--base-path="$DAEMON_DOCUMENT_ROOT_PATH" \
		"$@" "$DAEMON_DOCUMENT_ROOT_PATH" \
		>&3 2>&4 &
	DAEMON_PID=$!
}

stop_daemon() {
	if test -z "$DAEMON_PID"
	then
		return
	fi

	trap 'die' EXIT

	# kill git-daemon child of git
	say >&3 "Stopping git daemon ..."
	pkill -P "$DAEMON_PID"
	wait "$DAEMON_PID"
	ret=$?
	if test $ret -ne 143
	then
		error "git daemon exited with status: $ret"
	fi
	DAEMON_PID=
}
