# Tests are implemented as a series of example C files, each of which should
# compile successfully without gnome-clang, and which should fail compilation
# with gnome-clang (and -Werror).

TEST_EXTENSIONS = .c
C_LOG_COMPILER = ./wrapper-compiler-errors

TESTS = \
	gvariant-new.c

-include $(top_srcdir)/git.mk
