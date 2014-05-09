
;---------------------------------------------------------------------------
;  machines.clp - LLSF RefBox CLIPS machine processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule m-shutdown "Shutdown machines at the end"
  (finalize)
  ?mf <- (machine (name ?m) (desired-lights $?dl&:(> (length$ ?dl) 0)))
  =>
  (modify ?mf (desired-lights))
)

(defrule machine-lights "Set machines if desired lights differ from actual lights"
  ?mf <- (machine (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  ;(printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
  (modify ?mf (actual-lights ?dl))
  (foreach ?color (create$ RED YELLOW GREEN)
    (if (member$ (sym-cat ?color "-ON") ?dl)
    then 
      (sps-set-signal (str-cat ?m) ?color "ON")
    else
      (if (member$ (sym-cat ?color "-BLINK") ?dl)
      then
        (sps-set-signal (str-cat ?m) ?color "BLINK")
      else
        (sps-set-signal (str-cat ?m) ?color "OFF")
      )
    )
  )
)

(deffunction machine-magenta-for-cyan-gate (?cyan-gate)
  (return
    (switch ?cyan-gate
      (case D1 then D4)
      (case D2 then D5)
      (case D3 then D6)
      (default ANY)
    )
  )
)


(deffunction machine-magenta-for-cyan-field (?m-cyan)
  (return
    (switch ?m-cyan
      (case M1  then M13)
      (case M2  then M14)
      (case M3  then M15)
      (case M4  then M16)
      (case M5  then M17)
      (case M6  then M18)
      (case M7  then M19)
      (case M8  then M20)
      (case M9  then M21)
      (case M10 then M22)
      (case M11 then M23)
      (case M12 then M24)
      (default M13)
    )
  )
)

(deffunction machine-init-randomize ()
  (if ?*RANDOMIZE-GAME* then
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
        ;(printout t "Light code " ?light-code:code " for machine type " ?mspec:mtype crlf)
      )
      (modify ?mspec (light-code (nth$ 1 ?light-codes)))
      (bind ?light-codes (delete$ ?light-codes 1 1))
    )
  )


  ; reset machines
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (loaded-with) (productions 0) (state IDLE)
	             (proc-start 0.0) (puck-id 0) (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )

  ; assign random machine types out of the start distribution
  ;(printout t "Initial machine distribution:    " ?*MACHINE-DISTRIBUTION* crlf)
  (if ?*RANDOMIZE-GAME*
    then
      (bind ?machine-assignment (randomize$ ?*MACHINE-DISTRIBUTION*))
      ;(printout t "Randomized machine distribution: " ?machine-assignment crlf)
    else (bind ?machine-assignment ?*MACHINE-DISTRIBUTION*)
  )
  (delayed-do-for-all-facts ((?machine machine))
    (and (any-factp ((?mspec machine-spec)) (eq ?mspec:mtype ?machine:mtype))
	 (eq ?machine:team CYAN))
    (if (= (length$ ?machine-assignment) 0)
     then (printout error "No machine assignment available for " ?machine:name crlf)
     else
       (bind ?mtype (nth$ 1 ?machine-assignment))
       (bind ?machine-assignment (delete$ ?machine-assignment 1 1))
       ;(printout t "Assigning type " ?mtype " to machine " ?machine:name crlf)
       (modify ?machine (mtype ?mtype))
    )
  )

  (if (or (not (any-factp ((?dp delivery-period)) TRUE)) ?*RANDOMIZE-GAME*)
   then
    ; no delivery periods exist at all or randomization has been enabled
    ; erase all existing delivery periods, might be left-overs
    ; from pre-defined facts for non-random game
    (delayed-do-for-all-facts ((?p delivery-period)) TRUE (retract ?p))
    ; assign random active delivery gate times
    (bind ?delivery-gates (create$))
    (do-for-all-facts ((?m machine)) (and (eq ?m:mtype DELIVER) (eq ?m:team CYAN))
      (bind ?delivery-gates (create$ ?delivery-gates ?m:name))
    )

    (bind ?PROD-END-TIME (+ ?*PRODUCTION-TIME* ?*PRODUCTION-OVERTIME*))
    (bind ?deliver-period-end-time 0)
    (bind ?last-delivery-gate NONE)
    (while (< ?deliver-period-end-time ?PROD-END-TIME)
      (bind ?start-time ?deliver-period-end-time)
      (bind ?deliver-period-end-time
        (min (+ ?start-time (random ?*DELIVERY-GATE-MIN-TIME* ?*DELIVERY-GATE-MAX-TIME*))
	     ?PROD-END-TIME))
      (if (>= ?deliver-period-end-time (- ?PROD-END-TIME ?*DELIVERY-GATE-MIN-TIME*))
      ; expand this delivery gates' time
        then (bind ?deliver-period-end-time ?PROD-END-TIME))
      (bind ?candidates (delete-member$ ?delivery-gates ?last-delivery-gate))
      (bind ?delivery-gate (nth$ (random 1 (length$ ?candidates)) ?candidates))
      (bind ?last-delivery-gate ?delivery-gate)
      (assert (delivery-period (delivery-gates ?delivery-gate
					       (machine-magenta-for-cyan-gate ?delivery-gate))
			       (period ?start-time ?deliver-period-end-time)))
    )
  )

  ; assign random down times
  (if ?*RANDOMIZE-GAME* then
    (bind ?num-down-times (random ?*DOWN-NUM-MIN* ?*DOWN-NUM-MAX*))
    (bind ?candidates
	  (find-all-facts ((?m machine))
	    (and (neq ?m:mtype DELIVER) (neq ?m:mtype RECYCLE) (eq ?m:team CYAN))))
    (bind ?candidates (subseq$ (randomize$ ?candidates) 1 ?num-down-times))
    (foreach ?c ?candidates
      (bind ?duration (random ?*DOWN-TIME-MIN* ?*DOWN-TIME-MAX*))
      (bind ?start-time (random 1 (- ?*PRODUCTION-TIME* ?duration)))
      (bind ?end-time (+ ?start-time ?duration))
      (modify ?c (down-period ?start-time ?end-time))
    )

    ; assign down-time to recycling machine
    (bind ?recycling-down-time  (random ?*RECYCLE-DOWN-TIME-MIN* ?*RECYCLE-DOWN-TIME-MAX*))
    (bind ?recycling-down-start (random 1 (- ?*PRODUCTION-TIME* ?recycling-down-time)))
    (bind ?recycling-down-end   (+ ?recycling-down-start ?recycling-down-time))
    (delayed-do-for-all-facts ((?m-rec machine)) (eq ?m-rec:mtype RECYCLE)
      (modify ?m-rec (down-period ?recycling-down-start ?recycling-down-end))
    )

    ;(printout t "Assigning processing times to machines" crlf)
    (delayed-do-for-all-facts ((?mspec machine-spec)) TRUE
      (bind ?proc-time (random ?mspec:proc-time-min ?mspec:proc-time-max))
      (modify ?mspec (proc-time ?proc-time))
    )
  )

  ; Copy randomization from CYAN to MAGENTA
  (loop-for-count (?i 12) do
    (bind ?m-cyan-name    (sym-cat M ?i))
    (bind ?m-magenta-name (sym-cat M (+ ?i 12)))
    (do-for-fact ((?m-cyan machine) (?m-magenta machine))
      (and (eq ?m-cyan:name ?m-cyan-name) (eq ?m-magenta:name ?m-magenta-name))
      (modify ?m-magenta (mtype ?m-cyan:mtype) (down-period ?m-cyan:down-period))
    )
  )

  ; Swap some machines
  (switch ?*TOURNAMENT-PHASE*
    (case ROUND-ROBIN then
      (bind ?mtype-to-swap (pick-random$ ?*MACHINE-SWAP-ROUND-ROBIN*))
      (printout t "Round-robin: chose " ?mtype-to-swap " for machine swap" crlf)
      (delayed-do-for-all-facts ((?m-cyan machine) (?m-magenta machine))
        (and (eq ?m-cyan:team CYAN) (eq ?m-magenta:team MAGENTA)
	     (eq ?m-cyan:mtype ?mtype-to-swap)
	     (eq ?m-magenta:name (machine-magenta-for-cyan-field ?m-cyan:name)))

	(printout t "Swapping " ?m-cyan:name " and " ?m-magenta:name crlf)
        (modify ?m-cyan (team MAGENTA))
	(modify ?m-magenta (team CYAN))
      )
    )
    (case PLAY-OFFS then
      (bind ?candidates (create$))
      (do-for-all-facts ((?m machine))
	(and (eq ?m:team CYAN) (member$ ?m:mtype ?*MACHINE-SWAP-PLAY-OFFS-TYPES*))
	(bind ?candidates (append$ ?candidates ?m:name))
      )
      (bind ?candidates (subseq$ (randomize$ ?candidates) 1 ?*MACHINE-SWAP-PLAY-OFFS-NUM*))
      (printout t "Candidates: " ?candidates crlf)
      
      (delayed-do-for-all-facts ((?m-cyan machine) (?m-magenta machine))
        (and (eq ?m-cyan:team CYAN) (eq ?m-magenta:team MAGENTA)
	     (member$ ?m-cyan:name ?candidates)
	     (eq ?m-magenta:name (machine-magenta-for-cyan-field ?m-cyan:name)))

	(printout t "Swapping " ?m-cyan:name " and " ?m-magenta:name crlf)
        (modify ?m-cyan (team MAGENTA))
	(modify ?m-magenta (team CYAN))
      )

    )
    (case FINALS then
      (delayed-do-for-all-facts ((?m-cyan machine) (?m-magenta machine))
        (and (eq ?m-cyan:team CYAN) (eq ?m-magenta:team MAGENTA)
	     (member$ ?m-cyan:mtype ?*MACHINE-SWAP-FINALS-TYPES*)
	     (eq ?m-magenta:name (machine-magenta-for-cyan-field ?m-cyan:name)))

	(if (= (random 0 1) 1)
         then
	  (printout t "Swapping " ?m-cyan:name " and " ?m-magenta:name crlf)
          (modify ?m-cyan (team MAGENTA))
	  (modify ?m-magenta (team CYAN))
        )
      )
    )
    (default
      (printout error "Unknown tournament phase " ?*TOURNAMENT-PHASE* crlf)
    )
  )

  ; if PLC is disabled do not clear, we generated fake pucks
  (if (not (any-factp ((?c confval))
             (and (eq ?c:path "/llsfrb/sps/enable") (eq ?c:type BOOL) (eq ?c:value false))))
   then
    ; retract all pucks, they might have been auto-learned because they were
    ; still lying under the machine when the refbox was started
    (delayed-do-for-all-facts ((?p puck)) TRUE (retract ?p))
  )

  (assert (machines-initialized))
)

(defrule machines-reset-print
  (game-reset)
  ?mf <- (machines-printed)
  =>
  (retract ?mf)
)

(defrule machines-print
  (machines-initialized)
  (gamestate (teams $?teams) (phase PRODUCTION|EXPLORATION))
  (not (machines-printed))
  =>
  (assert (machines-printed))
  (bind ?t (if (eq ?teams (create$ "" "")) then t else debug))

  (bind ?pp-mach-assignment (create$))
  (do-for-all-facts ((?machine machine) (?mspec machine-spec))
    (eq ?machine:mtype ?mspec:mtype)

    (bind ?pp-mach-assignment
	  (append$ ?pp-mach-assignment
		   (sym-cat ?machine:name "/" ?machine:mtype "/" ?machine:team)))
  )
  (printout ?t "Machines: " ?pp-mach-assignment crlf)

  (do-for-all-facts ((?mspec machine-spec)) TRUE
    (do-for-fact ((?light-code machine-light-code)) (= ?light-code:id ?mspec:light-code)
      (printout ?t "Light code " ?light-code:code " for machine type " ?mspec:mtype crlf)
    )
  )

  (do-for-all-facts ((?m machine)) (> (nth$ 1 ?m:down-period) -1.0)
    (printout ?t ?m:name " down from "
		(time-sec-format (nth$ 1 ?m:down-period))
		" to " (time-sec-format (nth$ 2 ?m:down-period))
		" (" (- (nth$ 2 ?m:down-period) (nth$ 1 ?m:down-period)) " sec)" crlf)
  )

  (do-for-all-facts ((?period delivery-period)) TRUE
    (printout ?t "Deliver time " ?period:delivery-gates ": "
	      (time-sec-format (nth$ 1 ?period:period)) " to "
	      (time-sec-format (nth$ 2 ?period:period)) " ("
	      (- (nth$ 2 ?period:period) (nth$ 1 ?period:period)) " sec)" crlf)
  )

  (do-for-all-facts ((?mspec machine-spec)) TRUE
    (printout ?t "Proc time for " ?mspec:mtype " will be " ?mspec:proc-time " sec" crlf)
  )

)
