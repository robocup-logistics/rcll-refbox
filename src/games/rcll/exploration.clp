
;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule exploration-start
  (declare (salience ?*PRIORITY_HIGH*))
  ?gf <- (gamestate (phase EXPLORATION) (prev-phase ~EXPLORATION))
  =>
  ; Set prev phase to avoid re-firing, reset game time
  (modify ?gf (prev-phase EXPLORATION) (game-time 0.0))

  ; Retract all existing reports for the new exploration phase
  (delayed-do-for-all-facts ((?report exploration-report)) TRUE
    (retract ?report)
  )

  ; Set lights
  (delayed-do-for-all-facts ((?machine machine)) TRUE
		(modify ?machine (desired-lights YELLOW-ON))
  )

  ;(assert (attention-message (text "Entering Exploration Phase")))
)

(defrule exploration-pause
  ?gf <- (gamestate (phase EXPLORATION) (state PAUSED) (prev-state ~PAUSED))
  =>
  (modify ?gf (prev-state PAUSED))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights))
  )
)

(defrule exploration-continue
  ?gf <- (gamestate (phase EXPLORATION) (state RUNNING) (prev-state ~RUNNING))
  =>
  (modify ?gf (prev-state RUNNING))
  (delayed-do-for-all-facts ((?machine machine)) TRUE

		(if (any-factp ((?er exploration-report)) (eq ?er:name ?machine:name))
		 then
		  (do-for-fact ((?er exploration-report)) (eq ?er:name ?machine:name)
				(if (eq ?er:correctly-reported UNKNOWN)
				 then
					(modify ?machine (desired-lights YELLOW-ON))
				 else
					(modify ?machine (desired-lights (create$ (if ?er:correctly-reported then GREEN-BLINK else RED-BLINK))))
				)
			)
		 else
		  (modify ?machine (desired-lights YELLOW-ON))
		)
	)
)

(defrule exploration-report-incoming
  ?gf <- (gamestate (phase EXPLORATION) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (foreach ?m (pb-field-list ?p "machines")
    (bind ?name (sym-cat (pb-field-value ?m "name")))
    (bind ?zone (if (pb-has-field ?m "zone") then (sym-cat (pb-field-value ?m "zone")) else NOT-REPORTED))
    (bind ?rotation (if (pb-has-field ?m "rotation") then (pb-field-value ?m "rotation") else -1))
		(assert (exploration-report (rtype INCOMING)
							(name ?name) (zone ?zone) (rotation ?rotation)
							(game-time ?game-time)
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

(defrule exploration-report-new
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone) (rotation ?rotation)
											(game-time ?game-time)
											(team ?team) (host ?from-host) (port ?from-port))
	(not (exploration-report (rtype RECORD) (name ?name)))
	=>
	(assert (exploration-report (rtype RECORD)
															(name ?name) (zone NOT-REPORTED) (rotation -1)
															(game-time ?game-time)
															(team ?team) (host ?from-host) (port ?from-port)))
)

(defrule exploration-report-rotation-correct
  ; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
											(game-time ?game-time)
											(team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone-state CORRECT_REPORT) (rotation -1))
	?mf <- (machine (name ?name) (team ?team) (rotation ?rotation))
	=>
  (modify ?er (rotation ?rotation) (rotation-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (rotation " ?rotation "). "
						"Awarding " ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ROTATION-POINTS*)
									(phase EXPLORATION) (team ?team) (game-time ?game-time)
									(reason (str-cat "Correct partial exploration report for "
																	 ?name ": rotation = " ?rotation))))
  (modify ?mf (desired-lights GREEN-BLINK))
)

(defrule exploration-report-rotation-wrong
  ; only triggers after zone-state is CORRECT_REPORT
	(exploration-report (rtype INCOMING) (name ?name) (rotation ?rotation&~-1)
											(game-time ?game-time)
											(team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone-state CORRECT_REPORT) (rotation -1))
	?mf <- (machine (name ?name) (team ?team) (rotation ?erotation&~?rotation))
	=>
  (modify ?er (rotation ?rotation) (rotation-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (rotation " ?rotation "). "
						"Awarding " ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ROTATION-POINTS*)
									(phase EXPLORATION) (team ?team) (game-time ?game-time)
									(reason (str-cat "Wrong partial exploration report for "
													 ?name ": rotation = " ?rotation " (should be " ?erotation ")"))))
  (modify ?mf (desired-lights GREEN-ON RED-ON))
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

(defrule exploration-report-zone-correct
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
											(game-time ?game-time)
											(team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?zone))
	=>
  (modify ?er (zone ?zone) (zone-state CORRECT_REPORT))
	(printout t "Correct partial report: " ?name " (zone " ?zone "). "
						"Awarding " ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS* " points." crlf)
	(assert (points (points ?*EXPLORATION-CORRECT-REPORT-ZONE-POINTS*)
									(phase EXPLORATION) (team ?team) (game-time ?game-time)
									(reason (str-cat "Correct partial exploration report for "
																	 ?name ": zone = " ?zone))))

	(modify ?mf (desired-lights GREEN-ON))
)

