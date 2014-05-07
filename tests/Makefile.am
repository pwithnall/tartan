# Tests are implemented as a series of example C files, each of which should
# compile successfully without Tartan, and which should fail compilation
# with Tartan (and -Werror).

TEST_EXTENSIONS = .c
C_LOG_COMPILER = $(top_srcdir)/tests/wrapper-compiler-errors

c_tests = \
	assertion-extraction.c \
	assertion-extraction-return.c \
	gsignal-connect.c \
	gvariant-builder.c \
	gvariant-get.c \
	gvariant-get-child.c \
	gvariant-iter.c \
	gvariant-lookup.c \
	gvariant-new.c \
	nonnull.c \
	$(NULL)

templates = \
	assertion.head.c \
	assertion.tail.c \
	assertion-return.head.c \
	assertion-return.tail.c \
	generic.head.c \
	generic.tail.c \
	gsignal.head.c \
	gsignal.tail.c \
	gvariant.head.c \
	gvariant.tail.c \
	$(NULL)

TESTS = $(c_tests)
EXTRA_DIST = \
	$(templates) \
	$(c_tests) \
	wrapper-compiler-errors \
	$(NULL)

-include $(top_srcdir)/git.mk
