#!/bin/sh

test_description='validate format of own code'

. ./helpers.sh
. ./sharness.sh

for f in ../../*.c; do
	[ -f "$f" ] || continue
	test_expect_success "$(basename $f)" "test_format $f"
done

test_done