(defrule exploration-report-zone-wrong
	(exploration-report (rtype INCOMING) (name ?name) (zone ?zone&~NOT-REPORTED)
											(game-time ?game-time)
											(team ?team) (host ?from-host) (port ?from-port))
  ?er <- (exploration-report (rtype RECORD) (name ?name) (zone NOT-REPORTED))
	?mf <- (machine (name ?name) (team ?team) (zone ?mzone&~?zone))
	=>
  (modify ?er (zone ?zone) (zone-state WRONG_REPORT))
	(printout t "Wrong partial report: " ?name " (zone " ?zone "). "
						"Awarding " ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS* " points" crlf)
	(assert (points (points ?*EXPLORATION-WRONG-REPORT-ZONE-POINTS*)
									(phase EXPLORATION) (team ?team) (game-time ?game-time)
									(reason (str-cat "Wrong partial exploration report for "
																	 ?name ": zone = " ?zone " (should be " ?mzone ")"))))
  (modify ?mf (desired-lights RED-BLINK))
)

(defrule exploration-report-complete-correct
  ?er <- (exploration-report (rtype RECORD) (correctly-reported UNKNOWN)
														 (name ?name) (zone ?zone&~NOT-REPORTED) (rotation ?rotation&~-1) )
	?mf <- (machine (name ?name) (team ?team) (zone ?zone) (rotation ?rotation))
	=>
  (modify ?er (correctly-reported TRUE))
;	(modify ?mf (desired-lights GREEN-BLINK))
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
;	(modify ?mf (desired-lights RED-BLINK))
)

