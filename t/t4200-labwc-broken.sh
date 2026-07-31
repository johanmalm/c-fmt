#!/bin/sh

test_description='validate format of labwc'

. ./helpers.sh
. ./sharness.sh

test_expect_success "cycle." "test_format ../labwc/src/cycle/cycle.c"
test_expect_success "osd-field.c" "test_format ../labwc/src/cycle/osd-field.c"
test_expect_success "img-xpm.c" "test_format ../labwc/src/img/img-xpm.c"
test_expect_success "keybind.c" "test_format ../labwc/src/config/keybind.c"
test_expect_success "session.c" "test_format ../labwc/src/config/session.c"
test_expect_success "rcxml.c" "test_format ../labwc/src/config/rcxml.c"
test_expect_success "workspaces" "test_format ../labwc/src/workspaces.c"
test_expect_success "action.c" "test_format ../labwc/src/action.c"
test_expect_success "tablet.c" "test_format ../labwc/src/input/tablet.c"
test_expect_success "touch.c" "test_format ../labwc/src/input/touch.c"
test_expect_success "keyboard.c" "test_format ../labwc/src/input/keyboard.c"
test_expect_success "string-helpers.c" "test_format ../labwc/src/common/string-helpers.c"
test_expect_success "dir.c" "test_format ../labwc/src/common/dir.c"
test_expect_success "buf.c" "test_format ../labwc/src/common/buf.c"
test_expect_success "menu.c" "test_format ../labwc/src/menu/menu.c"

test_done
