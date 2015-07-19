
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
  (mps-reset (str-cat ?m))
)


(defrule machine-init
  (init)
  =>
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (mps-reset ?machine:name)
  )
)

(defrule machine-lights "Set machines if desired lights differ from actual lights"
  ?mf <- (machine (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  ;(printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
  (modify ?mf (actual-lights ?dl))
  (foreach ?color (create$ RED YELLOW GREEN)
    (if (member$ (sym-cat ?color "-ON") ?dl)
    then 
      (mps-set-light (str-cat ?m) (str-cat ?color) "ON")
    else
      (if (member$ (sym-cat ?color "-BLINK") ?dl)
      then
        (mps-set-light (str-cat ?m) (str-cat ?color) "BLINK")
      else
        (mps-set-light (str-cat ?m) (str-cat ?color) "OFF")
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

(deffunction machine-opposite-team (?m-name)
	(bind ?team (sub-string 1 1 ?m-name))
	(return (sym-cat (if (eq ?team "C") then "M" else "C")
									 (sub-string 2 (str-length ?m-name) ?m-name)))
)

(deffunction machine-opposite-zone (?z-name)
	(if (member$ ?z-name ?*MACHINE-ZONES-CYAN*)
   then
	  (bind ?idx (member$ ?z-name ?*MACHINE-ZONES-CYAN*))
		(return (nth$ ?idx ?*MACHINE-ZONES-MAGENTA*))
   else
	  (bind ?idx (member$ ?z-name ?*MACHINE-ZONES-MAGENTA*))
		(return (nth$ ?idx ?*MACHINE-ZONES-CYAN*))
  )
)

(deffunction machine-init-randomize (?ring-colors)
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
    (if (eq ?machine:mtype RS) then (mps-reset-base-counter (str-cat ?machine:name)))
    (modify ?machine (productions 0) (state IDLE)
	             (proc-start 0.0) (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )

	(if (any-factp ((?m machine)) (eq ?m:zone TBD))
   then
    (printout t "Randomizing from scratch" crlf)
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

   else
	  (printout t "Performing " ?*RANDOMIZE-STEPS-MACHINES*
							" randomization steps on loaded config" crlf)
	  (loop-for-count ?*RANDOMIZE-STEPS-MACHINES*
      ; collect all machines on cyan side
      (bind ?cyan-side-machines (create$))
			(do-for-all-facts ((?m machine)) TRUE
				(if
				  (and (member$ ?m:zone ?*MACHINE-ZONES-CYAN*)
							 (member$ (sym-cat (sub-string 3 4 ?m:name)) ?*MACHINE-RANDOMIZE-TYPES*))
         then
          (bind ?cyan-side-machines (append$ ?cyan-side-machines ?m:name))
        )
      )
			; decide on randomization step
      (bind ?rm (pick-random$ ?cyan-side-machines))
      (if (> (random 1 10) ?*RANDOMIZE-INTER-SIDE-SWAP-PROB*)
       then
        ; swap with another zone on the same side
			 (bind ?candidates ?*MACHINE-ZONES-CYAN*)
			 ; Remove machine itself
			 (do-for-fact ((?m machine)) (eq ?m:name ?rm)
				 (bind ?idx (member$ ?m:zone ?candidates))
         (bind ?candidates (delete$ ?candidates ?idx ?idx))
       )

			 ; Remove DS and BS zones
			 (do-for-all-facts ((?m machine))
				 (and (eq ?m:team CYAN) (or (eq ?m:mtype BS) (eq ?m:mtype DS)))

				 (bind ?idx (member$ ?m:zone ?candidates))
         (bind ?candidates (delete$ ?candidates ?idx ?idx))
			 )

			 (bind ?swap-zone (pick-random$ ?candidates))

			 (if (any-factp ((?m machine)) (eq ?m:zone ?swap-zone))
        then
				 (do-for-fact ((?m machine)) (eq ?m:zone ?swap-zone)
			 	   (bind ?swap-m ?m:name)
         )

         (printout t "On-side dual-machine swap " ?rm " with " ?swap-m crlf)

				 (do-for-fact ((?m1 machine) (?m2 machine))
				 	(and (eq ?m1:name ?rm) (eq ?m2:name ?swap-m))

					(modify ?m1 (zone ?m2:zone))
					(modify ?m2 (zone ?m1:zone))
					(assert (zone-swap (m1-name ?m1:name) (m1-new-zone ?m2:zone)
														 (m2-name ?m2:name) (m2-new-zone ?m1:zone)))
         )

				 (do-for-fact ((?m1 machine) (?m2 machine))
					 (and (eq ?m1:name (machine-opposite-team ?rm))
								(eq ?m2:name (machine-opposite-team ?swap-m)))

					 (modify ?m1 (zone ?m2:zone))
					 (modify ?m2 (zone ?m1:zone))
					 (assert (zone-swap (m1-name ?m1:name) (m1-new-zone ?m2:zone)
															(m2-name ?m2:name) (m2-new-zone ?m1:zone)))
				 )
        else

         (printout t "On-side single-machine swap " ?rm " to zone " ?swap-zone crlf)

				 (bind ?m2-swap-zone (machine-opposite-zone ?swap-zone))

				 (do-for-fact ((?m1 machine) (?m2 machine))
					 (and (eq ?m1:name ?rm)
								(eq ?m2:name (machine-opposite-team ?rm)))


					(modify ?m1 (zone ?swap-zone))
					(modify ?m2 (zone ?m2-swap-zone))
					(assert (zone-swap (m1-name ?m1:name) (m1-new-zone ?swap-zone)
														 (m2-name ?m2:name) (m2-new-zone ?m2-swap-zone)))
         )
       )
       else
        (printout t "Inter-side swap " ?rm crlf)
        ; swap machine with other field side and matching machine
			  ; of the same type
				(bind ?m1-team (sub-string 1 1 ?rm))
				(bind ?m-type (sym-cat (sub-string 3 4 ?rm)))
				(bind ?m1-num  (sym-cat (sub-string 5 5 ?rm)))
				(bind ?m2-team (if (eq ?m1-team "C") then "M" else "C"))
				(bind ?m3-num  (if (eq ?m1-num "1") then "2" else "1"))

				(bind ?m1-name (sym-cat ?m1-team "-" ?m-type ?m1-num))
				(bind ?m2-name (sym-cat ?m2-team "-" ?m-type ?m1-num))

				(bind ?m3-name (sym-cat ?m1-team "-" ?m-type ?m3-num))
				(bind ?m4-name (sym-cat ?m2-team "-" ?m-type ?m3-num))

				(do-for-fact ((?m1 machine) (?m2 machine))
					(and (eq ?m1:name ?m1-name) (eq ?m2:name ?m2-name))
          (modify ?m1 (zone ?m2:zone))
          (modify ?m2 (zone ?m1:zone))
					;(printout t "M1/M2: Swapping " ?m1-name " with " ?m2-name crlf)
					(assert (zone-swap (m1-name ?m1:name) (m1-new-zone ?m2:zone)
														 (m2-name ?m2:name) (m2-new-zone ?m1:zone)))
        )

				(do-for-fact ((?m3 machine) (?m4 machine))
					(and (eq ?m3:name ?m3-name) (eq ?m4:name ?m4-name))
          (modify ?m3 (zone ?m4:zone))
          (modify ?m4 (zone ?m3:zone))
					;(printout t "M3/M4: Swapping " ?m3-name " with " ?m4-name crlf)
					(assert (zone-swap (m1-name ?m3:name) (m1-new-zone ?m4:zone)
														 (m2-name ?m4:name) (m2-new-zone ?m3:zone)))
        )
      )
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

  ; Calculate exploration hashes
  (delayed-do-for-all-facts ((?m-cyan machine) (?m-magenta machine))
    (and (eq ?m-cyan:team CYAN) (eq ?m-magenta:team MAGENTA)
	 (eq ?m-magenta:name (machine-magenta-for-cyan ?m-cyan:name)))

    (bind ?rs (gen-random-string 8))

    (printout t "Machines " ?m-cyan:name "/" ?m-magenta:name " exploration string:" ?rs crlf)
    (modify ?m-cyan (exploration-type ?rs))
    (modify ?m-magenta (exploration-type ?rs))
  )

  ; Randomize ring colors per machine
  (do-for-fact ((?m-cyan machine) (?m-magenta machine))
    (and (eq ?m-cyan:name C-RS1) (eq ?m-magenta:name M-RS1))

    (modify ?m-cyan    (rs-ring-colors (subseq$ ?ring-colors 1 2)))
    (modify ?m-magenta (rs-ring-colors (subseq$ ?ring-colors 1 2)))
  )
  (do-for-fact ((?m-cyan machine) (?m-magenta machine))
    (and (eq ?m-cyan:name C-RS2) (eq ?m-magenta:name M-RS2))

    (modify ?m-cyan    (rs-ring-colors (subseq$ ?ring-colors 3 4)))
    (modify ?m-magenta (rs-ring-colors (subseq$ ?ring-colors 3 4)))
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
)