; (defrule exploration-handle-report
;   ?gf <- (gamestate (phase EXPLORATION) (game-time ?game-time))
;   ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
; 		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
;   =>
;   (retract ?mf)
;   (bind ?team (sym-cat (pb-field-value ?p "team_color")))
;   (foreach ?m (pb-field-list ?p "machines")
;     (bind ?name (sym-cat (pb-field-value ?m "name")))
;     (bind ?type (pb-field-value ?m "type"))
;     (bind ?zone (sym-cat (pb-field-value ?m "zone")))
;     (if (member$ ?name (deftemplate-slot-allowed-values exploration-report name))
;     then
;       (do-for-fact ((?machine machine)) (eq ?machine:name ?name)
;         (if (neq (sym-cat ?machine:team) ?team)
; 				 then
;           (if (not (any-factp ((?report exploration-report))
; 			      (and (eq ?report:name ?name) (eq ?report:team ?team))))
; 					 then
; 					  (printout t "Invalid report: " ?name " of type " ?type " from other team." crlf)
;              ; "Awarding " ?*EXPLORATION-INVALID-REPORT-POINTS* " points" crlf)
; 	           ; (assert (points (points ?*EXPLORATION-INVALID-REPORT-POINTS*)
; 	           ; 		    (phase EXPLORATION) (team ?team) (game-time ?game-time)
; 	           ; 		    (reason (str-cat "Report for machine of other team"
; 	           ; 				     ?name "|" ?type))))
; 	           ; (assert (exploration-report (name ?name) (team ?team) (type ?type)
; 	           ; 				(game-time ?game-time) (correctly-reported FALSE)
; 	           ; 				(host ?from-host) (port ?from-port)))
; 	           ; (modify ?machine (desired-lights RED-BLINK YELLOW-BLINK))
; 					  )
; 	         else
;             ; If it has not been reported, yet
;             (if (not (any-factp ((?report exploration-report))
; 			        (and (eq ?report:name ?name) (eq ?report:team ?team))))
; 	           then
; 	             (printout t "Comparing T " ?machine:exploration-type " " ?type "  Z " ?machine:zone " " ?zone crlf)
;                (if (and (eq ?machine:exploration-type ?type) (eq ?machine:zone ?zone))
;                 then ; correct report
; 	               (printout t "Correct report: " ?name " (type " ?type ") in zone " ?zone ". "
; 			              "Awarding " ?*EXPLORATION-CORRECT-REPORT-POINTS* " points" crlf)
; 	               (assert (points (points ?*EXPLORATION-CORRECT-REPORT-POINTS*)
; 			      (phase EXPLORATION) (team ?team) (game-time ?game-time)
; 			      (reason (str-cat "Correct exploration report for "
; 					       ?name "|" ?type))))
; 	      (assert (exploration-report (name ?name) (type ?type) (zone ?zone)
; 					  (game-time ?game-time) (correctly-reported TRUE)
; 					  (team ?team) (host ?from-host) (port ?from-port)))
; 	      (modify ?machine (desired-lights GREEN-BLINK))
;             else ; wrong report
; 	      (printout t "Wrong report: " ?name " (type " ?type ") in zone " ?zone ". "
; 			"Penalizing with " ?*EXPLORATION-WRONG-REPORT-POINTS* " points" crlf)
; 	      (assert (points (points ?*EXPLORATION-WRONG-REPORT-POINTS*)
; 			      (phase EXPLORATION) (team ?team) (game-time ?game-time)
; 			      (reason (str-cat "Wrong exploration report for "
; 					       ?name "|" ?type))))
; 	      (assert (exploration-report (name ?name) (type ?type) (zone ?zone)
; 					  (game-time ?game-time) (correctly-reported FALSE)
; 					  (team ?team) (host ?from-host) (port ?from-port)))
; 	      (modify ?machine (desired-lights RED-BLINK))
; 	    )
;           )
;         )
;       )
;     )
;     (pb-destroy ?m)
;   )
; )

(defrule exploration-cleanup-report
  (gamestate (phase ~EXPLORATION))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
)

(defrule exploration-send-MachineReportInfo
  (time $?now)
  (gamestate (phase EXPLORATION))
  ?sf <- (signal (type machine-report-info)
		 (time $?t&:(timeout ?now ?t ?*BC-MACHINE-REPORT-INFO-PERIOD*)) (seq ?seq))
  (network-peer (group CYAN) (id ?peer-id-cyan))
  (network-peer (group MAGENTA) (id ?peer-id-magenta))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))

  ; CYAN
  (bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

  (pb-set-field ?s "team_color" CYAN)
  (do-for-all-facts ((?report exploration-report)) (and (eq ?report:team CYAN) (eq ?report:rtype RECORD))
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?peer-id-cyan ?s)
  (pb-destroy ?s)

  ; MAGENTA
  (bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

  (pb-set-field ?s "team_color" MAGENTA)
  (do-for-all-facts ((?report exploration-report)) (and (eq ?report:team MAGENTA) (eq ?report:rtype RECORD))
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?peer-id-magenta ?s)
  (pb-destroy ?s)
)
