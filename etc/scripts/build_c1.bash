#! /bin/bash
#
# build_c1_r1.bash fakes a series of agent tasks for 2 bots to build a C1
#
# Copyright (C) 2022 Matteo Tschesche <matteo.tschesche@rwth-aachen.de>
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

#R1 prepare cap
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 1 C-CS1 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE CAP_GREY BufferStation 2 C-CS1 1 & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 RETRIEVE_CAP & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 3 C-CS1 OUTPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Retrieve 4 C-CS1 OUTPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Move 5 C_Z18 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_CLEAR NONE NONE NONE NONE Deliver 6 C_Z18 INPUT & sleep 8 ; kill $!

#R2 get base
./llsf-fake-robot bot-2 Carologistics 2 3 NONE NONE NONE NONE NONE Move 1 C-BS INPUT & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-BS INPUT BASE_BLACK & sleep 10 ; kill $!
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Retrieve 2 C-BS INPUT & sleep 8 ; kill $!

#R1 get base and feed RS
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 7 C-BS INPUT & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-BS INPUT BASE_BLACK & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Retrieve 8 C-BS INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Move 9 C-RS1 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK NONE NONE NONE NONE Deliver 10 C-RS1 SHELF & sleep 8 ; kill $!
./rcll-machine-add-base C-RS1 & sleep 10 ; kill $!

#R2 transport base to RS
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Move 3 C-RS1 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Deliver 4 C-RS1 INPUT & sleep 8 ; kill $!

#R2 get base, feed RS and mount ring
./llsf-fake-robot bot-2 Carologistics 2 3 NONE NONE NONE NONE NONE Move 5 C-BS INPUT & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-BS INPUT BASE_BLACK & sleep 10 ; kill $!
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Retrieve 6 C-BS INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Move 7 C-RS1 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-2 Carologistics 2 3 BASE_BLACK NONE NONE NONE NONE Deliver 8 C-RS1 SHELF & sleep 8 ; kill $!
./rcll-machine-add-base C-RS1 & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-RS1 RING_ORANGE & sleep 10 ; kill $!

#R1 mount cap
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 11 C-RS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE NONE Retrieve 12 C-RS1 OUTPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE NONE Move 13 C-CS1 INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE NONE Deliver 14 C-CS1 INPUT & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 MOUNT_CAP & sleep 15 ; kill $!

#R1 deliver
./llsf-fake-robot bot-1 Carologistics 1 3 NONE NONE NONE NONE NONE Move 15 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE CAP_GREY Retrieve 16 C-CS1 OUTPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE CAP_GREY Move 17 C-DS INPUT & sleep 8 ; kill $!
./llsf-fake-robot bot-1 Carologistics 1 3 BASE_BLACK RING_ORANGE NONE NONE CAP_GREY Deliver 18 C-DS INPUT & sleep 8 ; kill $!
./rcll-prepare-machine Carologistics C-DS 3 & sleep 10 ; kill $!
