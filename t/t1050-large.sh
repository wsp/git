#!/bin/sh
# Copyright (c) 2011, Google Inc.

test_description='adding and checking out large blobs'

. ./test-lib.sh

test_expect_success setup '
	git config core.bigfilethreshold 200k &&
	echo X | dd of=large bs=1k seek=2000 &&
	echo Y | dd of=huge bs=1k seek=2500
'

test_expect_success 'add a large file or two' '
	git add large huge &&
	# make sure we got a single packfile and no loose objects
	bad= count=0 &&
	for p in .git/objects/pack/pack-*.pack
	do
		count=$(( $count + 1 ))
		test -f "$p" && continue
		bad=t
	done &&
	test -z "$bad" &&
	test $count = 1 &&
	for l in .git/objects/??/??????????????????????????????????????
	do
		test -f "$l" || continue
		bad=t
	done &&
	test -z "$bad"
'

test_expect_success 'checkout a large file' '
	large=$(git rev-parse :large) &&
	git update-index --add --cacheinfo 100644 $large another &&
	git checkout another &&
	cmp large another ;# this must not be test_cmp
'

test_done
