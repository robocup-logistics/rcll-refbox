
;---------------------------------------------------------------------------
;  exploration.clp - LLSF RefBox CLIPS exploration phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule machine-enable-exploration
  ?gf <- (gamestate (phase EXPLORATION) (prev-phase ~EXPLORATION))
  ?mf <- (machine (mtype ?mtype))
  (machine-spec (mtype ?mtype) (light-code ?lc))
  =>
  ; Set prev phase to avoid re-firing, reset game time
  (modify ?gf (prev-phase EXPLORATION) (game-time 0.0))

  ; Retract all existing reports for the new exploration phase
  (delayed-do-for-all-facts ((?report exploration-report)) TRUE
    (retract ?report)
  )

  ; Gather all available light codes
  (bind ?light-codes (create$))
  (do-for-all-facts ((?lc machine-light-code)) TRUE
    (bind ?light-codes (create$ ?light-codes ?lc:id))
  )
  ; Randomize light codes
  (bind ?light-codes (randomize$ ?light-codes))
  ; Assign random light codes
  (delayed-do-for-all-facts ((?mspec machine-spec)) TRUE
    (do-for-fact ((?light-code machine-light-code)) (= ?light-code:id (nth$ 1 ?light-codes))
      (printout t "Light code " ?light-code:code " for machine type " ?mspec:mtype crlf)
    )
    (modify ?mspec (light-code (nth$ 1 ?light-codes)))
    (bind ?light-codes (delete$ ?light-codes 1 1))
  )

  ; Set lights
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (bind ?dl (create$))
    (do-for-fact ((?spec machine-spec) (?lc machine-light-code))
		 (and (eq ?machine:mtype ?spec:mtype) (= ?spec:light-code ?lc:id))
      (bind ?dl ?lc:code)
      ;(printout t "Assigning " ?lc:code " to machine " ?machine:name
      ;		   " of type " ?machine:mtype crlf)
    )
    (modify ?machine (desired-lights ?dl))
  )

  (assert (attention-message "Entering Exploration Phase" 5))
)


(defrule exploration-handle-report
  ?gf <- (gamestate (phase EXPLORATION) (points ?points) (game-time ?game-time))
  ?mf <- (protobuf-msg (type "llsf_msgs.MachineReport") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf)
  (foreach ?m (pb-field-list ?p "machines")
    (bind ?name (sym-cat (pb-field-value ?m "name")))
    (bind ?type (sym-cat (pb-field-value ?m "type")))
    (if (member$ ?name (deftemplate-slot-allowed-values exploration-report name))
    then
      (do-for-fact ((?machine machine)) (eq ?machine:name ?name)
        ; If it has not been reported, yet
        (if (not (any-factp ((?report exploration-report)) (eq ?report:name ?name)))
        then
          (if (eq ?machine:mtype ?type)
          then ; correct report
	    (printout t "Correct report: " ?name " of type " ?type crlf) 
	    (modify ?gf (points (+ ?points ?*EXPLORATION-CORRECT-REPORT-POINTS*)))
	    (assert (exploration-report (name ?name) (type ?type) (game-time ?game-time)
					(host ?from-host) (port ?from-port)))
          else ; wrong report
	    (printout t "Wrong report: " ?name " of type " ?type crlf) 
	    (modify ?gf (points (max (+ ?points ?*EXPLORATION-WRONG-REPORT-POINTS*) 0)))
	    (assert (exploration-report (name ?name) (type WRONG) (game-time ?game-time)
					(host ?from-host) (port ?from-port)))
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

(defrule exploration-send-MachineReportInfo
  (time $?now)
  (gamestate (phase EXPLORATION))
  ?sf <- (signal (type machine-report-info)
		 (time $?t&:(timeout ?now ?t ?*BC-MACHINE-REPORT-INFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))
  (bind ?s (pb-create "llsf_msgs.MachineReportInfo"))

  (do-for-all-facts ((?report exploration-report)) TRUE
    (pb-add-list ?s "reported_machines" ?report:name)
  )

  (pb-broadcast ?s)
  (pb-destroy ?s)
)
