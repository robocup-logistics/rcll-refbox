
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

(deffunction zone-magenta-for-cyan (?cyan-zone)
  (return (nth$ (member$ ?cyan-zone ?*MACHINE-ZONES-CYAN*) ?*MACHINE-ZONES-MAGENTA*))
)

(deffunction machine-magenta-for-cyan (?m-cyan)
  (return (sym-cat M- (sub-string 3 (str-length ?m-cyan) ?m-cyan)))
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
    (delayed-do-for-all-facts ((?m-cyan machine)) (eq ?m-cyan:team CYAN)
      ;(do-for-fact ((?light-code machine-light-code)) (= ?light-code:id (nth$ 1 ?light-codes))
      ;  (printout t "Light code " ?light-code:code " for machine " ?m-cyan:name crlf)
      ;)
      (modify ?m-cyan (exploration-light-code (nth$ 1 ?light-codes)))

      (do-for-fact ((?m-magenta machine))
        (eq ?m-magenta:name (machine-magenta-for-cyan ?m-cyan:name))
	(modify ?m-magenta (exploration-light-code (nth$ 1 ?light-codes)))
      )

      (bind ?light-codes (delete$ ?light-codes 1 1))
    )
  )

  ; reset machines
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (loaded-with 0) (productions 0) (state IDLE)
	             (proc-start 0.0) (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )

  ; randomly assigned machines to zones
  (bind ?zones-cyan (randomize$ ?*MACHINE-ZONES-CYAN*))
  ; Remove all zones for which a machine has already been assigned
  (do-for-all-facts ((?m machine)) (neq ?m:zone TBD)
    (bind ?zones-cyan (delete-member$ ?zones-cyan ?m:zone))
  )
  (delayed-do-for-all-facts ((?m-cyan machine))
    (and (eq ?m-cyan:team CYAN) (eq ?m-cyan:zone TBD))

    (bind ?zone (nth$ 1 ?zones-cyan))
    (bind ?zones-cyan (delete$ ?zones-cyan 1 1))
    (printout t "CYAN Machine " ?m-cyan:name " is in zone " ?zone crlf)
    (modify ?m-cyan (zone ?zone))

    (do-for-fact ((?m-magenta machine))
      (eq ?m-magenta:name (machine-magenta-for-cyan ?m-cyan:name))
      (printout t "MAGENTA Machine " ?m-magenta:name " is in zone " (zone-magenta-for-cyan ?zone) crlf)
      (modify ?m-magenta (zone (zone-magenta-for-cyan ?zone)))
    )
  )

  ; assign random down times
  (if ?*RANDOMIZE-GAME* then
    (bind ?candidates (create$))
    (foreach ?t ?*DOWN-TYPES*
      (bind ?t-candidates (find-all-facts ((?m machine))
    	      (and (eq ?m:mtype ?t) (eq ?m:team CYAN))))

      (bind ?candidates (append$ ?candidates (first$ (randomize$ ?t-candidates))))
    )

    (foreach ?c ?candidates
      (bind ?duration (random ?*DOWN-TIME-MIN* ?*DOWN-TIME-MAX*))
      (bind ?start-time (random 1 (- ?*PRODUCTION-TIME* ?duration)))
      (bind ?end-time (+ ?start-time ?duration))

      ; Copy to magenta machine
      (do-for-fact ((?m-magenta machine))
        (eq ?m-magenta:name (machine-magenta-for-cyan (fact-slot-value ?c name)))
        (modify ?m-magenta (down-period ?start-time ?end-time))
      )

      (modify ?c (down-period ?start-time ?end-time))
    )
  )

  ; Swap machines
  (bind ?machines-to-swap
	(create$ (str-cat "RS" (random 1 2)) (str-cat "CS" (random 1 2))))
  (foreach ?ms ?machines-to-swap
    (do-for-fact ((?m-cyan machine) (?m-magenta machine))
      (and (eq ?m-cyan:team CYAN) (eq ?m-cyan:name (sym-cat C- ?ms))
	   (eq ?m-magenta:team MAGENTA) (eq ?m-magenta:name (sym-cat M- ?ms)))

      (printout t "Swapping " ?m-cyan:name " with " ?m-magenta:name crlf)

      (bind ?z-cyan ?m-cyan:zone)
      (bind ?z-magenta ?m-magenta:zone)
      (modify ?m-cyan    (zone ?z-magenta))
      (modify ?m-magenta (zone ?z-cyan))
    )
  )

  ; Calculate exploration hashes
  (delayed-do-for-all-facts ((?m-cyan machine) (?m-magenta machine))
    (and (eq ?m-cyan:team CYAN) (eq ?m-magenta:team MAGENTA)
	 (eq ?m-magenta:name (machine-magenta-for-cyan ?m-cyan:name)))

    (bind ?rs (gen-random-string 8))

    (printout t "Machines " ?m-cyan:name "/" ?m-magenta:name " exploration string:" ?rs crlf)
    (modify ?m-cyan (exploration-type ?rs))
    (modify ?m-magenta (exploration-type ?rs))
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

  (do-for-all-facts ((?m machine)) TRUE
    (do-for-fact ((?light-code machine-light-code)) (= ?light-code:id ?m:exploration-light-code)
      (printout ?t "Light code " ?light-code:code " for machine " ?m:name crlf)
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
