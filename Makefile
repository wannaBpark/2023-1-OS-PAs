TARGET	= mash
CFLAGS	= -g -c -D_POSIX_C_SOURCE -D_GNU_SOURCE -D_XOPEN_SOURCE=700
CFLAGS += -std=c99 -Wall -Wextra -Wno-unused-parameter -Werror
LDFLAGS	=

all: mash toy

mash: pa1.o mash.o parser.o
	gcc $(LDFLAGS) $^ -o $@

toy: toy.o
	gcc $(LDFLAGS) $^ -o $@

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf $(TARGET) toy *.o *.dSYM


.PHONY: test-run
test-run: $(TARGET) toy testcases/test-run
	./$< -q < testcases/test-run

.PHONY: test-cd
test-cd: $(TARGET) testcases/test-cd
	./$< -q < testcases/test-cd

.PHONY: test-aliases
test-aliases: $(TARGET) testcases/test-aliases
	./$< -q < testcases/test-aliases

.PHONY: test-pipe
test-pipe: $(TARGET) testcases/test-pipe
	./$< -q < testcases/test-pipe

.PHONY: test-all
test-all: $(TARGET) testcases/test-all
	./$< -q < testcases/test-all

test-all-case: test-run test-cd test-aliases test-pipe test-all
	echo
