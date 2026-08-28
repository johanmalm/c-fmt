#!/bin/sh

test_description='validate format of labwc'

. ./helpers.sh
. ./sharness.sh

test_expect_success "cycle." "test_format ../labwc/src/cycle/cycle.c"
test_expect_success "osd-field.c" "test_format ../labwc/src/cycle/osd-field.c"
test_expect_success "img-xpm.c" "test_format ../labwc/src/img/img-xpm.c"
test_expect_success "keybind.c" "test_format ../labwc/src/config/keybind.c"
test_expect_success "rcxml.c" "test_format ../labwc/src/config/rcxml.c"
test_expect_success "action.c" "test_format ../labwc/src/action.c"
test_expect_success "keyboard.c" "test_format ../labwc/src/input/keyboard.c"
test_expect_success "menu.c" "test_format ../labwc/src/menu/menu.c"

test_done
