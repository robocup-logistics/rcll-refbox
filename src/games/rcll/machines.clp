
; Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

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
	(gamestate (phase POST_GAME))
  ?ml <- (machine-lights (name ?m) (desired-lights $?dl&:(> (length$ ?dl) 0)))
  =>
  (modify ?ml (desired-lights))
  (mps-reset (str-cat ?m))
)

(defrule machine-meta-init
  (declare (salience ?*PRIORITY_HIGH*))
  (machine (name ?n) (mtype ?type))
  (not (bs-meta (name ?n)))
  (not (rs-meta (name ?n)))
  (not (cs-meta (name ?n)))
  (not (ds-meta (name ?n)))
  (not (ss-meta (name ?n)))
  =>
  (str-assert (str-cat "(" (lowcase ?type) "-meta (name " ?n "))"))
)

(defrule machine-meta-retract
  (declare (salience ?*PRIORITY_HIGH*))
  (or ?mf <- (bs-meta (name ?n))
      ?mf <- (rs-meta (name ?n))
      ?mf <- (cs-meta (name ?n))
      ?mf <- (ds-meta (name ?n))
      ?mf <- (ss-meta (name ?n))
  )
  (not (machine (name ?n)))
  =>
  (retract ?mf)
)

(defrule machine-lights "Set machines if desired lights differ from actual lights"
  ?ml <- (machine-lights (name ?m) (actual-lights $?al) (desired-lights $?dl&:(neq ?al ?dl)))
  =>
  ;(printout t ?m " actual lights: " ?al "  desired: " ?dl crlf)
  (modify ?ml (actual-lights ?dl))
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

(deffunction machine-team (?machine-name)
	(bind ?prefix (sub-string 1 1 (str-cat ?machine-name)))
	(return (if (eq ?prefix "C") then CYAN else MAGENTA))
)

(deffunction machine-retrieve-generated-mps ()
	(bind ?machines (mps-generator-get-generated-field))
	(delayed-do-for-all-facts ((?m machine)) (and (eq ?m:team CYAN) (eq ?m:zone TBD))
		(if (member$ ?m:name ?machines)
		 then
			(bind ?zone (nth$ (+ (member$ ?m:name ?machines) 1) ?machines))
			(bind ?rot (nth$ (+ (member$ ?m:name ?machines) 2) ?machines))
			(printout t ?m:name ": " ?zone " with " ?rot crlf)
			(modify ?m (zone ?zone) (rotation ?rot))
		 else
			(printout t ?m:name " not found in generation, skipping" crlf)
			(retract ?m)
		)
	)
	; Mirror machines for other team
	(delayed-do-for-all-facts ((?mm machine)) (eq ?mm:team CYAN)
		(do-for-fact ((?mc machine)) (and (eq ?mm:name (mirror-name ?mc:name)) (eq ?mc:team MAGENTA))
			(modify ?mc
			  (zone (mirror-zone ?mm:zone))
			  (rotation (mirror-orientation ?mm:mtype ?mm:zone ?mm:rotation))
			)
		)
	)
	(delayed-do-for-all-facts ((?m machine)) (eq ?m:zone TBD)
		(printout t ?m:name " not found in generation, skipping" crlf)
		(retract ?m)
	)
)

(deffunction machine-randomize-positions ()
	(printout t "Randomizing from scratch" crlf)
	; reset all zones, since we cannot do partial assignment anymore
	(delayed-do-for-all-facts ((?m machine)) TRUE
	  (modify ?m (zone TBD))
	)
	; randomly assigned machines to zones using the external generator
	(bind ?zones-magenta ?*MACHINE-ZONES-MAGENTA*)
	(machine-retrieve-generated-mps)

	(if ?*FIELD-MIRRORED* then
		; Swap machines
		(bind ?machines-to-swap
		(create$ (str-cat "RS" (random 1 2)) (str-cat "CS" (random 1 2))))
		(foreach ?ms ?machines-to-swap
			(do-for-fact ((?m-cyan machine) (?m-magenta machine))
			    (and (eq ?m-cyan:team CYAN) (eq ?m-cyan:name (sym-cat C- ?ms))
			         (eq ?m-magenta:team MAGENTA) (eq ?m-magenta:name (sym-cat M- ?ms)))

			  (printout t "Swapping " ?m-cyan:name " with " ?m-magenta:name crlf)

			  (bind ?z-cyan ?m-cyan:zone)
			  (bind ?r-cyan ?m-cyan:rotation)
			  (bind ?z-magenta ?m-magenta:zone)
			  (bind ?r-magenta ?m-magenta:rotation)
			  (modify ?m-cyan    (zone ?z-magenta) (rotation ?r-magenta))
			  (modify ?m-magenta (zone ?z-cyan) (rotation ?r-cyan))
			)
		)
	)
)

(deffunction machine-randomize-machine-setup ()
	; Randomize ring colors per machine
	;(bind ?ring-colors (create$))
	;(do-for-all-facts ((?rs ring-spec)) TRUE
	;  (bind ?ring-colors (append$ ?ring-colors ?rs:color))
	;)
	;(do-for-fact ((?m-cyan machine) (?m-magenta machine))
	;  (and (eq ?m-cyan:name C-RS1) (eq ?m-magenta:name M-RS1))
	;  (modify ?m-cyan    (available-colors (subseq$ ?ring-colors 1 2)))
	;  (modify ?m-magenta (available-colors (subseq$ ?ring-colors 1 2)))
	;)
	;(do-for-fact ((?m-cyan machine) (?m-magenta machine))
	;  (and (eq ?m-cyan:name C-RS2) (eq ?m-magenta:name M-RS2))
	;  (modify ?m-cyan    (available-colors (subseq$ ?ring-colors 3 4)))
	;  (modify ?m-magenta (available-colors (subseq$ ?ring-colors 3 4)))
	;)
	; No need to randomize color assignment, the color costs are randomized
	; anyways
	(do-for-fact ((?m-cyan rs-meta) (?m-magenta rs-meta))
	  (and (eq ?m-cyan:name C-RS1) (eq ?m-magenta:name M-RS1))
	  (modify ?m-cyan    (available-colors RING_ORANGE RING_GREEN))
	  (modify ?m-magenta (available-colors RING_ORANGE RING_GREEN))
	)
	(do-for-fact ((?m-cyan rs-meta) (?m-magenta rs-meta))
	  (and (eq ?m-cyan:name C-RS2) (eq ?m-magenta:name M-RS2))
	  (modify ?m-cyan    (available-colors RING_BLUE RING_YELLOW))
	  (modify ?m-magenta (available-colors RING_BLUE RING_YELLOW))
	)

	; assign random down times
  (if (and ?*RANDOMIZE-GAME*
           (config-get-bool "/llsfrb/game/random-machine-down-times"))
   then
		(bind ?candidates (create$))
		(foreach ?t ?*DOWN-TYPES*
			(bind ?t-candidates (find-all-facts ((?m machine))
			                      (and (eq ?m:mtype ?t) (eq ?m:team CYAN))))
			(if (> (length$ ?t-candidates) 0) then
				(bind ?candidates (append$ ?candidates (first$ (randomize$ ?t-candidates))))
			)
		)

		(foreach ?c ?candidates
			(bind ?duration (random ?*DOWN-TIME-MIN* ?*DOWN-TIME-MAX*))
			(bind ?start-time (random ?*EXPLORATION-TIME* (- ?*PRODUCTION-TIME* ?duration)))
			(bind ?end-time (+ ?start-time ?duration))

			; Copy to magenta machine
			(do-for-fact ((?m-magenta machine))
			  (eq ?m-magenta:name (machine-magenta-for-cyan (fact-slot-value ?c name)))
			  (modify ?m-magenta (down-period ?start-time ?end-time))
			)

		  (modify ?c (down-period ?start-time ?end-time))
		)
	)
)

(defrule machines-print
  (game-parameters (is-parameterized TRUE))
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

(defrule machine-SetMachinePose
  ?m-fact <- (machine (name ?name) (zone ?old-zone) (rotation ?old-rotation))
  ?cmd <- (production-SetMachinePose ?str-name&:(eq ?name (sym-cat ?str-name)) ?rotation ?str-zone)
  =>
  (printout t "Moving " ?str-name " from " ?old-zone ", " ?old-rotation " to " ?str-zone ", " ?rotation crlf)
  (bind ?new-zone (sym-cat ?str-zone))
  (bind ?new-rotation ?rotation)
  (if (neq ?new-zone ?old-zone) then
    (delayed-do-for-all-facts ((?m machine)) (eq ?m:zone ?new-zone)
      (printout t "Swapping with " ?m:name crlf)
      (modify ?m (zone ?old-zone))
    )
  )
  (modify ?m-fact (zone ?new-zone)
    (rotation ?new-rotation)
  )
  (retract ?cmd)
)
