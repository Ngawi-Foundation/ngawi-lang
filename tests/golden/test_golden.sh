#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

mkdir -p tests/golden/actual

cases=(
  "hello:examples/hello.ngawi"
  "factorial:examples/factorial.ngawi"
  "if_else:examples/if_else.ngawi"
  "while_loop:examples/while.ngawi"
  "forward_call:examples/forward_call.ngawi"
  "for_loop:examples/for_loop.ngawi"
  "break_continue:examples/break_continue.ngawi"
  "modulo:examples/modulo.ngawi"
  "cast:examples/cast.ngawi"
  "string_eq:examples/string_eq.ngawi"
  "compound_assign:examples/compound_assign.ngawi"
  "incdec:examples/incdec.ngawi"
  "len:examples/len.ngawi"
  "string_concat:examples/string_concat.ngawi"
  "elif:examples/elif.ngawi"
)

for entry in "${cases[@]}"; do
  name="${entry%%:*}"
  src="${entry#*:}"

  ./ngawic build "$src" -o "tests/golden/actual/$name" -S >/dev/null

  if ! diff -u "tests/golden/expected/$name.c" "tests/golden/actual/$name.c" >/tmp/ngawi_golden_diff.txt; then
    echo "GOLDEN FAIL: $name"
    cat /tmp/ngawi_golden_diff.txt
    exit 1
  fi

done

echo "All golden codegen tests passed"
