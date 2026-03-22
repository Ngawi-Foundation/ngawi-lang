CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -pedantic -g
RELEASE_CFLAGS := -std=c11 -Wall -Wextra -pedantic -O2

SRC := src/main.c src/common/diag.c src/lexer/lexer.c src/parser/parser.c src/sema/sema.c src/codegen/cgen.c
TEST_LEXER_SRC := tests/lexer/test_lexer.c src/lexer/lexer.c
TEST_PARSER_SRC := tests/parser/test_parser.c src/parser/parser.c src/lexer/lexer.c src/common/diag.c
TEST_SEMA_SRC := tests/sema/test_sema.c src/sema/sema.c src/parser/parser.c src/lexer/lexer.c src/common/diag.c

.PHONY: all debug release clean test test_e2e test_golden update_golden

all: debug

debug: ngawic

release:
	$(CC) $(RELEASE_CFLAGS) $(SRC) -o ngawic

ngawic: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o ngawic

clean:
	rm -f ngawic test_lexer test_parser test_sema hello hello.c factorial factorial.c fact fact.c forward forward.c e2e_* e2e_*.c tests/golden/actual/*.c *.o *.c.gcov *.gcda *.gcno

test: test_lexer test_parser test_sema test_e2e test_golden
	./test_lexer
	./test_parser
	./test_sema
	./tests/e2e/run_e2e.sh
	./tests/golden/test_golden.sh

test_lexer: $(TEST_LEXER_SRC)
	$(CC) $(CFLAGS) $(TEST_LEXER_SRC) -o test_lexer

test_parser: $(TEST_PARSER_SRC)
	$(CC) $(CFLAGS) $(TEST_PARSER_SRC) -o test_parser

test_sema: $(TEST_SEMA_SRC)
	$(CC) $(CFLAGS) $(TEST_SEMA_SRC) -o test_sema

test_e2e: ngawic
	@true

test_golden: ngawic
	@true

update_golden: ngawic
	./tests/golden/update_golden.sh
