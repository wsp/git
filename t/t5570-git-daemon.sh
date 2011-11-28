#!/bin/sh

test_description='test fetching over git protocol'
. ./test-lib.sh

LIB_DAEMON_PORT=${LIB_DAEMON_PORT-5570}
. "$TEST_DIRECTORY"/lib-daemon.sh
start_daemon

test_expect_success 'setup repository' '
	echo content >file &&
	git add file &&
	git commit -m one
'

test_expect_success 'create git-accessible bare repository' '
	mkdir "$DAEMON_DOCUMENT_ROOT_PATH/repo.git" &&
	(cd "$DAEMON_DOCUMENT_ROOT_PATH/repo.git" &&
	 git --bare init &&
	 : >git-daemon-export-ok
	) &&
	git remote add public "$DAEMON_DOCUMENT_ROOT_PATH/repo.git" &&
	git push public master:master
'

test_expect_success 'clone git repository' '
	git clone $DAEMON_URL/repo.git clone &&
	test_cmp file clone/file
'

test_expect_success 'fetch changes via git protocol' '
	echo content >>file &&
	git commit -a -m two &&
	git push public &&
	(cd clone && git pull) &&
	test_cmp file clone/file
'

test_expect_failure 'remote detects correct HEAD' '
	git push public master:other &&
	(cd clone &&
	 git remote set-head -d origin &&
	 git remote set-head -a origin &&
	 git symbolic-ref refs/remotes/origin/HEAD > output &&
	 echo refs/remotes/origin/master > expect &&
	 test_cmp expect output
	)
'

test_expect_success 'prepare pack objects' '
	cp -R "$DAEMON_DOCUMENT_ROOT_PATH"/repo.git "$DAEMON_DOCUMENT_ROOT_PATH"/repo_pack.git &&
	(cd "$DAEMON_DOCUMENT_ROOT_PATH"/repo_pack.git &&
	 git --bare repack &&
	 git --bare prune-packed
	)
'

test_expect_success 'fetch notices corrupt pack' '
	cp -R "$DAEMON_DOCUMENT_ROOT_PATH"/repo_pack.git "$DAEMON_DOCUMENT_ROOT_PATH"/repo_bad1.git &&
	(cd "$DAEMON_DOCUMENT_ROOT_PATH"/repo_bad1.git &&
	 p=`ls objects/pack/pack-*.pack` &&
	 chmod u+w $p &&
	 printf %0256d 0 | dd of=$p bs=256 count=1 seek=1 conv=notrunc
	) &&
	mkdir repo_bad1.git &&
	(cd repo_bad1.git &&
	 git --bare init &&
	 test_must_fail git --bare fetch $DAEMON_URL/repo_bad1.git &&
	 test 0 = `ls objects/pack/pack-*.pack | wc -l`
	)
'

test_expect_success 'fetch notices corrupt idx' '
	cp -R "$DAEMON_DOCUMENT_ROOT_PATH"/repo_pack.git "$DAEMON_DOCUMENT_ROOT_PATH"/repo_bad2.git &&
	(cd "$DAEMON_DOCUMENT_ROOT_PATH"/repo_bad2.git &&
	 p=`ls objects/pack/pack-*.idx` &&
	 chmod u+w $p &&
	 printf %0256d 0 | dd of=$p bs=256 count=1 seek=1 conv=notrunc
	) &&
	mkdir repo_bad2.git &&
	(cd repo_bad2.git &&
	 git --bare init &&
	 test_must_fail git --bare fetch $DAEMON_URL/repo_bad2.git &&
	 test 0 = `ls objects/pack | wc -l`
	)
'

test_remote_error()
{
	do_export=YesPlease
	while test $# -gt 0
	do
		case $1 in
		-x)
			shift
			chmod -X "$DAEMON_DOCUMENT_ROOT_PATH/repo.git"
			;;
		-n)
			shift
			do_export=
			;;
		*)
			break
		esac
	done

	if test $# -ne 3
	then
		error "invalid number of arguments"
	fi

	cmd=$1
	repo=$2
	msg=$3

	if test -x "$DAEMON_DOCUMENT_ROOT_PATH/$repo"
	then
		if test -n "$do_export"
		then
			: >"$DAEMON_DOCUMENT_ROOT_PATH/$repo/git-daemon-export-ok"
		else
			rm -f "$DAEMON_DOCUMENT_ROOT_PATH/$repo/git-daemon-export-ok"
		fi
	fi

	test_must_fail git "$cmd" "$DAEMON_URL/$repo" 2>output &&
	echo "fatal: remote error: $msg: /$repo" >expect &&
	test_cmp expect output
	ret=$?
	chmod +X "$DAEMON_DOCUMENT_ROOT_PATH/repo.git"
	(exit $ret)
}

msg="access denied or repository not exported"
test_expect_success 'clone non-existent' "test_remote_error    clone nowhere.git '$msg'"
test_expect_success 'push disabled'      "test_remote_error    push  repo.git    '$msg'"
test_expect_success 'read access denied' "test_remote_error -x fetch repo.git    '$msg'"
test_expect_success 'not exported'       "test_remote_error -n fetch repo.git    '$msg'"

stop_daemon
start_daemon --informative-errors

test_expect_success 'clone non-existent' "test_remote_error    clone nowhere.git 'no such repository'"
test_expect_success 'push disabled'      "test_remote_error    push  repo.git    'service not enabled'"
test_expect_success 'read access denied' "test_remote_error -x fetch repo.git    'permission denied'"
test_expect_success 'not exported'       "test_remote_error -n fetch repo.git    'repository not exported'"

stop_daemon
test_done
