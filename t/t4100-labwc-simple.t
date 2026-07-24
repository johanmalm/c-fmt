#!/bin/sh

test_description='validate format of labwc'

. ./helpers.sh
. ./sharness.sh

for f in $(find ../labwc/src/ -name "*.c"); do
	[ -f "$f" ] || continue

	[ $f = "../labwc/src/interactive.c" ] && continue
	[ $f = "../labwc/src/cycle/cycle.c" ] && continue
	[ $f = "../labwc/src/cycle/osd-field.c" ] && continue
	[ $f = "../labwc/src/img/img.c" ] && continue
	[ $f = "../labwc/src/img/img-xpm.c" ] && continue
	[ $f = "../labwc/src/config/keybind.c" ] && continue
	[ $f = "../labwc/src/config/session.c" ] && continue
	[ $f = "../labwc/src/config/rcxml.c" ] && continue
	[ $f = "../labwc/src/workspaces.c" ] && continue
	[ $f = "../labwc/src/action.c" ] && continue
	[ $f = "../labwc/src/input/tablet.c" ] && continue
	[ $f = "../labwc/src/input/touch.c" ] && continue
	[ $f = "../labwc/src/input/keyboard.c" ] && continue
	[ $f = "../labwc/src/common/string-helpers.c" ] && continue
	[ $f = "../labwc/src/common/dir.c" ] && continue
	[ $f = "../labwc/src/common/buf.c" ] && continue
	[ $f = "../labwc/src/menu/menu.c" ] && continue

	test_expect_success "$f" "test_format $f"
done

test_done
