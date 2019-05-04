
;---------------------------------------------------------------------------
;  game.clp - LLSF RefBox CLIPS game maintenance
;
;  Created: Tue Jun 11 15:19:25 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction game-calc-phase-points (?team-color ?phase)
  (bind ?phase-points 0)
  (do-for-all-facts ((?p points)) (and (eq ?p:phase ?phase) (eq ?p:team ?team-color))
    (bind ?phase-points (+ ?phase-points ?p:points))
  )
  (return ?phase-points)
)

(deffunction game-calc-points (?team-color)
  (bind ?points 0)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (bind ?phase-points (game-calc-phase-points ?team-color ?phase))
    (bind ?points (+ ?points (max ?phase-points 0)))
  )
  (return ?points)
)

(defrule game-mps-solver-start
  "start the solver"
  (gamestate (game-time ?gt))
  (not (game-parameterized))
  ?mg <- (machine-generation (state NOT-STARTED))
  =>
  (printout t "starting the solver for the generation of the machine positions" crlf)
  (mps-generator-start)
  (modify ?mg (state STARTED) (generation-state-last-checked ?gt))
)

(defrule game-mps-solver-check
  "check if the solver is finished"
  (gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME) (game-time ?gt))
  ?mg <- (machine-generation (state STARTED) (generation-state-last-checked ?gs&:(timeout-sec ?gt ?gs ?*MACHINE-GENERATION-TIMEOUT-CHECK-STATE*)))
  =>
  (if (and (not (mps-generator-running)) (mps-generator-field-generated))
   then
    (printout t "the machine generation is finished" crlf)
    (modify ?mg (state FINISHED))
   else
    (printout warn "waiting for the generation of the machine positions" crlf)
    (modify ?mg (generation-state-last-checked ?gt))
  )
)

