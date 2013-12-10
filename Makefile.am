ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST =
GITIGNOREFILES =

# clang-plugin library
clangdir = $(libdir)/gnome-clang/$(LLVM_VERSION)/
clang_LTLIBRARIES = clang-plugin/libclang-gnome.la

clang_plugin_libclang_gnome_la_SOURCES = \
	clang-plugin/assertion-extracter.cpp \
	clang-plugin/debug.cpp \
	clang-plugin/plugin.cpp \
	clang-plugin/gir-attributes.cpp \
	clang-plugin/gir-manager.cpp \
	clang-plugin/gassert-attributes.cpp \
	clang-plugin/nullability-checker.cpp \
	$(NULL)

clang_plugin_libclang_gnome_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir) \
	$(DISABLE_DEPRECATED) \
	$(LLVM_CPPFLAGS) \
	$(NULL)

clang_plugin_libclang_gnome_la_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-std=c++0x -pedantic \
	$(GNOME_CLANG_CFLAGS) \
	$(LLVM_CXXFLAGS) \
	$(WARN_CXXFLAGS) \
	$(NULL)

clang_plugin_libclang_gnome_la_LIBADD = \
	$(AM_LIBADD) \
	$(GNOME_CLANG_LIBS) \
	$(LLVM_LIBS) \
	$(NULL)

clang_plugin_libclang_gnome_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	$(LLVM_LDFLAGS) \
	-avoid-version \
	-module \
	-no-undefined \
	$(NULL)

# Clang wrapper script
dist_bin_SCRIPTS = scripts/gnome-clang

# Website
EXTRA_DIST += \
	website/index.html \
	$(NULL)

# Cleaning
EXTRA_DIST += \
	autogen.sh \
	HACKING \
	gnome-clang.doap \
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
ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./build-aux/missing --run \
	   git log --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog

MAINTAINERCLEANFILES += ChangeLog

-include $(top_srcdir)/git.mk
