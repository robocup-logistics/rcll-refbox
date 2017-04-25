
;---------------------------------------------------------------------------
;  machines.clp - LLSF RefBox CLIPS machine processing
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
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
	(if (member$ RED-ON ?dl) then (bind ?red-state ON)
	 else (if (member$ RED-BLINK ?dl) then (bind ?red-state BLINK)
	 else (bind ?red-state OFF)))
	(if (member$ YELLOW-ON ?dl) then (bind ?yellow-state ON)
	 else (if (member$ YELLOW-BLINK ?dl) then (bind ?yellow-state BLINK)
	 else (bind ?yellow-state OFF)))
	(if (member$ GREEN-ON ?dl) then (bind ?green-state ON)
	 else (if (member$ GREEN-BLINK ?dl) then (bind ?green-state BLINK)
	 else (bind ?green-state OFF)))

	(mps-set-lights (str-cat ?m) (str-cat ?red-state) (str-cat ?yellow-state) (str-cat ?green-state))
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
		(bind ?zones-cyan ?*MACHINE-ZONES-CYAN*)
    ; Remove all zones for which a machine has already been assigned
		(do-for-all-facts ((?m machine)) (neq ?m:zone TBD)
      (bind ?zones-cyan (delete-member$ ?zones-cyan ?m:zone))
	  )
    ; TODO 2017 include generator for positions
		(delayed-do-for-all-facts ((?m-cyan machine))
      (and (eq ?m-cyan:team CYAN) (eq ?m-cyan:zone TBD))
      
      ; bind zones to machines here 
      ;(bind ?zone (nth$ 1 ?zones-cyan))
      ;(printout t "CYAN Machine " ?m-cyan:name " is in zone " ?zone crlf)
      ;(modify ?m-cyan (zone ?zone))

      ;(do-for-fact ((?m-magenta machine))
      ;  (eq ?m-magenta:name (machine-magenta-for-cyan ?m-cyan:name))
      ;  (printout t "MAGENTA Machine " ?m-magenta:name " is in zone " (zone-magenta-for-cyan ?zone) crlf)
      ;  (modify ?m-magenta (zone (zone-magenta-for-cyan ?zone)))
      ;)
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

; TODO 2017
;  (do-for-all-facts ((?m machine)) TRUE
;    (do-for-fact ((?light-code machine-light-code)) (= ?light-code:id ?m:exploration-light-code)
;      (printout ?t "Light code " ?light-code:code " for machine " ?m:name crlf)
;    )
;  )

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

(defrule machine-pb-recv-SetMachineLights
  ?pf <- (protobuf-msg (type "llsf_msgs.SetMachineLights") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
	(bind ?lights OFF OFF OFF)

	(foreach ?l (pb-field-list ?p "lights")
		(bind ?idx 0)
		(switch (sym-cat (pb-field-value ?l "color"))
			(case RED then (bind ?idx 1))
			(case YELLOW then (bind ?idx 2))
			(case GREEN then (bind ?idx 3))
		)
		(bind ?lights (replace$ ?lights ?idx ?idx (sym-cat (pb-field-value ?l "state"))))
	)
  (printout t "Received lights " ?lights " for machine " ?mname crlf)
	(bind ?lights (replace$ ?lights 1 1 (sym-cat RED- (nth$ 1 ?lights))))
	(bind ?lights (replace$ ?lights 2 2 (sym-cat YELLOW- (nth$ 2 ?lights))))
	(bind ?lights (replace$ ?lights 3 3 (sym-cat GREEN- (nth$ 3 ?lights))))
	
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
		(modify ?m (desired-lights ?lights))
  )
)
