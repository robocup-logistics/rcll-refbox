#*****************************************************************************
#             Makefile Build System for Fawkes: protobuf_comm bits
#                            -------------------
#   Created on Tue May 24 15:53:49 2016
#   Copyright (C) 2012-2016 by Tim Niemueller, AllemaniACs RoboCup Team
#
#*****************************************************************************
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#*****************************************************************************

ifndef __buildsys_config_mk_
$(error config.mk must be included before protobuf.mk)
endif

ifndef __buildsys_protobuf_comm_mk_
__buildsys_protobuf_comm_mk_ := 1


ifneq ($(PKGCONFIG),)
  HAVE_PROTOBUF_COMM = $(if $(shell $(PKGCONFIG) --exists 'protobuf_comm'; echo $${?/1/}),1,0)
endif

ifeq ($(HAVE_PROTOBUF_COMM),1)
  CFLAGS_PROTOBUF_COMM  = -DHAVE_PROTOBUF_COMM $(shell $(PKGCONFIG) --cflags 'protobuf_comm')
  LDFLAGS_PROTOBUF_COMM = $(shell $(PKGCONFIG) --libs 'protobuf_comm')
endif

endif # __buildsys_protobuf_comm_mk_
