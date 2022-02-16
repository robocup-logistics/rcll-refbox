#! /bin/bash
#
# rcll-normal-startup.bash
# Copyright (C) 2022 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  Read the full text in the LICENSE.GPL file in the doc directory.

GAME_CFG=
MPS_CFG=" --cfg-mps mps/carologistics_mps.yaml"
COMM_CFG=" --cfg-comm comm/carologistics_comm.yaml"
DUMP_CFG=" --dump-cfg"
MONGODB_CFG=" --cfg-mongodb mongodb/enable_mongodb.yaml"

if [ -z "$LLSF_REFBOX_DIR" ]; then
	echo "LLSF_REFBOX_DIR not set, abort"
	exit
fi

$LLSF_REFBOX_DIR/bin/./llsf-refbox $MONGODB_CFG $MPS_CFG $COMM_CFG $GAME_CFG $DUMP_CFG $@
