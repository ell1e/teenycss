
MAIN_HEADER=./split-source/csslib.h
OTHER_HEADERS=$(filter-out $(MAIN_HEADER),$(wildcard ./split-source/*.h))
SOURCES=$(wildcard ./split-source/*.c)
TEST_BINS:=$(patsubst %.c, %.bin, $(wildcard ./tests/test_*.c))

.PHONY: assemble tests


assemble:
	echo "#ifndef TEENYCSS_H_" > .begin
	echo "#define TEENYCSS_H_" >> .begin
	echo "#endif  // TEENYCSS_H_" > .end
	echo "#ifdef TEENYCSS_IMPLEMENTATION" > .begin_impl
	echo "#endif  // TEENYCSS_IMPLEMENTATION" > .end_impl
	cat .begin $(MAIN_HEADER) $(OTHER_HEADERS) .begin_impl vendor/siphash.c $(SOURCES) .end_impl .end > ./teenycss.h
	rm -f .begin .begin_impl .end .end_impl
clean:
	rm -f teeny_css.h $(TEST_BINS)
test: assemble $(TEST_BINS)
	for x in $(TEST_BINARIES); do echo "TEST: $$x"; ./$$x || { exit 1; }; done
	@echo "All tests were run."
test_%.bin: test_%.c
	$(CC) $(CFLAGS) -I. -pthread -o ./$(basename $@).bin $(basename $<).c -lcheck -lrt -lsubunit $(LDFLAGS)
