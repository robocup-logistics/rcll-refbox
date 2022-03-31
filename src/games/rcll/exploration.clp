;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration report processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;             2022  Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction type-can-be-reported (?type ?team)
	(bind ?num-reported-types (length$ (find-all-facts ((?exp exploration-report))
		(and (eq ?exp:team ?team)
		     (eq ?exp:type ?type) (eq ?exp:rtype RECORD)
		)
	)))
	(bind ?num-machines-of-type (length$ (find-all-facts ((?m machine)) (and (eq ?m:mtype ?type) (eq ?m:team ?team)))))
	(return (< ?num-reported-types ?num-machines-of-type))
)

(deffunction machine-can-be-reported (?type ?team)
	(bind ?num-reported-types (length$ (find-all-facts ((?exp exploration-report))
		(or (and (eq ?exp:team ?team)
		         (eq ?exp:type ?type)
		         (eq ?exp:rtype RECORD)
		         (eq ?exp:type-state WRONG_REPORT))
		    (and (str-index (str-cat ?type) (str-cat ?exp:name))
		         (eq ?exp:team ?team)
		         (eq ?exp:rtype RECORD))
		)
	)))
	(bind ?num-machines-of-type (length$ (find-all-facts ((?m machine)) (and (eq ?m:mtype ?type) (eq ?m:team ?team)))))
	(return (< ?num-reported-types ?num-machines-of-type))
)

(deffunction net-create-MachineTypeFeedback (?type ?zone ?name ?team_color)
	(bind ?m (pb-create "llsf_msgs.MachineTypeFeedback"))
	(pb-set-field ?m "type" ?type)
	(pb-set-field ?m "zone" ?zone)
	(if ?name then
		(pb-set-field ?m "name" ?name)
		(pb-set-field ?m "team_color" ?team_color)
	)
	(return ?m)
)

(defrule exploration-report-incoming
	(gamestate (phase EXPLORATION|PRODUCTION) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
	                     (rcvd-from ?from-host ?from-port) (rcvd-via ?via)
	                     (client-type ?ct) (client-id ?cid))
	(network-peer (id ?cid) (group ?group))
	=>
	(retract ?mf)
	(bind ?team (sym-cat (pb-field-value ?p "team_color")))
	(if (and (eq ?ct PEER) (neq ?team ?group))
	 then
		; message received for a team over the wrong channel, deny
		(assert (attention-message (team ?group)
		        (text (str-cat "Invalid prepare for team " ?team " of team " ?group))))
		(return)
	)
	(foreach ?m (pb-field-list ?p "machines")
		(bind ?name (if (pb-has-field ?m "name") then (sym-cat (pb-field-value ?m "name")) else UNKNOWN))
		(bind ?type (if (pb-has-field ?m "type") then (sym-cat (pb-field-value ?m "type")) else UNKNOWN))
		(bind ?zone (if (pb-has-field ?m "zone") then (sym-cat (pb-field-value ?m "zone")) else NOT-REPORTED))
		(bind ?rotation (if (pb-has-field ?m "rotation") then (pb-field-value ?m "rotation") else -1))
		(assert (exploration-report (rtype INCOMING)
		                            (name ?name) (zone ?zone) (rotation ?rotation)
		                            (game-time ?game-time) (type ?type)
		                            (team ?team) (host ?from-host) (port ?from-port)))
		(pb-destroy ?m)
	)
)

(defrule exploration-report-incoming-cleanup
	(declare (salience ?*PRIORITY_CLEANUP*))
	?ei <- (exploration-report (rtype INCOMING))
	=>
	(retract ?ei)
)

(defrule exploration-report-new-machine
	?exp <- (exploration-report (rtype INCOMING) (name ?name&~UNKNOWN)
	                            (zone ?zone) (rotation ?rotation)
	                            (game-time ?game-time)
	                            (team ?team) (host ?from-host) (port ?from-port))
	(machine (name ?name) (mtype ?type))
	(not (exploration-report (rtype RECORD) (name ?name)))
	=>
	(if (machine-can-be-reported ?type ?team)
		then
			(assert (exploration-report (rtype RECORD)
			                            (name ?name) (zone NOT-REPORTED) (rotation -1)
			                            (game-time ?game-time)
			                            (team ?team) (host ?from-host) (port ?from-port)))
	 else
			(printout t "Team " ?team " reported too many machines of type " ?type
			            " already, ignoring further reports of this type" crlf)
			(retract ?exp)
	)
)

