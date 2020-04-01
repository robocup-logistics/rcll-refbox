#*****************************************************************************
#                      Makefile Build System for Fawkes
#                            -------------------
#   Created on Fri Jan 18 00:17:05 2013
#   Copyright (C) 2006-2011 by Tim Niemueller, Carologistics RoboCup Team
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

SUBDIRS = core utils config logging netcomm protobuf_comm protobuf_clips \
	  mongodb_log mps_comm mps_placing_clips  webview  rest-api websocket

# Explicit dependencies, this is needed to have make bail out if there is any
# error. This is also necessary for working parallel build (i.e. for dual core)
netcomm config mongodb_log: core utils
utils mps_placing_clips: core
mps_comm: core config
logging: core protobuf_comm
protobuf_clips: protobuf_comm
mongodb_log: logging
rest-api: webview
webview: core logging utils

include $(BUILDSYSDIR)/base.mk
