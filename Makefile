
MAIN_HEADER=./split-source/csslib.h
OTHER_HEADERS=$(filter-out $(MAIN_HEADER),$(wildcard ./split-source/*.h))
SOURCES=$(wildcard ./split-source/*.c)
TESTS:=$(patsubst %.c, %.bin, $(wildcard ./tests/test_*.c))



blob:
	echo "HEADERS: $(MAIN_HEADER), $(OTHER_HEADERS) $(SOURCES)"
assemble:
	echo "#ifndef TEENYCSS_H_" > .begin
	echo "#define TEENYCSS_H_" >> .begin
	echo "#endif  // TEENYCSS_H_" > .end
	echo "#ifdef TEENYCSS_IMPLEMENTATION" > .begin_impl
	echo "#endif  // TEENYCSS_IMPLEMENTATION" > .end_impl
	cat .begin $(MAIN_HEADER) $(OTHER_HEADERS) .begin_impl $(SOURCES) .end_impl .end > ./teenycss.h
	rm -f .begin .begin_impl .end .end_impl
