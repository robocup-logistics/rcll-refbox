#*****************************************************************************
#             Makefile Build System for Fawkes : LLSFVis
#                            -------------------
#   Created on Wed Jan 16 17:43:33 2013
#   Copyright (C) 2006-2008 by Tim Niemueller, Daniel Ewert, Carologistics RoboCup Team
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
include $(BUILDSYSDIR)/boost.mk

CFLAGS += $(CFLAGS_CPP11) -I/usr/include/webkitgtk-3.0 -I/usr/include/libsoup-2.4/

REQ_BOOST_LIBS = thread asio system signals2 format date_time
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_llsfvis = stdc++ llsfrbcore llsf_protobuf_comm llsf_msgs llsf_log_msgs webkitgtk-3.0 m
OBJS_llsfvis = llsfvis.o MainWindow.o PlayFieldWidget.o StateWidget.o \
    RobotStatesWidget.o PuckDialog.o LogWidget.o RefboxClient.o PlacePuckDialog.o \
    PucksWidget.o PuckStateWidget.o OrdersWidget.o OrderWidget.o HTMLWidget.o


ifeq ($(HAVE_GTKMM_3)$(HAVE_BOOST_LIBS)$(HAVE_PROTOBUF),111)
  OBJS_all =	$(OBJS_llsfvis)
  BINS_all =	$(BINDIR)/llsfvis
  
  CFLAGS  += $(CFLAGS_PROTOBUF)$(CFLAGS_GUI)\
  	$(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF)$(LDFLAGS_GUI)\
  	$(call boost-libs-ldflags,$(REQ_BOOST_LIBS))
  
else
   ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
   endif
   ifneq ($(HAVE_GTKMM_3),1)
    WARN_TARGETS += warning_nogtkmm
   endif
   ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
   endif
endif
ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS)
gui: $(WARN_TARGETS)
.PHONY: warning_nogtkmm
warning_nogtkmm:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSFVis$(TNORMAL) (gtkmm-3.0 not found)"
	$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf_comm library$(TNORMAL) (Boost library $* not found)"
endif

include $(BUILDSYSDIR)/base.mk