(defrule game-parameterize
  (gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
  (not (game-parameterized))
  (or (machine-generation (state FINISHED))
      (and (not (test (any-factp ((?m machine)) (eq ?m:zone TBD))))
           (confval (path "/llsfrb/game/random-field") (type BOOL) (value false))
      )
  )
  =>
  (assert (game-parameterized))

  (bind ?ring-colors (create$))
  (do-for-all-facts ((?rs ring-spec)) TRUE
    (bind ?ring-colors (append$ ?ring-colors ?rs:color))
  )
  (bind ?ring-colors (randomize$ ?ring-colors))
	(bind ?c1-first-ring (subseq$ ?ring-colors 1 1))
	(bind ?c2-first-ring (subseq$ ?ring-colors 2 2))
	(bind ?c3-first-ring (subseq$ ?ring-colors 3 3))

  ; machine assignment if not already done
  (if (not (any-factp ((?mi machines-initialized)) TRUE))
			then (machine-init-randomize ?ring-colors))

  ; reset orders, assign random times
  (delayed-do-for-all-facts ((?order order)) TRUE
    (bind ?deliver-start
      (random (nth$ 1 ?order:start-range)
              (nth$ 2 ?order:start-range)))
    (bind ?deliver-end
      (+ ?deliver-start (random (nth$ 1 ?order:duration-range)
																(nth$ 2 ?order:duration-range))))
		(if (and (> ?deliver-end ?*PRODUCTION-TIME*) (not ?order:allow-overtime))
		 then
		  (printout t "Revising deliver time (" ?deliver-start "-" ?deliver-end ") to ("
								(- ?deliver-start (- ?deliver-end ?*PRODUCTION-TIME*)) "-" ?*PRODUCTION-TIME* "), "
								"time shift: " (- ?deliver-end ?*PRODUCTION-TIME*) crlf)
			(bind ?deliver-start (- ?deliver-start (- ?deliver-end ?*PRODUCTION-TIME*)))
			(bind ?deliver-end ?*PRODUCTION-TIME*)
		)
    (bind ?activation-pre-time
          (random (nth$ 1 ?order:activation-range) (nth$ 2 ?order:activation-range)))
    (bind ?activate-at (max (- ?deliver-start ?activation-pre-time) 0))
		(if ?*RANDOMIZE-ACTIVATE-ALL-AT-START* then (bind ?activate-at 0))
    (bind ?gate (random 1 3))

		; check workpiece-assign-order rule in workpieces.clp for specific
		; assumptions for the 2016 game and order to workpiece assignment!
		(bind ?order-ring-colors (create$))
		(switch ?order:complexity
			;(case C0 then) ; for C0 we have nothing to do, no ring color
			(case C1 then (bind ?order-ring-colors (create$ ?c1-first-ring)))
			(case C2 then (bind ?order-ring-colors (create$ ?c2-first-ring (subseq$ (randomize$ (remove$ ?ring-colors ?c2-first-ring)) 1 1))))
			(case C3 then (bind ?order-ring-colors (create$ ?c3-first-ring (subseq$ (randomize$ (remove$ ?ring-colors ?c3-first-ring)) 1 2))))
    )

		(bind ?order-base-color (pick-random$ (deftemplate-slot-allowed-values order base-color)))
		(bind ?order-cap-color (pick-random$ (deftemplate-slot-allowed-values order cap-color)))

    (modify ?order (active FALSE) (activate-at ?activate-at) (delivery-gate ?gate)
	    (delivery-period ?deliver-start ?deliver-end) (base-color ?order-base-color)
			(ring-colors ?order-ring-colors) (cap-color ?order-cap-color))
  )

  ; Randomize number of required additional bases
  (bind ?m-add-bases (randomize$ (create$ 1 3)))
  (do-for-fact ((?ring ring-spec)) (eq ?ring:color (nth$ (nth$ 1 ?m-add-bases) ?ring-colors))
    (modify ?ring (req-bases 2))
  )
  (do-for-fact ((?ring ring-spec)) (eq ?ring:color (nth$ (nth$ 2 ?m-add-bases) ?ring-colors))
    (modify ?ring (req-bases 1))
  )
  (delayed-do-for-all-facts ((?ring ring-spec))
    (or (eq ?ring:color (nth$ 2 ?ring-colors)) (eq ?ring:color (nth$ 4 ?ring-colors)))
    (modify ?ring (req-bases 0))
  )

	; Randomly assign an order to be a competitive order
	(bind ?potential-competitive-orders (create$))
	(do-for-all-facts ((?order order))
	                  (and (eq ?order:complexity C0) ; must be C0
	                       (eq ?order:quantity-requested 1) ; must not request more than 1 product
	                       (eq ?order:allow-overtime FALSE) ; no overtime order
	                       (or (neq (nth$ 1 ?order:delivery-period) 0) ; no standing order
	                           (neq (nth$ 2 ?order:delivery-period) ?*PRODUCTION-TIME*))
	                  )
	                  (bind ?potential-competitive-orders (insert$ ?potential-competitive-orders 1 ?order:id))
	)
	;(printout t "Potential competitive orders: " ?potential-competitive-orders crlf)
	(bind ?competitive-order-id (nth$ (random 1 (length$ ?potential-competitive-orders)) ?potential-competitive-orders))
	(do-for-fact ((?order order)) (eq ?order:id ?competitive-order-id)
	  (modify ?order (competitive TRUE)))
)

(defrule game-print
  (game-parameterized)
  (gamestate (teams $?teams))
  (not (game-printed))
  =>
  (assert (game-printed))

  (bind ?t t)
  (if (neq ?teams (create$ "" ""))
   then
    (bind ?t debug)
    (printout warn "Printing game config to debug log only" crlf)
  )

  ; Print orders
  (do-for-all-facts ((?order order)) TRUE
    (bind ?duration (- (nth$ 2 ?order:delivery-period) (nth$ 1 ?order:delivery-period)))
    (printout ?t "Order " ?order:id ": "
	      ?order:complexity " (" ?order:base-color "|" (implode$ ?order:ring-colors)
	      "|" ?order:cap-color ") from " (time-sec-format (nth$ 1 ?order:delivery-period))
	      " to " (time-sec-format (nth$ 2 ?order:delivery-period))
	      " (@" (time-sec-format ?order:activate-at) " ~" ?duration "s) "
	      "D" ?order:delivery-gate crlf)
  )

  ; Print required additional bases
  (do-for-all-facts ((?ring ring-spec)) TRUE
    (printout t "Ring color " ?ring:color " requires " ?ring:req-bases " additional bases" crlf)
  )

  ; Print required additional bases
  (do-for-all-facts ((?m machine)) (eq ?m:mtype RS)
    (printout t "RS " ?m:name " has colors " ?m:rs-ring-colors crlf)
  )

	; Print machine swapping info
	(do-for-all-facts ((?zs zone-swap)) TRUE
	  (printout warn "Swap " ?zs:m1-name " to zone " ?zs:m1-new-zone
							" and " ?zs:m2-name " to zone " ?zs:m2-new-zone crlf)
  )
)

(defrule game-update-gametime-points
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  ?gf <- (gamestate (phase SETUP|EXPLORATION|PRODUCTION|WHACK_A_MOLE_CHALLENGE|NAVIGATION_CHALLENGE)
		    (state RUNNING)
		    (game-time ?game-time) (cont-time ?cont-time)
		    (last-time $?last-time&:(neq ?last-time ?now)))
  ?st <- (sim-time (enabled ?sts) (estimate ?ste) (now $?sim-time)
		   (real-time-factor ?rtf) (last-recv-time $?lrt))
  (or (sim-time (enabled false))
      (gamestate (last-time $?last-time&:(neq ?last-time ?sim-time))))
  =>
  (bind ?points-cyan (game-calc-points CYAN))
  (bind ?points-magenta (game-calc-points MAGENTA))
  (bind ?now (get-time ?sts ?ste ?now ?sim-time ?lrt ?rtf))
  (bind ?timediff (time-diff-sec ?now ?last-time))
  (modify ?gf (game-time (+ ?game-time ?timediff)) (cont-time (+ ?cont-time ?timediff))
	  (last-time ?now) (points ?points-cyan ?points-magenta))
)

(defrule game-update-last-time
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  (or (gamestate (phase ~PRODUCTION&~EXPLORATION&~SETUP&~WHACK_A_MOLE_CHALLENGE&~NAVIGATION_CHALLENGE))
      (gamestate (state ~RUNNING)))
  ?st <- (sim-time (enabled ?sts) (estimate ?ste) (now $?sim-time)
		   (real-time-factor ?rtf) (last-recv-time $?lrt))
  ?gf <- (gamestate (last-time $?last-time&:(neq ?last-time ?now)))
  (or (sim-time (enabled false))
      (gamestate (last-time $?last-time&:(neq ?last-time ?sim-time))))
  =>
  (bind ?now (get-time ?sts ?ste ?now ?sim-time ?lrt ?rtf))
  (modify ?gf (last-time ?now))
)


(defrule game-switch-to-pre-game
  ?gs <- (gamestate (phase PRE_GAME) (prev-phase ~PRE_GAME))
  =>
  (modify ?gs (prev-phase PRE_GAME) (game-time 0.0) (state WAIT_START))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )
  ;(assert (reset-game))
)

