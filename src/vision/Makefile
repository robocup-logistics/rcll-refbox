#*****************************************************************************
#            Makefile Build System for Fawkes : Vision Processor
#                            -------------------
#   Created on Fri Jun 07 14:22:08 2013
#   Copyright (C) 2013 by Tim Niemueller, Carologistics RoboCup Team
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
include $(BUILDSYSDIR)/boost.mk

PRESUBDIRS = ssl_msgs
CFLAGS += $(CFLAGS_CPP11)

REQ_BOOST_LIBS = thread asio system signals2 format date_time
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_llsf_vision_processor = m stdc++ llsfrbcore llsfrbconfig llsf_protobuf_comm \
			 llsf_msgs ssl_msgs
OBJS_llsf_vision_processor = main.o visproc.o robocup_ssl_client.o netraw.o

ifeq ($(HAVE_PROTOBUF)$(HAVE_BOOST_LIBS),11)
  OBJS_all =	$(OBJS_llsf_vision_proc)
  BINS_all =	$(BINDIR)/llsf-vision-processor

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_NCURSES) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(LDFLAGS_NCURSES) \
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

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS) $(WARN_TARGETS_BOOST) warning_ncurses
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox vision processor$(TNORMAL) (Boost library $* not found)"
endif

include $(BUILDSYSDIR)/base.mk
