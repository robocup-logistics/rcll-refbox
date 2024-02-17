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
include $(BASEDIR)/src/libs/mps_comm/freeopcua.mk

LIBS_libmps_comm = stdc++ m llsfrbcore llsfrbconfig pthread
OBJS_opcua = opcua/opc_utils.o opcua/machine.o opcua/base_station.o \
						 opcua/cap_station.o opcua/delivery_station.o opcua/ring_station.o \
             opcua/storage_station.o
OBJS_mockup = mockup/machine.o mockup/base_station.o mockup/cap_station.o \
							mockup/delivery_station.o mockup/ring_station.o \
              mockup/storage_station.o
OBJS_mqtt = mqtt/machine.o mqtt/base_station.o mqtt/mqtt_utils.o mqtt/mqtt_callback.o mqtt/mqtt_action_listener.o \
            mqtt/mqtt_client_wrapper.o mqtt/cap_station.o mqtt/delivery_station.o mqtt/ring_station.o mqtt/storage_station.o
OBJS_libmps_comm = time_utils.o machine_factory.o

ifeq ($(HAVE_CPP17),1)
  OBJS_libmps_comm += $(OBJS_mockup)
  CFLAGS += $(CFLAGS_CPP17) -DHAVE_MOCKUP
else
  WARN_TARGETS += warning_cpp17
endif

HAVE_MQTT = $(shell if [ -f /usr/include/mqtt/async_client.h ]; then echo 1; else echo 0; fi)
ifeq ($(HAVE_MQTT),1)
  LIBS_libmps_comm += paho-mqtt3cs paho-mqtt3c paho-mqtt3as paho-mqtt3a paho-mqttpp3
  OBJS_libmps_comm += $(OBJS_mqtt)
  CFLAGS += $(CFLAGS_CPP17) -DHAVE_MQTT
else
  WARN_TARGETS += warning_mqtt
endif


ifeq ($(HAVE_FREEOPCUA),1)
  OBJS_libmps_comm += $(OBJS_opcua)
  CFLAGS  += $(CFLAGS_FREEOPCUA) -DHAVE_FREEOPCUA
  LDFLAGS += $(LDFLAGS_FREEOPCUA)
else
  WARN_TARGETS += warning_freeopcua
endif

OBJS_all = $(OBJS_libmps_comm)
LIBS_all = $(LIBDIR)/libmps_comm.so

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
.PHONY: $(WARN_TARGETS)
warning_freeopcua:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)No OPC-UA support$(TNORMAL) ($(WARNING_FREEOPCUA))"
warning_mqtt:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)No mqtt support$(TNORMAL) (paho-c(-devel) and paho-mqtt-cpp(-devel) required)"
warning_cpp17:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)No Mockup support$(TNORMAL) (C++17 not supported)"
endif

include $(BUILDSYSDIR)/base.mk
