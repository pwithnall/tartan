SUBDIRS = . po tests

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST =
GITIGNOREFILES =

# clang-plugin library
clangdir = $(libdir)/tartan/$(LLVM_VERSION)/
clang_LTLIBRARIES = clang-plugin/libtartan.la

clang_plugin_libtartan_la_SOURCES = \
	clang-plugin/assertion-extracter.cpp \
	clang-plugin/assertion-extracter.h \
	clang-plugin/debug.cpp \
	clang-plugin/debug.h \
	clang-plugin/plugin.cpp \
	clang-plugin/gerror-checker.cpp \
	clang-plugin/gerror-checker.h \
	clang-plugin/gir-attributes.cpp \
	clang-plugin/gir-attributes.h \
	clang-plugin/gir-manager.cpp \
	clang-plugin/gir-manager.h \
	clang-plugin/gassert-attributes.cpp \
	clang-plugin/gassert-attributes.h \
	clang-plugin/gsignal-checker.cpp \
	clang-plugin/gsignal-checker.h \
	clang-plugin/gvariant-checker.cpp \
	clang-plugin/gvariant-checker.h \
	clang-plugin/nullability-checker.cpp \
	clang-plugin/nullability-checker.h \
	clang-plugin/checker.cpp \
	clang-plugin/checker.h \
	clang-plugin/type-manager.cpp \
	clang-plugin/type-manager.h \
	$(NULL)

clang_plugin_libtartan_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"tartan\" \
	$(DISABLE_DEPRECATED) \
	$(LLVM_CPPFLAGS) \
	$(NULL)

clang_plugin_libtartan_la_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-std=c++0x -pedantic \
	$(CODE_COVERAGE_CFLAGS) \
	$(TARTAN_CFLAGS) \
	$(LLVM_CXXFLAGS) \
	$(WARN_CXXFLAGS) \
	$(NULL)

clang_plugin_libtartan_la_LIBADD = \
	$(AM_LIBADD) \
	$(CODE_COVERAGE_LDFLAGS) \
	$(TARTAN_LIBS) \
	$(LLVM_LIBS) \
	$(NULL)

clang_plugin_libtartan_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	$(LLVM_LDFLAGS) \
	$(WARN_LDFLAGS) \
	-avoid-version \
	-module \
	-no-undefined \
	$(NULL)

# Clang and scan-build wrapper scripts
dist_bin_SCRIPTS = \
	scripts/tartan \
	scripts/tartan-build \
	$(NULL)

# Code coverage
@CODE_COVERAGE_RULES@
CODE_COVERAGE_IGNORE_PATTERN = \
	"include/clang/*" \
	"include/llvm/*" \
	"include/c++/*" \
	$(NULL)

# Cleaning
EXTRA_DIST += \
	autogen.sh \
	HACKING \
	tartan.doap \
	$(NULL)

CLEANFILES += \
	`find "$(srcdir)" -type f -name .dirstamp -print` \
	$(NULL)

MAINTAINERCLEANFILES += \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	build-aux/ \
	ABOUT-NLS \
	INSTALL \
	omf.make \
	config.rpath \
	`find "m4" -type f -name "*.m4" -print` \
	$(NULL)

GITIGNOREFILES += \
	.be/id-cache \
	$(NULL)

# ChangeLog
@GENERATE_CHANGELOG_RULES@
dist-book: dist-ChangeLog

-include $(top_srcdir)/git.mk
