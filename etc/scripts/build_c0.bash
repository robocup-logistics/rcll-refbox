#! /bin/bash
#
# build_c0.bash fakes a series of agent tasks to build a C0
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

#prepare cap
./llsf-fake-robot bot-1 Carologistics 3 NONE NONE NONE NONE NONE Move 1 C-CS1 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_CLEAR NONE NONE NONE CAP_GREY BufferStation 2 C-CS1 1 & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 RETRIEVE_CAP & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 NONE NONE NONE NONE NONE Move 3 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_CLEAR NONE NONE NONE NONE Retrieve 4 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_CLEAR NONE NONE NONE NONE Move 5 C_Z18 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_CLEAR NONE NONE NONE NONE Deliver 6 C_Z18 INPUT & sleep 10 ; kill $!

#get base
./llsf-fake-robot bot-1 Carologistics 3 NONE NONE NONE NONE NONE Move 7 C-BS INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-BS INPUT BASE_BLACK & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE NONE Retrieve 8 C-BS INPUT & sleep 10 ; kill $!

#mount cap
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE NONE Move 9 C-CS1 INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE NONE Deliver 10 C-CS1 INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-CS1 MOUNT_CAP & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 NONE NONE NONE NONE NONE Move 11 C-CS1 OUTPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE CAP_GREY Retrieve 12 C-CS1 OUTPUT & sleep 10 ; kill $!

#deliver
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE CAP_GREY Move 13 C-DS INPUT & sleep 10 ; kill $!
./llsf-fake-robot bot-1 Carologistics 3 BASE_BLACK NONE NONE NONE CAP_GREY Deliver 14 C-DS INPUT & sleep 10 ; kill $!
./rcll-prepare-machine Carologistics C-DS 3 & sleep 10 ; kill $!
