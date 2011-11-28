#!/bin/sh

test_description='external credential helper tests'
. ./test-lib.sh
. "$TEST_DIRECTORY"/lib-credential.sh

if test -z "$GIT_TEST_CREDENTIAL_HELPER"; then
	say "# skipping external helper tests (set GIT_TEST_CREDENTIAL_HELPER)"
else
	helper_test "$GIT_TEST_CREDENTIAL_HELPER"
fi

if test -z "$GIT_TEST_CREDENTIAL_HELPER_TIMEOUT"; then
	say "# skipping external helper timeout tests"
else
	helper_test_timeout "$GIT_TEST_CREDENTIAL_HELPER_TIMEOUT"
fi

test_done
