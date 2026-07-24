test_format () {
	#printf '%b\n' "file: $1" >&5
	cat "$1" > expect
	../../c-fmt "$1" > actual
	diff -u expect actual
}