(defrule game-start-training
  ?gs <- (gamestate (teams "" "") (phase PRE_GAME) (state RUNNING))
  =>
  (modify ?gs (phase EXPLORATION) (prev-phase PRE_GAME) (game-time 0.0) (start-time (now)))
  (assert (attention-message (text "Starting  *** TRAINING ***  game")))
)

(defrule game-start
  ?gs <- (gamestate (teams ?team_cyan ?team_magenta) (phase PRE_GAME) (state RUNNING))
  =>
  (modify ?gs (phase SETUP) (prev-phase PRE_GAME) (start-time (now)))
  (assert (attention-message (text "Starting setup phase") (time 15)))
)

(defrule game-setup-warn-end-near
  (gamestate (phase SETUP) (state RUNNING)
	     (game-time ?game-time&:(>= ?game-time (* ?*SETUP-TIME* .9))))
  (not (setup-warned))
  =>
  (assert (setup-warned))
  (assert (attention-message (text "Setup phase is about to end")))
)

(defrule game-switch-to-exploration
  ?gs <- (gamestate (phase SETUP) (state RUNNING)
		    (game-time ?game-time&:(>= ?game-time ?*SETUP-TIME*)))
  =>
  (modify ?gs (phase EXPLORATION) (prev-phase SETUP) (game-time 0.0))
  (assert (attention-message (text "Switching to exploration phase")))
)

(defrule game-switch-to-production
  ?gs <- (gamestate (phase EXPLORATION) (state RUNNING)
		    (game-time ?game-time&:(>= ?game-time ?*EXPLORATION-TIME*)))
  =>
  (modify ?gs (phase PRODUCTION) (prev-phase EXPLORATION) (game-time 0.0))
  (assert (attention-message (text "Switching to production phase")))
)

