# Depends

- [tree-sitter]

[tree-sitter]: https://archlinux.org/packages/extra/x86_64/tree-sitter/

# Build

    git clone https://github.com/tree-sitter/tree-sitter-c
    make

# Tests

The tests in `t/` use [sharness] licensed under gpl-2.

Use `TEST_OPT=--verbose make test` for more verbose output

[sharness]: https://github.com/felipec/sharness
