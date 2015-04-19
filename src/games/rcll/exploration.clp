
;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
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
  (delayed-do-for-all-facts ((?machine machine) (?lc machine-light-code))
    (= ?machine:exploration-light-code ?lc:id)
    (modify ?machine (desired-lights ?lc:code))
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
  (delayed-do-for-all-facts ((?machine machine) (?lc machine-light-code))
    (= ?machine:exploration-light-code ?lc:id)
    (modify ?machine (desired-lights ?lc:code))
  )
)

(defrule exploration-handle-report
  ?gf <- (gamestate (phase EXPLORATION) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (foreach ?m (pb-field-list ?p "machines")
    (bind ?name (sym-cat (pb-field-value ?m "name")))
    (bind ?type (pb-field-value ?m "type"))
    (bind ?zone (sym-cat (pb-field-value ?m "zone")))
    (if (member$ ?name (deftemplate-slot-allowed-values exploration-report name))
    then
      (do-for-fact ((?machine machine)) (eq ?machine:name ?name)
        (if (neq (sym-cat ?machine:team) ?team)
	then
          (if (not (any-factp ((?report exploration-report))
			      (and (eq ?report:name ?name) (eq ?report:team ?team))))
	  then
	    (printout t "Invalid report: " ?name " of type " ?type " from other team. "
		      "Awarding " ?*EXPLORATION-INVALID-REPORT-POINTS* " points" crlf)
	    (assert (points (points ?*EXPLORATION-INVALID-REPORT-POINTS*)
			    (phase EXPLORATION) (team ?team) (game-time ?game-time)
			    (reason (str-cat "Report for machine of other team"
					     ?name "|" ?type))))
	    (assert (exploration-report (name ?name) (team ?team) (type ?type)
					(game-time ?game-time) (correctly-reported FALSE)
					(host ?from-host) (port ?from-port)))
	  )
	else
          ; If it has not been reported, yet
          (if (not (any-factp ((?report exploration-report))
			      (and (eq ?report:name ?name) (eq ?report:team ?team))))
	  then
	    (printout t "Comparing T " ?machine:exploration-type " " ?type "  Z " ?machine:zone " " ?zone crlf)
            (if (and (eq ?machine:exploration-type ?type) (eq ?machine:zone ?zone))
            then ; correct report
	      (printout t "Correct report: " ?name " (type " ?type ") in zone " ?zone ". "
			"Awarding " ?*EXPLORATION-CORRECT-REPORT-POINTS* " points" crlf) 
	      (assert (points (points ?*EXPLORATION-CORRECT-REPORT-POINTS*)
			      (phase EXPLORATION) (team ?team) (game-time ?game-time)
			      (reason (str-cat "Correct exploration report for "
					       ?name "|" ?type))))
	      (assert (exploration-report (name ?name) (type ?type) (zone ?zone)
					  (game-time ?game-time) (correctly-reported TRUE)
					  (team ?team) (host ?from-host) (port ?from-port)))
            else ; wrong report
	      (printout t "Wrong report: " ?name " (type " ?type ") in zone " ?zone ". "
			"Penalizing with " ?*EXPLORATION-WRONG-REPORT-POINTS* " points" crlf)
	      (assert (points (points ?*EXPLORATION-WRONG-REPORT-POINTS*)
			      (phase EXPLORATION) (team ?team) (game-time ?game-time)
			      (reason (str-cat "Wrong exploration report for "
					       ?name "|" ?type))))
	      (assert (exploration-report (name ?name) (type ?type) (zone ?zone)
					  (game-time ?game-time) (correctly-reported FALSE)
					  (team ?team) (host ?from-host) (port ?from-port)))
	    )
          )
        )
      )
    )
    (pb-destroy ?m)
  )
)

(defrule exploration-cleanup-report
  (gamestate (phase ~EXPLORATION))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
)


(defrule exploration-send-ExplorationInfo
  (time $?now)
  (gamestate (phase EXPLORATION))
  ?sf <- (signal (type exploration-info)
		 (time $?t&:(timeout ?now ?t ?*BC-EXPLORATION-INFO-PERIOD*)) (seq ?seq))
  (network-peer (group PUBLIC) (id ?peer-id-public))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))
  (bind ?ei (pb-create "llsf_msgs.ExplorationInfo"))

  (bind ?machines (create$))
  (do-for-all-facts ((?m machine)) (and (eq ?m:team CYAN) (<> ?m:exploration-light-code 0))
    (bind ?machines (append$ ?machines ?m))
  )

  ; Randomize machines, otherwise order in messages would yield info
  ; on machine to light assignments and tag detection alone would suffice
  (bind ?machines (randomize$ ?machines))

  (foreach ?m ?machines
    (do-for-fact ((?lc machine-light-code))
      (= (fact-slot-value ?m exploration-light-code) ?lc:id)

      (bind ?s (pb-create "llsf_msgs.ExplorationSignal"))
      (pb-set-field ?s "type" (fact-slot-value ?m exploration-type))
      ; nested foreach are broken, hence use progn$
      (progn$ (?color (create$ RED YELLOW GREEN))
        (bind ?state OFF)
        (progn$ (?poss-state (create$ OFF ON BLINK))
          (if (member$ (sym-cat ?color "-" ?poss-state) ?lc:code)
	    then (bind ?state (str-cat ?poss-state))
          )
        )
        (bind ?s-light (pb-create "llsf_msgs.LightSpec"))
	(pb-set-field ?s-light "color" ?color)
	(pb-set-field ?s-light "state" ?state)
        (pb-add-list ?s "lights" ?s-light)
      )

      (pb-add-list ?ei "signals" ?s)
    )
  )

  (bind ?zones-cyan    ?*MACHINE-ZONES-CYAN*)
  (bind ?zones-magenta ?*MACHINE-ZONES-MAGENTA*)
  (do-for-all-facts ((?m machine))
    (and (eq ?m:team CYAN) (or (eq ?m:mtype RS) (eq ?m:mtype CS)))

    (bind ?z-index (member$ ?m:zone ?zones-cyan))

    (if (not ?z-index) then
      ; machines swapped
      (bind ?z-index (member$ ?m:zone ?zones-magenta))
      (bind ?z-cyan    (nth$ ?z-index ?zones-cyan))
      (bind ?z-magenta (nth$ ?z-index ?zones-magenta))
      (bind ?zones-cyan    (replace$ ?zones-cyan ?z-index ?z-index ?z-magenta))
      (bind ?zones-magenta (replace$ ?zones-magenta ?z-index ?z-index ?z-cyan))
    )
  )

  (foreach ?z ?zones-cyan
    (bind ?zm (pb-create "llsf_msgs.ExplorationZone"))
    (pb-set-field ?zm "zone" ?z)
    (pb-set-field ?zm "team_color" CYAN)
    (pb-add-list ?ei "zones" ?zm)
  )
  (foreach ?z ?zones-magenta
    (bind ?zm (pb-create "llsf_msgs.ExplorationZone"))
    (pb-set-field ?zm "zone" ?z)
    (pb-set-field ?zm "team_color" MAGENTA)
    (pb-add-list ?ei "zones" ?zm)
  )

  (pb-broadcast ?peer-id-public ?ei)
  (pb-destroy ?ei)
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
  (do-for-all-facts ((?report exploration-report)) (eq ?report:team CYAN)
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?peer-id-cyan ?s)
  (pb-destroy ?s)

  ; MAGENTA
  (bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

  (pb-set-field ?s "team_color" MAGENTA)
  (do-for-all-facts ((?report exploration-report)) (eq ?report:team MAGENTA)
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?peer-id-magenta ?s)
  (pb-destroy ?s)
)
