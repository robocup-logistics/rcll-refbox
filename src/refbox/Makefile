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
include $(BUILDSYSDIR)/clips.mk
include $(BUILDSYSDIR)/boost.mk
include $(BUILDCONFDIR)/mongodb_log/mongodb.mk
include $(BUILDCONFDIR)/netcomm/netcomm.mk

CFLAGS += -Wno-deprecated-declarations

REQ_BOOST_LIBS = thread asio system signals2
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_llsf_refbox = stdc++ stdc++fs llsfrbcore llsfrbconfig llsfrblogging llsfrbnetcomm \
		   llsfrbutils llsf_protobuf_comm llsf_protobuf_clips mps_comm \
		   llsf_mps_placing_clips llsfrbrestapi

OBJS_llsf_refbox = main.o refbox.o clips_logger.o

ifeq ($(HAVE_CPP17)$(HAVE_PROTOBUF)$(HAVE_CLIPS)$(HAVE_BOOST_LIBS),1111)
  OBJS_all =	$(OBJS_llsf_refbox)
  BINS_all =	$(BINDIR)/llsf-refbox

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_CLIPS) $(CFLAGS_CPP17) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(LDFLAGS_CLIPS) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))
  #MANPAGES_all =  $(MANDIR)/man1/llsf-refbox.1

  ifeq ($(HAVE_AVAHI),1)
    CFLAGS  += $(CFLAGS_AVAHI)
    LDFLAGS += $(LDFLAGS_AVAHI)
  endif

  ifeq ($(HAVE_MONGODB),1)
    CFLAGS += $(CFLAGS_MONGODB)
    LDFLAGS += $(LDFLAGS_MONGODB)
    LIBS_llsf_refbox += llsf_mongodb_log
  else
    WARN_TARGETS += warning_mongodb
  endif
else
  ifneq ($(HAVE_CPP17),1)
    WARN_TARGETS += warning_cpp17
  endif
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_CLIPS),1)
    WARN_TARGETS += warning_clips
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
  ifneq ($(HAVE_MONGODB),1)
    WARN_TARGETS += warning_mongodb
  endif
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: $(WARN_TARGETS)
warning_cpp17:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build RCLL RefBox$(TNORMAL) (C++17 not supported)"
warning_protobuf:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build RCLL RefBox$(TNORMAL) (protobuf not found)"
warning_clips:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build RCLL RefBox$(TNORMAL) (clipsmm not found)"
warning_mongodb:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting MongoDB logging support$(TNORMAL) (MongoDB not found)"
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf_comm library$(TNORMAL) (Boost library $* not found)"

endif

include $(BUILDSYSDIR)/base.mk