(defrule exploration-report-new-type
	?exp <- (exploration-report (rtype INCOMING) (name UNKNOWN) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time) (type ?type)
	                    (team ?team) (host ?from-host) (port ?from-port))
	(not (exploration-report (rtype RECORD) (zone ?zone) (type ?type)))
	=>
	(if (type-can-be-reported ?type ?team)
		then
			(assert (exploration-report (rtype RECORD)
			                            (name UNKNOWN) (zone ?zone) (rotation -1)
			                            (game-time ?game-time) (type ?type)
			                            (team ?team) (host ?from-host) (port ?from-port)))
	 else
			(printout t "Team " ?team " reported too many machines of type " ?type
			            " already, ignoring further reports of this type" crlf)
			(retract ?exp)
	)
)

(defrule exploration-report-rotation-correct
	; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name)
	                           (zone-state CORRECT_REPORT) (rotation -1))
	(machine (name ?name) (team ?team) (rotation ?rotation))
	(gamestate (phase ?phase))
	=>
  (modify ?er (rotation ?rotation) (rotation-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (rotation " ?rotation "). "
	            "Awarding " ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS*)
	                (phase ?phase) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Correct partial exploration report for "
	                                 ?name ": rotation = " ?rotation))))
)

(defrule exploration-report-rotation-wrong
	; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name)
	                           (zone-state CORRECT_REPORT) (rotation -1))
	?mf <- (machine (name ?name) (team ?team) (rotation ?erotation&~?rotation))
	(gamestate (phase ?phase))
	=>
	(modify ?er (rotation ?rotation) (rotation-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (rotation " ?rotation "). "
	          "Awarding " ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS*)
	                (phase ?phase) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Wrong partial exploration report for "
	                        ?name ": rotation = " ?rotation " (should be " ?erotation ")"))))
)

(defrule exploration-report-rotation-ignored
	; triggers when zone-state is WRONG_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone-state WRONG_REPORT) (rotation -1))
	(machine (name ?name) (team ?team))
	=>
  (modify ?er (rotation ?rotation))
  (printout t "Ignored partial report: " ?name " (rotation " ?rotation "). since zone is wrong" crlf)
)

(defrule exploration-report-type-correct
	(exploration-report (rtype INCOMING) (name UNKNOWN) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time) (type ?type) (team ?team)
	                    (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (type ?type) (zone ?zone) (team ?team) (type-state ~CORRECT_REPORT))
	?mf <- (machine (name ?name) (team ?m-team) (zone ?zone) (mtype ?type))
	=>
	; % TODO: ignore if prior wrong reports are inbound
	(modify ?er (type-state CORRECT_REPORT))
	(printout t "Correct type report: " ?type " (zone " ?zone ") from " ?team " is machine " ?name "." crlf)
)

(defrule exploration-report-type-wrong
	(exploration-report (rtype INCOMING) (name UNKNOWN) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time) (type ?type)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name) (type ?type) (zone ?zone) (type-state ~WRONG_REPORT))
	(not (machine (zone ?zone) (mtype ?type)))
	=>
	(modify ?er (type-state WRONG_REPORT))
	(printout t "Wrong type report: " ?type " (zone " ?zone ") from " ?team "." crlf)
)

(defrule exploration-report-zone-correct
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?zone))
	(gamestate (phase ?phase))
	=>
	(modify ?er (zone ?zone) (zone-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (zone " ?zone "). "
	          "Awarding " ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS* " points." crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS*)
	                (phase ?phase) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Correct partial exploration report for "
	                        ?name ": zone = " ?zone))))
)

(defrule exploration-report-zone-wrong
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
	                    (game-time ?game-time)
	                    (team ?team) (host ?from-host) (port ?from-port))
	?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?mzone&~?zone))
	(gamestate (phase ?phase))
	=>
	(modify ?er (zone ?zone) (zone-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (zone " ?zone "). "
	          "Awarding " ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS*)
	                (phase ?phase) (team ?team) (game-time ?game-time)
	                (reason (str-cat "Wrong partial exploration report for "
	                        ?name ": zone = " ?zone " (should be " ?mzone ")"))))
)

