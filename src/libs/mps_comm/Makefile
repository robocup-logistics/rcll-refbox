#*****************************************************************************
#           Makefile Build System for Fawkes: mps_comm Library
#                            -------------------
#   Created on Thu Feb 14 09:30:32 2013
#   Copyright (C) 2013 by Tim Niemueller, AllemaniACs RoboCup Team
#                 2019 by Till Hofmann
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

LIBS_libmps_comm = stdc++ m llsfrbcore pthread
OBJS_libmps_comm = opcua/opc_utils.o opcua/machine.o opcua/base_station.o opcua/cap_station.o opcua/delivery_station.o opcua/ring_station.o opcua/storage_station.o time_utils.o machine_factory.o \
                   mockup/machine.o mockup/base_station.o
HDRS_libmps_comm = subscription_client.h opc_utils.h machine.h base_station.h cap_station.h delivery_station.h ring_station.h storage_station.h time_utils.h mps_io_mapping.h timeouts.h machine_factory.h

OBJS_all = $(OBJS_libmps_comm)

ifeq ($(HAVE_MPS_COMM),1)
  CFLAGS  += $(CFLAGS_MPS_COMM)
  LDFLAGS += $(LDFLAGS_MPS_COMM)
  LIBS_all = $(LIBDIR)/libmps_comm.so
else
  WARN_TARGETS += warning_libmps_comm
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
.PHONY: $(WARN_TARGETS)
warning_libmps_comm:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build libmps_comm$(TNORMAL) (freeopcua not found)"
endif

include $(BUILDSYSDIR)/base.mk
