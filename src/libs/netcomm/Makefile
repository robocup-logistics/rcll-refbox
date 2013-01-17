#*****************************************************************************
#               Makefile Build System for Fawkes: NetComm Library
#                            -------------------
#   Created on Tue Nov 07 16:43:45 2006
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

ifneq ($(HAVE_AVAHI),1)
  WARN_TARGETS += warn_avahi
  OMIT_OBJECTS += dns-sd/%
else
  CFLAGS += $(CFLAGS_AVAHI)
  LDFLAGS_libllsfrbnetcomm += $(LDFLAGS_AVAHI)
endif

LIBS_libllsfrbnetcomm = stdc++ m llsfrbcore llsfrbutils
OBJS_libllsfrbnetcomm = $(filter-out $(OMIT_OBJECTS),$(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp $(SRCDIR)/*/*/*.cpp))))))
HDRS_libllsfrbnetcomm = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h $(SRCDIR)/*/*.h $(SRCDIR)/*/*/*.h))

OBJS_all = $(OBJS_libllsfrbnetcomm)
LIBS_all  = $(LIBDIR)/libllsfrbnetcomm.so

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(ERROR_TARGETS)
.PHONY: warn_avahi
warn_avahi:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting mDNS/DNS-SD support$(TNORMAL) (Avahi not installed)"

endif

include $(BUILDSYSDIR)/base.mk

