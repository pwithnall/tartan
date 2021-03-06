AC_PREREQ(2.65)

# Tartan release version
m4_define(tartan_version_major, 0)
m4_define(tartan_version_minor, 4)
m4_define(tartan_version_micro, 0)

AC_INIT([Tartan],
        [tartan_version_major.tartan_version_minor.tartan_version_micro],
        [https://phabricator.freedesktop.org/maniphest/task/edit/form/1/?tags=tartan],
        [tartan],[http://www.freedesktop.org/software/tartan/])
AX_IS_RELEASE([git-directory])

# Debugging
AX_CHECK_ENABLE_DEBUG([no])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([clang-plugin/plugin.cpp])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.12 dist-xz no-dist-gzip check-news subdir-objects color-tests parallel-tests])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AM_PROG_CC_C_O
AX_CXX_COMPILE_STDCXX_11()
LT_INIT([])
PKG_PROG_PKG_CONFIG

# Requirements
LLVM_REQS=3.5
GLIB_REQS=2.38  # TODO
GIO_REQS=2.38  # TODO
GIR_REQS=1.38.0  # TODO

# Tartan versioning
AC_SUBST([TARTAN_VERSION_MAJOR],tartan_version_major)
AC_SUBST([TARTAN_VERSION_MINOR],tartan_version_minor)
AC_SUBST([TARTAN_VERSION_MICRO],tartan_version_micro)

# Tartan pkg-config dependencies
TARTAN_PACKAGES="gobject-2.0 glib-2.0 >= $GLIB_REQS gio-2.0 >= $GIO_REQS \
                 gobject-introspection-1.0 >= $GIR_REQS"
AC_SUBST([TARTAN_PACKAGES])

PKG_CHECK_MODULES([TARTAN],[$TARTAN_PACKAGES])

# Tartan LLVM dependency
AC_PATH_PROG([LLVM_CONFIG],[llvm-config],"failed")
AS_IF([test $LLVM_CONFIG = "failed"],[
	AC_MSG_ERROR([LLVM version $LLVM_REQS or later needed.])
])

AC_MSG_CHECKING([for LLVM])

# LLVM_REQS is either (e.g.) 3.5, 3.6svn or 3.4.2
req_major=`echo $LLVM_REQS | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(\.[[0-9]]*\)\?\(svn\)\?/\1/'`
req_minor=`echo $LLVM_REQS | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(\.[[0-9]]*\)\?\(svn\)\?/\2/'`

llvm_version=`$LLVM_CONFIG --version`

major=`echo $llvm_version | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(\.[[0-9]]*\)\?\(svn\)\?/\1/'`
minor=`echo $llvm_version | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(\.[[0-9]]*\)\?\(svn\)\?/\2/'`

AS_IF([test "$major" -gt "$req_major" -o \( "$major" -eq "$req_major" -a "$minor" -ge "$req_minor" \)],[
	LLVM_CPPFLAGS=`$LLVM_CONFIG --cppflags`
	LLVM_CXXFLAGS=`$LLVM_CONFIG --cxxflags`
	LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags`
	# Don't link against the LLVM libs because they're huge. All the symbols
	# will be available when the plugin is loaded anyway.
	#LLVM_LIBS=`$LLVM_CONFIG --libs`
	LLVM_LIBS=''
	LLVM_VERSION="$major.$minor"  # don’t include the ‘svn’ suffix
	AC_MSG_RESULT([yes])
],[
	AC_MSG_ERROR([LLVM version $LLVM_REQS or later needed.])
])

AC_SUBST([LLVM_CPPFLAGS])
AC_SUBST([LLVM_CXXFLAGS])
AC_SUBST([LLVM_LDFLAGS])
AC_SUBST([LLVM_LIBS])
AC_SUBST([LLVM_VERSION])

AC_DEFINE_UNQUOTED([LLVM_CONFIG_VERSION],"$llvm_version",
                   [Configured LLVM version])

# LLVM-specific version definitions
AS_IF([test "$major" -gt 3 -o "$major" -eq 3 -a "$minor" -ge 5],[
	AC_DEFINE([HAVE_LLVM_3_5], [1], [Whether LLVM ≥ 3.5 is available])
])
AS_IF([test "$major" -gt 3 -o "$major" -eq 3 -a "$minor" -ge 6],[
	AC_DEFINE([HAVE_LLVM_3_6], [1], [Whether LLVM ≥ 3.6 is available])
])
AS_IF([test "$major" -gt 3 -o "$major" -eq 3 -a "$minor" -ge 7],[
	AC_DEFINE([HAVE_LLVM_3_7], [1], [Whether LLVM ≥ 3.7 is available])
])
AS_IF([test "$major" -gt 3 -o "$major" -eq 3 -a "$minor" -ge 8],[
	AC_DEFINE([HAVE_LLVM_3_8], [1], [Whether LLVM ≥ 3.8 is available])
])
AS_IF([test "$major" -gt 3 -o "$major" -eq 3 -a "$minor" -ge 9],[
	AC_DEFINE([HAVE_LLVM_3_9], [1], [Whether LLVM ≥ 3.9 is available])
])
AS_IF([test "$major" -gt 4 -o "$major" -eq 4 -a "$minor" -ge 0],[
	AC_DEFINE([HAVE_LLVM_4_0], [1], [Whether LLVM ≥ 4.0 is available])
])

# Clang dependency (e.g. the clang-devel package on Fedora)
AC_LANG_PUSH([C++])
old_cppflags="$CPPFLAGS"
CPPFLAGS="$LLVM_CPPFLAGS $CPPFLAGS"
AC_CHECK_HEADER([clang/AST/Expr.h],[],[
	AC_MSG_ERROR([Clang headers version $LLVM_REQS or later needed.])
])
CPPFLAGS="$old_cppflags"
AC_LANG_POP([C++])

# Internationalisation
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],"$GETTEXT_PACKAGE",
                   [The name of the gettext domain])
IT_PROG_INTLTOOL([0.40.0])

# Code coverage for unit tests
AX_CODE_COVERAGE

# Compiler warnings
AX_COMPILER_FLAGS

# ChangeLog generation
AX_GENERATE_CHANGELOG

AC_CONFIG_FILES([
Makefile
po/Makefile.in
tests/Makefile
])
AC_OUTPUT
