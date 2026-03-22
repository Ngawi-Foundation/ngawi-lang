#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT_DIR"

mkdir -p tests/golden/expected

./ngawic build examples/hello.ngawi -o tests/golden/expected/hello -S >/dev/null
./ngawic build examples/factorial.ngawi -o tests/golden/expected/factorial -S >/dev/null
./ngawic build examples/if_else.ngawi -o tests/golden/expected/if_else -S >/dev/null
./ngawic build examples/while.ngawi -o tests/golden/expected/while_loop -S >/dev/null
./ngawic build examples/forward_call.ngawi -o tests/golden/expected/forward_call -S >/dev/null
./ngawic build examples/for_loop.ngawi -o tests/golden/expected/for_loop -S >/dev/null
./ngawic build examples/break_continue.ngawi -o tests/golden/expected/break_continue -S >/dev/null
./ngawic build examples/modulo.ngawi -o tests/golden/expected/modulo -S >/dev/null
./ngawic build examples/cast.ngawi -o tests/golden/expected/cast -S >/dev/null
./ngawic build examples/string_eq.ngawi -o tests/golden/expected/string_eq -S >/dev/null
./ngawic build examples/compound_assign.ngawi -o tests/golden/expected/compound_assign -S >/dev/null
./ngawic build examples/incdec.ngawi -o tests/golden/expected/incdec -S >/dev/null
./ngawic build examples/len.ngawi -o tests/golden/expected/len -S >/dev/null
./ngawic build examples/string_concat.ngawi -o tests/golden/expected/string_concat -S >/dev/null
./ngawic build examples/elif.ngawi -o tests/golden/expected/elif -S >/dev/null
./ngawic build examples/match.ngawi -o tests/golden/expected/match -S >/dev/null
./ngawic build examples/match_bool.ngawi -o tests/golden/expected/match_bool -S >/dev/null
./ngawic build examples/string_builtins.ngawi -o tests/golden/expected/string_builtins -S >/dev/null
./ngawic build examples/import_main.ngawi -o tests/golden/expected/import_main -S >/dev/null

echo "Golden files updated"