(defrule exploration-report-complete-correct
	?er <- (exploration-report (rtype RECORD) (correctly-reported UNKNOWN)
	                           (name ?name) (zone ?zone&~NOT-REPORTED) (rotation ?rotation&~-1) )
	?mf <- (machine (name ?name) (team ?team) (zone ?zone) (rotation ?rotation))
	=>
	(modify ?er (correctly-reported TRUE))
)

(defrule exploration-report-complete-zone-wrong
	?er <- (exploration-report (rtype RECORD) (correctly-reported UNKNOWN)
	                           (name ?name) (zone ?zone&~NOT-REPORTED))
	?mf <- (machine (name ?name))
	(or
		(machine (name ?name) (team ?team) (zone ~?zone))
	)
	=>
	(modify ?er (correctly-reported FALSE))
)

(defrule exploration-cleanup-report
	(gamestate (phase ~EXPLORATION))
	?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
	       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
	=>
	(retract ?mf)
)

(defrule exploration-send-MachineReportInfo
	(time $?now)
	(gamestate (phase EXPLORATION|PRODUCTION) (game-time ?game-time&:(< ?game-time ?*EXPLORATION-TIME*)))
	?sf <- (signal (type machine-report-info)
	       (time $?t&:(timeout ?now ?t ?*BC-MACHINE-REPORT-INFO-PERIOD*)) (seq ?seq))
	(network-peer (group CYAN) (id ?peer-id-cyan))
	(network-peer (group MAGENTA) (id ?peer-id-magenta))
	=>
	(modify ?sf (time ?now) (seq (+ ?seq 1)))

	; CYAN
	(bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

	(pb-set-field ?s "team_color" CYAN)
	(do-for-all-facts ((?report exploration-report)) (and (eq ?report:team CYAN) (eq ?report:rtype RECORD) (neq ?report:name UNKNOWN))
		(pb-add-list ?s "reported_machines" ?report:name)
	)
	(do-for-all-facts ((?report exploration-report))
		(and (eq ?report:team CYAN)
		     (eq ?report:rtype RECORD)
		     (eq ?report:name UNKNOWN))
		(bind ?name FALSE)
		(bind ?team_color FALSE)
		(if (eq ?report:type-state CORRECT_REPORT) then
			(do-for-fact ((?m machine)) (eq ?m:zone ?report:zone)
				(bind ?name ?m:name)
				(bind ?team_color ?m:team)
			)
		)
		(bind ?m (net-create-MachineTypeFeedback ?report:type ?report:zone ?name ?team_color))
		(pb-add-list ?s "reported_types" ?m)
	)

	(pb-broadcast ?peer-id-cyan ?s)
	(pb-destroy ?s)

	; MAGENTA
	(bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

	(pb-set-field ?s "team_color" MAGENTA)
	(do-for-all-facts ((?report exploration-report)) (and (eq ?report:team MAGENTA) (eq ?report:rtype RECORD))
		(pb-add-list ?s "reported_machines" ?report:name)
	)
	(do-for-all-facts ((?report exploration-report))
		(and (eq ?report:team MAGENTA)
		     (eq ?report:rtype RECORD)
		     (eq ?report:name UNKNOWN))
		(bind ?name FALSE)
		(bind ?team_color FALSE)
		(if (eq ?report:type-state CORRECT_REPORT) then
			(do-for-fact ((?m machine)) (eq ?m:zone ?report:zone)
				(bind ?name ?m:name)
				(bind ?team_color ?m:team)
			)
		)
		(bind ?m (net-create-MachineTypeFeedback ?report:type ?report:zone ?name ?team_color))
		(pb-add-list ?s "reported_types" ?m)
	)

	(pb-broadcast ?peer-id-magenta ?s)
	(pb-destroy ?s)
)

(defrule exploration-set-ground-truth
	(machine (name ?n) (team ?team) (rotation ?rotation) (zone ?zone))
	(not (exploration-report (name ?n) (rtype RECORD) (correctly-reported TRUE)))
	(gamestate (phase PRODUCTION) (game-time ?game-time&:(>= ?game-time ?*EXPLORATION-TIME*)))
	=>
	(do-for-all-facts ((?exp exploration-report))
		(retract ?exp)
	)
	(do-for-all-facts ((?m machine))
		(assert (exploration-report (name ?m:name) (rtype RECORD) (zone ?zone)
		                            (rotation ?rotation) (correctly-reported TRUE)))
	)
)
