# Tests are implemented as a series of example C files, each of which should
# compile successfully without gnome-clang, and which should fail compilation
# with gnome-clang (and -Werror).

TEST_EXTENSIONS = .c
C_LOG_COMPILER = ./wrapper-compiler-errors

TESTS = \
	gvariant-builder.c \
	gvariant-get.c \
	gvariant-get-child.c \
	gvariant-iter.c \
	gvariant-lookup.c \
	gvariant-new.c \
	nonnull.c \
	$(NULL)

templates = \
	generic.head.c \
	generic.tail.c \
	gvariant.head.c \
	gvariant.tail.c \
	$(NULL)

EXTRA_DIST = $(templates)

-include $(top_srcdir)/git.mk
