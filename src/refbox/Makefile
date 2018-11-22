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
include $(BUILDCONFDIR)/llsf_sps/llsf_sps.mk
include $(BUILDCONFDIR)/mongodb_log/mongodb.mk
include $(BUILDCONFDIR)/netcomm/netcomm.mk

CFLAGS += $(CFLAGS_CPP11) -Wno-deprecated-declarations

REQ_BOOST_LIBS = thread asio system signals2
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

LIBS_llsf_refbox = stdc++ llsfrbcore llsfrbconfig llsfrblogging llsfrbnetcomm \
		   llsfrbutils llsf_protobuf_comm llsf_protobuf_clips llsf_sps llsf_mps_placing_clips
OBJS_llsf_refbox = main.o refbox.o clips_logger.o

ifeq ($(HAVE_PROTOBUF)$(HAVE_LIBMODBUS)$(HAVE_CLIPS)$(HAVE_BOOST_LIBS),1111)
  OBJS_all =	$(OBJS_llsf_refbox)
  BINS_all =	$(BINDIR)/llsf-refbox

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_LIBMODBUS) $(CFLAGS_CLIPS) $(CFLAGS_MONGODB) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(LDFLAGS_LIBMODBUS) $(LDFLAGS_CLIPS) $(LDFLAGS_MONGODB) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))
  #MANPAGES_all =  $(MANDIR)/man1/llsf-refbox.1

  ifeq ($(HAVE_AVAHI),1)
    CFLAGS  += $(CFLAGS_AVAHI)
    LDFLAGS += $(LDFLAGS_AVAHI)
  endif

  ifeq ($(HAVE_MONGODB),1)
    LIBS_llsf_refbox += llsf_mongodb_log
  endif
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_LIBMODBUS),1)
    WARN_TARGETS += warning_libmodbus
  endif
  ifneq ($(HAVE_CLIPS),1)
    WARN_TARGETS += warning_clips
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

ifneq ($(HAVE_MONGODB),1)
  WARN_TARGETS += warning_mongodb
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
.PHONY: warning_libmodbus warning_protobuf warning_clips warning_mongodb $(WARN_TARGETS_BOOST)
warning_libmodbus:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox$(TNORMAL) (libmodbus not found)"
warning_protobuf:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox$(TNORMAL) (protobuf not found)"
warning_clips:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build LLSF RefBox$(TNORMAL) (clipsmm not found)"
warning_mongodb:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting MongoDB logging support$(TNORMAL) (MongoDB not found)"
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build protobuf_comm library$(TNORMAL) (Boost library $* not found)"

endif

include $(BUILDSYSDIR)/base.mk
