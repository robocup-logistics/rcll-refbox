#*****************************************************************************
#             Makefile Build System for Fawkes : Tools
#                            -------------------
#   Created on Tue Feb 12 23:22:15 2013
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
include $(BUILDSYSDIR)/clips.mk
include $(BUILDSYSDIR)/boost.mk
include $(BUILDCONFDIR)/llsf_sps/llsf_sps.mk

CFLAGS += $(CFLAGS_CPP11)

REQ_BOOST_LIBS = thread asio system signals2
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_llsf_show_peers = stdc++ llsfrbcore llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_llsf_show_peers = llsf-show-peers.o

LIBS_llsf_puck = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_sps
OBJS_llsf_puck = llsf-puck.o

LIBS_llsf_fake_robot = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_llsf_fake_robot = llsf-fake-robot.o

LIBS_llsf_signal_ctrl = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_sps
OBJS_llsf_signal_ctrl = llsf-signal-ctrl.o

LIBS_llsf_report_machine = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_llsf_report_machine = llsf-report-machine.o

LIBS_rcll_prepare_machine = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_rcll_prepare_machine = rcll-prepare-machine.o

LIBS_rcll_set_machine_state = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_rcll_set_machine_state = rcll-set-machine-state.o

LIBS_rcll_machine_add_base = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_rcll_machine_add_base = rcll-machine-add-base.o

LIBS_rcll_set_machine_lights = stdc++ llsfrbcore llsfrbutils llsfrbconfig llsf_protobuf_comm llsf_msgs
OBJS_rcll_set_machine_lights = rcll-set-machine-lights.o

ifeq ($(HAVE_PROTOBUF)$(HAVE_BOOST_LIBS),11)
  OBJS_all += $(OBJS_llsf_show_peers) $(OBJS_llsf_fake_robot) $(OBJS_llsf_report_machine) \
	      $(OBJS_rcll_prepare_machine) $(OBJS_rcll_set_machine_state) \
	      $(OBJS_rcll_machine_add_base)
  BINS_all += $(BINDIR)/llsf-show-peers $(BINDIR)/llsf-fake-robot \
	      $(BINDIR)/llsf-report-machine $(BINDIR)/rcll-prepare-machine \
	      $(BINDIR)/rcll-set-machine-state \
	      $(BINDIR)/rcll-set-machine-lights \
	      $(BINDIR)/rcll-machine-add-base

  CFLAGS_llsf_show_peers  += $(CFLAGS_PROTOBUF) \
	     		     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_llsf_show_peers += $(LDFLAGS_PROTOBUF) \
	     		     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_llsf_fake_robot  += $(CFLAGS_PROTOBUF) \
	     		     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_llsf_fake_robot += $(LDFLAGS_PROTOBUF) \
	     		     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_llsf_report_machine  += $(CFLAGS_PROTOBUF) \
	     		         $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_llsf_report_machine += $(LDFLAGS_PROTOBUF) \
	     		         $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_rcll_prepare_machine  += $(CFLAGS_PROTOBUF) \
	     		         $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_rcll_prepare_machine += $(LDFLAGS_PROTOBUF) \
	     		         $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_rcll_set_machine_state  += $(CFLAGS_PROTOBUF) \
	     		         $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_rcll_set_machine_state += $(LDFLAGS_PROTOBUF) \
	     		         $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_rcll_set_machine_lights  += $(CFLAGS_PROTOBUF) \
	     		         $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_rcll_set_machine_lights += $(LDFLAGS_PROTOBUF) \
	     		         $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  CFLAGS_rcll_machine_add_base  += $(CFLAGS_PROTOBUF) \
	     		         $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS_rcll_machine_add_base += $(LDFLAGS_PROTOBUF) \
	     		         $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  #MANPAGES_all =  $(MANDIR)/man1/refbox-llsf.1
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

ifeq ($(HAVE_LIBMODBUS),1)
  OBJS_all +=	$(OBJS_llsf_puck) $(OBJS_llsf_signal_ctrl)
  BINS_all +=	$(BINDIR)/llsf-puck $(BINDIR)/llsf-signal-ctrl

  CFLAGS  += $(CFLAGS_PROTOBUF) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))
  #MANPAGES_all =  $(MANDIR)/man1/refbox-llsf.1
else
  ifneq ($(HAVE_LIBMODBUS),1)
    WARN_TARGETS += warning_libmodbus
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: warning_libmodbus
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf_comm library$(TNORMAL) (Boost library $* not found)"
warning_libmodbus:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF Puck Tool$(TNORMAL) (libmodbus not found)"
endif

include $(BUILDSYSDIR)/base.mk
