AC_PREREQ(2.65)

# gnome-clang release version
m4_define(gnome_clang_version_major, 0)
m4_define(gnome_clang_version_minor, 2)
m4_define(gnome_clang_version_micro, 0)

AC_INIT([gnome-clang],[gnome_clang_version_major.gnome_clang_version_minor.gnome_clang_version_micro],
        [http://people.collabora.com/~pwith/gnome-clang/],[gnome-clang],
        [http://people.collabora.com/~pwith/gnome-clang/])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([clang-plugin/plugin.cpp])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.12 dist-xz no-dist-gzip check-news subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AM_PROG_CC_C_O
LT_INIT([])
PKG_PROG_PKG_CONFIG

# Requirements
LLVM_REQS=3.4
GLIB_REQS=2.38  # TODO
GIO_REQS=2.38  # TODO
GIR_REQS=1.38.0  # TODO

# gnome-clang versioning
AC_SUBST([GNOME_CLANG_VERSION_MAJOR],gnome_clang_version_major)
AC_SUBST([GNOME_CLANG_VERSION_MINOR],gnome_clang_version_minor)
AC_SUBST([GNOME_CLANG_VERSION_MICRO],gnome_clang_version_micro)

# gnome-clang pkg-config dependencies
GNOME_CLANG_PACKAGES="gobject-2.0 glib-2.0 >= $GLIB_REQS gio-2.0 >= $GIO_REQS \
                      gobject-introspection-1.0 >= $GIR_REQS"
AC_SUBST([GNOME_CLANG_PACKAGES])

PKG_CHECK_MODULES([GNOME_CLANG],[$GNOME_CLANG_PACKAGES])

# gnome-clang LLVM dependency
AC_PATH_PROG([LLVM_CONFIG],[llvm-config],"failed")
AS_IF([test $LLVM_CONFIG = "failed"],[
	AC_MSG_ERROR([LLVM version $LLVM_REQS or later needed.])
])

AC_MSG_CHECKING([for LLVM])

# LLVM_REQS is either (e.g.) 3.5 or 3.6svn
req_major=`echo $LLVM_REQS | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(svn\)\?/\1/'`
req_minor=`echo $LLVM_REQS | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(svn\)\?/\2/'`

llvm_version=`$LLVM_CONFIG --version`

major=`echo $llvm_version | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(svn\)\?/\1/'`
minor=`echo $llvm_version | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\(svn\)\?/\2/'`

AS_IF([test "$major" -eq "$req_major" -a "$minor" -ge "$req_minor"],[
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

# General GNOME macros
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS([yes])
GNOME_MAINTAINER_MODE_DEFINES

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
po/Makefile.in
])
AC_OUTPUT
