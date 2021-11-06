#! /bin/bash
#
# challenge_startup.bash start and configure the refbox to play challenges
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

usage()
{
cat << EOF
usage: $0 options

This script automatically configures the RefBox to run challenges.

Options:
   -h                              Show this message

Exactly one of the following options is required:
   --production [c0|c1|c2|c3]      Production Challenge
   --navigation                    Navigation challenge
   --exploration                   Production challenge

Additional tweaking:
	 --ground-truth                  Send Ground Truth
                                   (ignored, unless --production is used)
   --difficulty [easy|medium|hard]
                                   (ignored, unless --navigation or
                                    --exploration is used)
   --team <team-name>              Add team to RefBox config
                                   (do not use this with --cfg-custom)
   --cfg-custom <yaml-file>        Load additional <yaml-file>
                                   (do not use this with --team)
EOF
}
CUSTOM_CFG=
CHALLENGE_OPT=" --cfg-challenges "
MPS_CFG=" --cfg-mps mps/mockup_mps.yaml "
CHALLENGE_FILE=
CHALLENGE_SUFFIX=
CHALLENGE_FOLDER=
MONGODB_CFG=" --cfg-mongodb mongodb/enable_mongodb.yaml"
OPTS=$(getopt -o "h" -l "production:,ground-truth,navigation,exploration,difficulty:,team:" -- "$@")
if [ $? != 0 ]
then
	echo "Failed to parse parameters"
	usage
	exit 1
fi
eval set -- "$OPTS"
while true; do
	OPTION=$1
	OPTARG=$2
	case $OPTION in
		-h)
			usage
			exit 1
			;;
		--production)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			case $OPTARG in
				c0)
					CHALLENGE_FILE="c0"
					;;
				c1)
					CHALLENGE_FILE="c1"
					;;
				c2)
					CHALLENGE_FILE="c2"
					;;
				c3)
					CHALLENGE_FILE="c3"
					;;
				*)
					echo "Option --production expected one of c0, c1, c2 or c3,\
					      instead got " $OPTARG ". Abort"
					;;
			esac
			if [ -z "$CHALLENGE_FOLDER" ]; then
				CHALLENGE_FOLDER="challenges/prod/"
			fi
			CHALLENGE_SUFFIX=".yaml"
			;;
		--exploration)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			CHALLENGE_FILE="exploration"
			CHALLENGE_FOLDER="challenges/exp/"
			;;
		--navigation)
			if [ -n "$CHALLENGE_FILE" ]; then
				echo "Can only use one challenge at a time!"
				exit
			fi
			CHALLENGE_FILE="nav"
			CHALLENGE_FOLDER="challenges/nav/"
			;;
		--difficulty)
			case $OPTARG in
				easy)
					CHALLENGE_SUFFIX="_easy.yaml"
					;;
				medium)
					CHALLENGE_SUFFIX="_medium.yaml"
					;;
				hard)
					CHALLENGE_SUFFIX="_hard.yaml"
					;;
				*)
					echo "Option --difficulty expected one of easy, medium or hard,\
					      instead got " $OPTARG ". Abort"
					;;
			esac
			;;
		--ground-truth)
			CHALLENGE_FOLDER="prod_no_gt/"
		;;
		--team)
			if [ -n "$CUSTOM_CFG" ]; then
				echo "Can only use either --team or -cfg-custom, not both!"
				exit
			fi
			printf '%s\n%s\n%s\n' '%YAML 1.2' '# Generated File, do not edit!' \
				"llsfrb/game/teams: [$OPTARG]" > $LLSF_REFBOX_DIR/cfg/team_generated.yaml
			CUSTOM_CFG=" --cfg-custom team_generated.yaml "
			;;
		--team)
			if [ -n "$CUSTOM_CFG" ]; then
				echo "Can only use either --team or -cfg-custom, not both!"
				exit
			fi
			CUSTOM_CFG=" --cfg-custom $OPTARG "
			;;
		--)
			shift
			break
			;;
		esac
		shift
done
if [ -z "$CHALLENGE_FILE" ]; then
	echo "No challenge selected, abort"
	usage
	exit
fi
if [ -z "$CHALLENGE_SUFFIX" ]; then
	echo "No difficulty selected, abort"
	usage
	exit
fi
$LLSF_REFBOX_DIR/bin/./llsf-refbox $CHALLENGE_OPT \
	$CHALLENGE_FOLDER$CHALLENGE_FILE$CHALLENGE_SUFFIX \
	$MONGODB_CFG $CUSTOM_CFG $MPS_CFG
