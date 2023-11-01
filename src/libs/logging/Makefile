#*****************************************************************************
#           Makefile Build System for LLSF RefBox: Logging Utils
#                            -------------------
#   Created on Sat May 28 00:16:57 2011
#   Copyright (C) 2006-2011 by Tim Niemueller, AllemaniACs RoboCup Team
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
include $(BUILDCONFDIR)/websocket/websocket.mk
include $(BUILDSYSDIR)/clips.mk


REQ_BOOST_LIBS = system asio
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))
HAVE_BOOST_LIBS_OPT = $(call boost-have-libs,$(OPT_BOOST_LIBS))

OBJS_WEBSOCKET = websocket.o
HDRS_WEBSOCKET = websocket.h

PRESUBDIRS = llsf_log_msgs
CFLAGS += $(CFLAGS_CPP11)

ifeq ($(HAVE_PROTOBUF_COMM)$(HAVE_PROTOBUF)$(HAVE_BOOST_LIBS),111)

  CFLAGS  += $(CFLAGS_PROTOBUF) $(CFLAGS_PROTOBUF_COMM) \
	     $(call boost-libs-cflags,$(REQ_BOOST_LIBS))
  LDFLAGS += $(LDFLAGS_PROTOBUF) $(LDFLAGS_PROTOBUF_COMM) \
	     $(call boost-libs-ldflags,$(REQ_BOOST_LIBS))

  LIBS_libllsfrblogging = stdc++ llsfrbcore llsf_log_msgs
  OBJS_libllsfrblogging = $(filter-out $(OBJS_WEBSOCKET), $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp))))))
  HDRS_libllsfrblogging = $(filter-out $(HDRS_WEBSOCKET), $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h)))

  ifeq ($(HAVE_WEBSOCKET),1)
    CFLAGS += $(CFLAGS_WEBSOCKET) $(CFLAGS_CLIPS)
    LDFLAGS += $(LDFLAGS_WEBSOCKET) $(LDFLAGS_CLIPS)

    LIBS_libllsfrblogging += $(LIBS_WEBSOCKET)
    OBJS_libllsfrblogging += $(OBJS_WEBSOCKET)
    HDRS_libllsfrblogging += $(HDRS_WEBSOCKET)
  else
    WARN_TARGETS_WEBSOCKET = $(foreach l,$(BOOST_LIBS_WEBSOCKET),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif

  LIBS_all  = $(LIBDIR)/libllsfrblogging.so
  OBJS_all = $(OBJS_libllsfrblogging)
else
  ifneq ($(HAVE_PROTOBUF),1)
    WARN_TARGETS += warning_protobuf
  endif
  ifneq ($(HAVE_PROTOBUF_COMM),1)
    WARN_TARGETS += warning_protobuf_comm
  endif
  ifneq ($(HAVE_BOOST_LIBS),1)
    WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
  endif
endif

network.o: | $(PRESUBDIRS)

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST) $(WARN_TARGETS_WEBSOCKET)
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build logging library$(TNORMAL) (Boost library $* not found)"
$(WARN_TARGETS_WEBSOCKET):
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting websocket logging$(TNORMAL) (Optional Boost libraries not available: $(BOOST_LIBS_WEBSOCKET))"
endif

include $(BUILDSYSDIR)/base.mk
