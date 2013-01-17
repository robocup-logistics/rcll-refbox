#*****************************************************************************
#                      Makefile for Fawkes Core Library
#                            -------------------
#   Created on Tue Sep 12 13:28:19 2006
#   Copyright (C) 2006-2008 by Tim Niemueller, AllemaniACs RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

BASEDIR = ../../..
include $(BASEDIR)/etc/buildsys/config.mk

LIBS_libllsfrbcore = pthread stdc++ m
ifneq ($(OS),Darwin)
  LIBS_libllsfrbcore += rt
endif
OBJS_libllsfrbcore = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp $(SRCDIR)/*/*/*.cpp)))))
HDRS_libllsfrbcore = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h $(SRCDIR)/*/*.h  $(SRCDIR)/*/*/*.h ))

OBJS_all = $(OBJS_libllsfrbcore)
LIBS_all = $(LIBDIR)/libllsfrbcore.so

ifeq ($(OS),FreeBSD)
  ifneq ($(wildcard /usr/local/include/execinfo.h),)
    LIBS_libllsfrbcore += execinfo
    CFLAGS += -DHAVE_EXECINFO
  else
    WARN_TARGETS += warn_libexecinfo
  endif
else
  CFLAGS += -DHAVE_EXECINFO
endif

ifeq ($(OBJSSUBMAKE),1)
  ifneq ($(ERROR_TARGETS)$(WARN_TARGETS),)
all: $(ERROR_TARGETS) $(WARN_TARGETS)
.PHONY: warn_libexecinfo
warn_libexecinfo:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)LLSF RefBox Core lib built w/o backtrace support$(TNORMAL) (FreeBSD: devel/libexecinfo not installed)"

  endif
endif

include $(BUILDSYSDIR)/base.mk

