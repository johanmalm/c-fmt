#!/bin/sh

test_description='typedef cast spacing'

. ./sharness.sh

test_expect_success "typedef-name cast keeps unary minus tight" '
	cat >input.c <<-\EOF &&
	typedef int gunichar;
	int f(void)
	{
		return (gunichar)-1;
	}
	EOF
	cat >expect <<-\EOF &&
	typedef int gunichar;

	int f(void)
	{
		return (gunichar)-1;
	}
	EOF
	../../c-fmt input.c >actual &&
	test_cmp expect actual
'

test_expect_success "parenthesized variable stays binary subtraction" '
	cat >input.c <<-\EOF &&
	int f(int a)
	{
		return (a)-1;
	}
	EOF
	cat >expect <<-\EOF &&
	int f(int a)
	{
		return (a) - 1;
	}
	EOF
	../../c-fmt input.c >actual &&
	test_cmp expect actual
'

test_expect_success "non-allowlisted typedef stays spaced" '
	cat >input.c <<-\EOF &&
	typedef int foo_t;
	int f(void)
	{
		return (foo_t)-1;
	}
	EOF
	cat >expect <<-\EOF &&
	typedef int foo_t;

	int f(void)
	{
		return (foo_t) - 1;
	}
	EOF
	../../c-fmt input.c >actual &&
	test_cmp expect actual
'

test_done
