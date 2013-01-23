#*****************************************************************************
#             Makefile Build System for Fawkes : Main Program
#                            -------------------
#   Created on Wed Jan 16 17:43:33 2013
#   Copyright (C) 2006-2008 by Tim Niemueller, Carologistics RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

BASEDIR = ../..

include $(BASEDIR)/etc/buildsys/config.mk
include $(BUILDSYSDIR)/protobuf.mk

ifneq ($(PKGCONFIG),)
  HAVE_LIBMODBUS = $(if $(shell $(PKGCONFIG) --exists 'libmodbus'; echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_LIBMODBUS),1)
  CFLAGS_LIBMODBUS  = $(shell $(PKGCONFIG) --cflags 'libmodbus')
  LDFLAGS_LIBMODBUS = $(shell $(PKGCONFIG) --libs 'libmodbus')
endif

LIBS_refbox_llsf = stdc++ llsfrbcore
OBJS_refbox_llsf = main.o sps_comm.o

ifeq ($(HAVE_PROTOBUF)$(HAVE_LIBMODBUS),11)
  OBJS_all =	$(OBJS_refbox_llsf)
  BINS_all =	$(BINDIR)/refbox-llsf

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_LIBMODBUS)
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(LDFLAGS_LIBMODBUS)
  #MANPAGES_all =  $(MANDIR)/man1/refbox-llsf.1
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_LIBMODBUS),1)
    WARN_TARGETS += warning_libmodbus
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
.PHONY: warning_libmodbus
warning_nolib:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox$(TNORMAL) (libmodbus not found)"
endif

include $(BUILDSYSDIR)/base.mk
