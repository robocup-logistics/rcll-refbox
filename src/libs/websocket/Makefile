#*****************************************************************************
#           Makefile Build System for LLSF RefBox: Websocket Backend
#                            -------------------
#   Created on Tue Mar 31 16:09:00 2020
#   Copyright (C) 2020 Daniel Swoboda [swoboda@kbsg.rwth-aachen.de]
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
include $(BASEDIR)/etc/buildsys/gcc.mk
include $(BUILDSYSDIR)/boost.mk
include $(LIBSRCDIR)/webview/webview.mk
include $(BUILDSYSDIR)/clips.mk

REQ_BOOST_LIBS = asio beast 
HAVE_BOOST_LIBS = $(call boost-have-libs,$(REQ_BOOST_LIBS))

CFLAGS += $(CFLAGS_CPP11)


ifeq ($(HAVE_BOOST_LIBS),1)

  LIBS_libllsfrbwebsocket = stdc++ pthread llsfrbcore 
  #OBJS_libllsfrbwebsocket = $(patsubst %.cpp,%.o,$(patsubst qa/%,,$(subst $(SRCDIR)/,,$(realpath $(wildcard $(SRCDIR)/*.cpp)))))
  OBJS_libllsfrbwebsocket = data.o server.o client.o backend.o
  HDRS_libllsfrbwebsocket = $(subst $(SRCDIR)/,,$(wildcard $(SRCDIR)/*.h))

  OBJS_all = $(OBJS_libllsfrwebsocket)

  ifeq ($(HAVE_CPP17)$(HAVE_RAPIDJSON),11)
    CFLAGS  += $(CFLAGS_CPP17)  $(CFLAGS_RAPIDJSON) $(CFLAGS_CLIPS)
    LDFLAGS += $(LDFLAGS_CPP17) $(LDFLAGS_RAPIDJSON) $(LDFLAGS_CLIPS)
    LIBS_all  = $(LIBDIR)/libllsfrbwebsocket.so
  else
    ifneq ($(HAVE_CPP17),1)
       WARN_TARGETS += warning_cpp17
    endif
    ifneq ($(HAVE_RAPIDJSON),1)
      WARN_TARGETS += warning_rapidjson
    endif
  endif

else
  WARN_TARGETS_BOOST = $(foreach l,$(REQ_BOOST_LIBS),$(if $(call boost-have-lib,$l),, warning_boost_$l))
endif

ifeq ($(OBJSSUBMAKE),1)
all: $(WARN_TARGETS) $(WARN_TARGETS_BOOST)
$(WARN_TARGETS_BOOST): warning_boost_%:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Cannot build websocket library$(TNORMAL) (Boost library $* not found)"
warning_cpp17:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting websocket library$(TNORMAL) (C++17 not supported)"
warning_rapidjson:
	$(SILENT)echo -e "$(INDENT_PRINT)--> $(TRED)Omitting websocket library$(TNORMAL) (RapidJSON not available)"

endif

include $(BUILDSYSDIR)/base.mk
