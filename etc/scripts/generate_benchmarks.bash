#! /bin/bash
#
# generate_benchmarks.bash Generate multiple game reports to get benchmark sets
#
# Copyright (C) 2021 Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
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
                                                                           #
echo "------------------------------------------------------------------------"
echo "Usage: ./generate_benchmarks.bash <report name> <number of reports>"
echo "Generates entries <report name>_<i> where i in [1...<number of reports>]"
echo "------------------------------------------------------------------------"
echo " "

if [ -z "$1" ] || [ -z "$2" ]
	then
		echo "expected 2 arguments"
		exit
fi

mongo --eval "print(\"Test for connection\")" &>/dev/null
if [ $? -ne 0 ]
	then
		echo "Mongo instance not running, abort."
		exit
fi

NAME_PREFIX=$1
NUM_BENCHMARKS=$2

tmpconfig=$(mktemp ${LLSF_REFBOX_DIR}/cfg/store_to_report_generated_XXXXXX.yaml)

TRAP_SIGNALS="SIGINT SIGTERM SIGPIPE EXIT"
cleanup () {
  trap - $TRAP_SIGNALS
  rm -f $tmpconfig
	killall -9 llsf-refbox &>/dev/null &
}
trap cleanup $TRAP_SIGNALS

for i in  $(seq 1 $NUM_BENCHMARKS)
do
	DOC_COUNT=$(mongo rcll --eval "db.game_report.count({\"report_name\": \"${NAME_PREFIX}_${i}\"})" --quiet)

	# make sure that the report name is fresh
	if [ ${DOC_COUNT} -ne 0 ]; then
		echo "Skipping ${NAME_PREFIX}_${i} as a report already exists"
	else
		truncate -s 0 ${tmpconfig}
		echo "llsfrb/game/store-to-report: \"${NAME_PREFIX}_${i}\"" > ${tmpconfig}
		# in rare occasions the generation fails, hence simply retry when it happens
		while [ ${DOC_COUNT} -ne 1 ]
		do
			echo "attempt to generate report"
			${LLSF_REFBOX_DIR}/bin/./llsf-refbox --cfg-mongodb mongodb/enable_mongodb.yaml \
			                                     --cfg-mps mps/mockup_mps.yaml \
			                                     --cfg-custom ${tmpconfig} \
			                                     --dump-cfg &>/dev/null &
			sleep 1
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p PRE_GAME -s RUNNING -c Carologistics &>/dev/null
			sleep 2
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p PRODUCTION &>/dev/null
			sleep 5
			${LLSF_REFBOX_DIR}/bin/./rcll-refbox-instruct -p POST_GAME &>/dev/null
			sleep 1
			killall -15 llsf-refbox  &>/dev/null &
			sleep 1
			DOC_COUNT=$(mongo rcll --eval "db.game_report.count({\"report_name\": \"${NAME_PREFIX}_${i}\"})" --quiet)
		done
		echo "Created report for ${NAME_PREFIX}_${i}"
	fi
done
