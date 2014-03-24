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

ifeq ($(CC),gcc)
  ifneq ($(call gcc_atleast_version,4,6),1)
    GCC_TOO_OLD=1
  endif
endif

ifneq ($(GCC_TOO_OLD),1)
  ifeq ($(HAVE_CPP11)$(HAVE_LIBMODBUS),11)
    CFLAGS  += $(CFLAGS_LIBMODBUS) $(CFLAGS_CPP11)
    LDFLAGS += $(LDFLAGS_LIBMODBUS)

    LIBS_all  = $(LIBDIR)/libllsf_sps.so
  else
    ifneq ($(HAVE_LIBMODBUS),1)
      WARN_TARGETS += warning_libmodbus
    endif
    ifneq ($(HAVE_CPP11),1)
      WARN_TARGETS += warning_cpp11
    endif
  endif
else
      WARN_TARGETS += warning_old_gcc
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS)
warning_libmodbus:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF SPS library$(TNORMAL) (libmodbus not found)"
warning_cpp11:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF SPS library$(TNORMAL) (compiler has no C++11 support)"
warning_old_gcc:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF SPS library$(TNORMAL) (GCC too old, have $(GCC_VERSION), required 4.6)"
endif

include $(BUILDSYSDIR)/base.mk

