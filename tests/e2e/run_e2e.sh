#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

run_case() {
  local src="$1"
  local bin="$2"
  local expected="$3"

  ./ngawic build "$src" -o "$bin" >/dev/null
  local got
  got="$(./$bin)"
  if [[ "$got" != "$expected" ]]; then
    echo "E2E FAIL: $src"
    echo "  expected: [$expected]"
    echo "  got:      [$got]"
    return 1
  fi
  rm -f "$bin" "$bin.c"
}

run_case examples/hello.ngawi e2e_hello "Hello, Ngawi"
run_case examples/factorial.ngawi e2e_factorial "fact 5 = 120"
run_case examples/if_else.ngawi e2e_if_else "big"
run_case examples/while.ngawi e2e_while $'1\n2\n3'
run_case examples/forward_call.ngawi e2e_forward "42"

echo "All e2e tests passed"
