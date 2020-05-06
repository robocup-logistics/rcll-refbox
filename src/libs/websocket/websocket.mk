#*****************************************************************************
#           Makefile Build System for LLSF RefBox: Websocket Backend
#                            -------------------
#   Created on Tue May 05 16:18:00 2020
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


include $(BASEDIR)/etc/buildsys/config.mk
include $(BASEDIR)/etc/buildsys/gcc.mk
include $(BUILDSYSDIR)/boost.mk

#define the boost libs we need
BOOST_LIBS_WEBSOCKET = asio beast 
HAVE_WEBSOCKET = $(call boost-have-libs,$(BOOST_LIBS_WEBSOCKET))

#needs include for this
ifeq ($(HAVE_WEBSOCKET),1)
  CFLAGS_WEBSOCKET = $(call boost-libs-cflags,$(BOOST_LIBS_WEBSOCKET)) -DHAVE_WEBSOCKETS
  LDFLAGS_WEBSOCKET = $(call boost-libs-ldflags,$(BOOST_LIBS_WEBSOCKET)) 
  LIBS_WEBSOCKET = llsfrbwebsocket
	
endif 


