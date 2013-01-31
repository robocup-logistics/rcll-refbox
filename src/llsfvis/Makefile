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
include $(BUILDSYSDIR)/gui.mk


CFLAGS  += $(CFLAGS_GUI)
LDFLAGS += $(LDFLAGS_GUI)



LIBS_llsfvis = stdc++ llsfrbcore
OBJS_llsfvis = llsfvis.o MainWindow.o PlayField.o


OBJS_all =	$(OBJS_llsfvis)
BINS_all =	$(BINDIR)/llsfvis

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
gui: $(WARN_TARGETS)
.PHONY: warning_libmodbus
warning_nolib:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox$(TNORMAL) (libmodbus not found)"
endif

include $(BUILDSYSDIR)/base.mk
