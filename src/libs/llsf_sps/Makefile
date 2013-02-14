#*****************************************************************************
#           Makefile Build System for Fawkes: llsf_sps Library
#                            -------------------
#   Created on Thu Feb 14 09:30:32 2013
#   Copyright (C) 2013 by Tim Niemueller, AllemaniACs RoboCup Team
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
include $(BUILDSYSDIR)/protobuf.mk
include $(BUILDSYSDIR)/boost.mk

LIBS_libllsf_sps = stdc++ m llsfrbcore
OBJS_libllsf_sps = sps_comm.o
HDRS_libllsf_sps = sps_comm.h

OBJS_all = $(OBJS_libllsf_sps)

ifeq ($(HAVE_LIBMODBUS),1)
  CFLAGS  += $(CFLAGS_LIBMODBUS)
  LDFLAGS += $(LDFLAGS_LIBMODBUS)

  LIBS_all  = $(LIBDIR)/libllsf_sps.so
else
  ifneq ($(HAVE_LIBMODBUS),1)
    WARN_TARGETS += warning_libmodbus
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS)
warning_libmodbus:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF SPS library$(TNORMAL) (libmodbus not found)"
endif

include $(BUILDSYSDIR)/base.mk

