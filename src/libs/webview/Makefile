#*****************************************************************************
#              Makefile Build System for Fawkes: Webview Library
#                            -------------------
#   Created on Sun Aug 30 14:26:04 2009
#   Copyright (C) 2006-2009 by Tim Niemueller, AllemaniACs RoboCup Team
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

LDFLAGS += $(LDFLAGS_LIBMICROHTTPD)
CFLAGS  += $(CFLAGS_LIBMICROHTTPD)
LIBS_libllsfrbwebview = stdc++ llsfrbcore llsfrbutils llsfrblogging
OBJS_libllsfrbwebview = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp)))))
HDRS_libllsfrbwebview = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h $(SRCDIR)/*/*.h))


ifeq ($(HAVE_LIBMICROHTTPD),1)
  OBJS_all = $(OBJS_libllsfrbwebview)
  LIBS_all = $(LIBDIR)/libllsfrbwebview.so
else
  WARN_TARGETS += warning_libmicrohttpd
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)

.PHONY: warning_libmicrohttpd warning_rapidjson
warning_libmicrohttpd:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting Webview Library$(TNORMAL) (libmicrohttpd not found)"
warning_rapidjson:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting Webview Library$(TNORMAL) (RapidJSON not found)"
endif

include $(BUILDSYSDIR)/base.mk

