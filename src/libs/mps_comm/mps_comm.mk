#*****************************************************************************
#                  Makefile Build System for the RCLL Refbox
#                            -------------------
#   Created on Sun 28 Apr 2019 16:00:20 CEST
#   Copyright (C) 2019 by Till Hofmann <hofmann@kbsg.rwth-aachen.de>
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

FREEOPCUA_COMPONENTS=libopcuacore libopcuaclient libopcuaprotocol
ifneq ($(PKGCONFIG),)
  HAVE_FREEOPCUA= $(if $(shell $(PKGCONFIG) --exists $(FREEOPCUA_COMPONENTS); echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_FREEOPCUA),1)
	HAVE_MPS_COMM = 1
  CFLAGS_MPS_COMM  = $(shell $(PKGCONFIG) --cflags $(FREEOPCUA_COMPONENTS))
  LDFLAGS_MPS_COMM = $(shell $(PKGCONFIG) --libs $(FREEOPCUA_COMPONENTS))
endif
