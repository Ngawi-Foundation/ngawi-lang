# Testing Guide

Ngawi uses unit tests, e2e tests, and golden codegen tests.

## Run all tests

```bash
make test
```

## Test layers

- `tests/lexer` for tokenization
- `tests/parser` for grammar and recovery
- `tests/sema` for type and scope rules
- `tests/e2e` for compile + run behavior, including import success/failure cases
- `tests/golden` for C11 output stability

## Golden workflow

When codegen changes intentionally:

```bash
make update_golden
make test
```

Do not update golden files for unrelated changes.

## Debugging negative tests

By default, expected-failure parser/sema/e2e tests silence diagnostics to keep output clean.

To show full diagnostics while running tests:

```bash
NGAWI_TEST_SHOW_ERRORS=1 make test
```