(deffunction game-print-points-team (?team)
  (bind ?points 0)
  (printout t "-- Awarded Points -- " ?team " --" crlf)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (if (any-factp ((?p points)) (and (eq ?phase ?p:phase) (eq ?team ?p:team) (> ?p:points 0)))
    then
      (printout t ?phase crlf)
      (bind ?phase-points 0)
      (do-for-all-facts ((?p points)) (and (eq ?p:team ?team) (eq ?p:phase ?phase))
        (printout t
          (format nil "  %s  %2d  %s" (time-sec-format ?p:game-time) ?p:points ?p:reason) crlf)
        (bind ?phase-points (+ ?phase-points ?p:points))
      )
      (printout t ?phase " TOTAL: " ?phase-points crlf)
      (bind ?points (+ ?points ?phase-points))
    )
  )
  (printout t "OVERALL TOTAL POINTS: " ?points crlf)
)

(deffunction game-print-points ()
  (game-print-points-team CYAN)
  (game-print-points-team MAGENTA)
)

(deffunction game-summary ()
  (game-print-points)
  (assert (attention-message (text "Game Over") (time 60)))
  (printout t "===  Game Over  ===" crlf)
)

(defrule game-over
  ?gs <- (gamestate (refbox-mode STANDALONE) (phase PRODUCTION) (state RUNNING)
		    (over-time FALSE) (points ?p-cyan ?p-magenta&:(<> ?p-cyan ?p-magenta))
		    (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION) (state PAUSED))
)

(defrule game-enter-overtime
  ?gs <- (gamestate (refbox-mode STANDALONE) (phase PRODUCTION) (state RUNNING)
		    (over-time FALSE) (points ?p-cyan ?p-magenta&:(= ?p-cyan ?p-magenta))
		    (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  =>
  (assert (attention-message (text "Entering over-time") (time 15)))
  (modify ?gs (over-time TRUE))
)

(defrule game-over-after-overtime
  ?gs <- (gamestate (refbox-mode STANDALONE) (phase PRODUCTION) (state RUNNING)
		    (over-time TRUE)
		    (game-time ?gt&:(>= ?gt (+ ?*PRODUCTION-TIME* ?*PRODUCTION-OVERTIME*))))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION) (state PAUSED) (end-time (now)))
)

(defrule game-goto-post-game
  ?gs <- (gamestate (phase POST_GAME) (prev-phase ~POST_GAME))
  =>
  (modify ?gs (prev-phase POST_GAME) (end-time (now)))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights RED-BLINK))
  )
  (if (any-factp ((?pd referee-confirmation)) TRUE) then
    (assert (attention-message (text "Game ended, please confirm deliveries!")))
    (assert (postgame-for-unconfirmed-deliveries))
  else
    (game-summary)
  )
)

(defrule game-over-on-finalize
	"Switch to post-game if the refbox is stopped"
	(finalize)
	?gs <- (gamestate (phase ~POST_GAME))
	=>
	(modify ?gs (phase POST_GAME))
)

(defrule game-postgame-no-unconfirmed-deliveries
  ?w <- (postgame-for-unconfirmed-deliveries)
  (not (referee-confirmation))
  =>
  (retract ?w)
  (game-summary)
)

(deffunction game-reset ()
	; Retract machine initialization state, if set
	(do-for-fact ((?mi machines-initialized)) TRUE
		(retract ?mi)
	)
	; Retract all delivery periods
	(delayed-do-for-all-facts ((?dp delivery-period)) TRUE
		(retract ?dp)
	)
	; Reset all down periods
	(delayed-do-for-all-facts ((?m machine)) TRUE
		(modify ?m (down-period (deftemplate-slot-default-value machine down-period)))
	)
	; Reset game parameterization
	(do-for-fact ((?gp game-parameterized)) TRUE
		(retract ?gp)
	)
	; Reset machine generation
	(do-for-fact ((?mg machine-generation)) TRUE
		(modify ?mg (state NOT-STARTED))
	)
	; Print machines again
	(do-for-fact ((?mp machines-printed)) TRUE
		(retract ?mp)
	)
	; Print game info again
	(do-for-fact ((?gp game-printed)) TRUE
		(retract ?gp)
	)
)

(defrule game-reset
	"Allow calling the function via fact to avoid file loading order issues"
	?gr <- (game-reset)
	=>
	(retract ?gr)
	(game-reset)
)
