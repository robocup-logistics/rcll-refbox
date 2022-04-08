
;---------------------------------------------------------------------------
;  game.clp - LLSF RefBox CLIPS game maintenance
;
;  Created: Tue Jun 11 15:19:25 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;             2019       Mostafa Gomaa
;             2020       Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction game-randomize-orders ()
	(bind ?ring-colors (create$))
	(do-for-all-facts ((?rs ring-spec)) TRUE
	  (bind ?ring-colors (append$ ?ring-colors ?rs:color))
	)
	(bind ?ring-colors (randomize$ ?ring-colors))
	(bind ?c1-first-ring (subseq$ ?ring-colors 1 1))
	(bind ?c2-first-ring (subseq$ ?ring-colors 2 2))
	(bind ?c3-first-ring (subseq$ ?ring-colors 3 3))
	(bind ?cx-first-ring (subseq$ ?ring-colors 4 4))

	(bind ?c1-counter 0)
	(bind ?c2-counter 0)
	(bind ?c3-counter 0)
	; reset orders, assign random times
	(delayed-do-for-all-facts ((?order order)) TRUE
		(bind ?deliver-start (random (nth$ 1 ?order:start-range)
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
			(case C1 then (bind ?c1-counter (+ ?c1-counter 1))
			              (if (<= ?c1-counter 1) then (bind ?first-ring ?c1-first-ring)
			                                     else (bind ?first-ring ?cx-first-ring))
			              (bind ?order-ring-colors (create$ ?first-ring)))
			(case C2 then (bind ?c2-counter (+ ?c2-counter 1))
			              (if (<= ?c2-counter 1) then (bind ?first-ring ?c2-first-ring)
			                                     else (bind ?first-ring ?cx-first-ring))
			              (bind ?order-ring-colors (create$ ?first-ring (subseq$ (randomize$ (remove$ ?ring-colors ?first-ring)) 1 1))))
			(case C3 then (bind ?c3-counter (+ ?c3-counter 1))
			              (if (<= ?c3-counter 1) then (bind ?first-ring ?c3-first-ring)
			                                     else (bind ?first-ring ?cx-first-ring))
			              (bind ?order-ring-colors (create$ ?first-ring (subseq$ (randomize$ (remove$ ?ring-colors ?first-ring)) 1 2))))
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

(defrule game-init-storage
	" Assert shelf-slot facts for each storage slot on each station.
	  Corresponding facts that already exist but do not have a machine specified
	  get properly initialized.
	"
	?init <- (init-storage-slots)
 =>
	(retract ?init)
	(delayed-do-for-all-facts ((?ssf machine-ss-shelf-slot)) (eq ?ssf:name UNSET)
		(do-for-all-facts ((?ss machine)) (eq ?ss:mtype SS)
			(duplicate ?ssf (name ?ss:name))
		)
		(retract ?ssf)
	)
	(do-for-all-facts ((?ss machine)) (eq ?ss:mtype SS)
		(loop-for-count (?shelf 0 ?*SS-MAX-SHELF*)
			(loop-for-count (?slot 0 ?*SS-MAX-SLOT*)
				(if (not (any-factp ((?ssf machine-ss-shelf-slot))
				                    (and (eq ?ss:name ?ssf:name)
				                         (eq ?ssf:position (create$ ?shelf ?slot)))))
				 then
					(assert (machine-ss-shelf-slot (name ?ss:name) (position (create$ ?shelf ?slot))))
				)
			)
		)
	)
)

(defrule game-init-parameterization-from-config
	?gt <- (game-parameters (is-initialized FALSE))
	(confval (path "/llsfrb/game/random-field") (type BOOL) (value ?random-field))
	(confval (path "/llsfrb/game/random-machine-setup") (type BOOL) (value ?random-machine-setup))
	(confval (path "/llsfrb/game/random-orders") (type BOOL) (value ?random-orders))
	(confval (path "/llsfrb/game/random-storage") (type BOOL) (value ?random-storage))
	(confval (path "/llsfrb/mongodb/enable") (type BOOL) (value ?cfg-mongodb-enabled))
	=>
	(bind ?m-positions PENDING)
	(bind ?m-setup PENDING)
	(bind ?orders PENDING)
	(bind ?s-status PENDING)
	(bind ?mongodb-enabled (eq ?cfg-mongodb-enabled true))
	(if (and (not ?mongodb-enabled) (member$ false (create$ ?random-field ?random-machine-setup ?random-orders ?random-storage )))
	 then
		(printout warn "Mongodb disabled, randomize all parameters despite configured static settings." crlf)
	)
	(if (or (not ?mongodb-enabled) (eq ?random-field  true)) then (bind ?m-positions RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?random-machine-setup true)) then (bind ?m-setup RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?random-orders true)) then (bind ?orders RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?random-storage true)) then (bind ?s-status RANDOM))
	(modify ?gt (machine-positions ?m-positions)
	            (machine-setup ?m-setup)
	            (orders ?orders)
	            (storage-status ?s-status)
	            (is-initialized TRUE))
)

(defrule game-mps-solver-start
  "start the solver"
  (gamestate (game-time ?gt))
  ?mg <- (machine-generation (state NOT-STARTED))
  (game-parameters (is-parameterized FALSE) (machine-positions RANDOM))
  =>
  (printout t "starting the solver for the generation of the machine positions" crlf)
  (mps-generator-set-field 5 7)
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
	?gp <- (game-parameters (machine-positions ?m-positions&~PENDING)
	                        (machine-setup ?m-setup&~PENDING)
	                        (orders ?orders&~PENDING)
	                        (storage-status ?s-status&~PENDING)
	                        (is-parameterized FALSE))
	(machine-generation (state ?s&:(or (eq ?s FINISHED) (eq ?m-positions STATIC))))
	=>
	(modify ?gp (is-parameterized TRUE))

	; reset machines
	(delayed-do-for-all-facts ((?machine machine)) TRUE
	  (if (eq ?machine:mtype RS) then (mps-reset-base-counter (str-cat ?machine:name)))
	  (modify ?machine (productions 0) (state IDLE) (cs-operation RETRIEVE_CAP)
	             (ss-operation STORE)
	             (proc-start 0.0) (desired-lights GREEN-ON YELLOW-ON RED-ON))
	)
	(if (eq ?m-positions RANDOM)
	 then
		(machine-randomize-positions)
	)
	(if (eq ?m-setup RANDOM)
	 then
		(machine-randomize-machine-setup)
	)
	(if (eq ?orders RANDOM)
	 then
		(game-randomize-orders)
	)
	(if (eq ?s-status RANDOM)
	 then
		(ss-shuffle-shelves)
	)

	(ss-print-storage C-SS)
	(ss-print-storage M-SS)
	(printout t "Top  5        1 3 5 7" crlf)
	(printout t " |   :        0 2 4 6" crlf)
	(printout t "Bot  0   Input ------> Output" crlf)
)

(defrule game-print
  (game-parameters (is-parameterized TRUE))
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
  ?gf <- (gamestate (phase SETUP|EXPLORATION|PRODUCTION)
		    (state RUNNING)
		    (game-time ?game-time) (cont-time ?cont-time)
		    (last-time $?last-time&:(neq ?last-time ?now)))
  ?st <- (sim-time (enabled ?sts) (estimate ?ste) (now $?sim-time)
		(speedup ?speedup) (real-time-factor ?rtf) (last-recv-time $?lrt))
  (or (sim-time (enabled false))
      (gamestate (last-time $?last-time&:(neq ?last-time ?sim-time))))
  =>
  (bind ?points-cyan (game-calc-points CYAN))
  (bind ?points-magenta (game-calc-points MAGENTA))
  (bind ?now (get-time ?sts ?ste ?now ?sim-time ?lrt ?rtf))
  (bind ?timediff (* (time-diff-sec ?now ?last-time) ?speedup))
  (modify ?gf (game-time (+ ?game-time ?timediff)) (cont-time (+ ?cont-time ?timediff))
	  (last-time ?now) (points ?points-cyan ?points-magenta))
)

(defrule game-update-last-time
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  (or (gamestate (phase ~PRODUCTION&~EXPLORATION&~SETUP))
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
  (modify ?gs (phase SETUP) (prev-phase PRE_GAME) (game-time 0.0) (start-time (now)))
  (assert (attention-message (text "Starting  *** TRAINING ***  game")))
)

(defrule game-start
  ?gs <- (gamestate (teams ?team_cyan ?team_magenta) (phase PRE_GAME) (state RUNNING))
  =>
  (modify ?gs (phase SETUP) (prev-phase PRE_GAME) (start-time (now)))
  (assert (attention-message (text "Starting setup phase") (time 15)))
)

(defrule game-set-start-time-if-unset
  ?gs <- (gamestate (teams ?team_cyan ?team_magenta) (phase ~PRE_GAME) (start-time 0 0))
  =>
  (modify ?gs (start-time (now)))
  (assert (attention-message (text "Start time unset, despite not in PRE_GAME. Setting it now!") (time 15)))
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
  (modify ?gs (phase PRODUCTION) (prev-phase SETUP) (game-time 0.0))
  (assert (attention-message (text "Switching to production phase")))
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

(deffunction machine> (?m1-fact ?m2-fact)
	(return (> (str-compare (fact-slot-value ?m1-fact name) (fact-slot-value ?m2-fact name)) 0))
)
(deffunction history> (?h1-fact ?h2-fact)
	(return (< (time-diff-sec (fact-slot-value ?h2-fact time) (fact-slot-value ?h1-fact time)) 0))
)

(deffunction id> (?o1-fact ?o2-fact)
	(return (> (fact-slot-value ?o1-fact id) (fact-slot-value ?o2-fact id)))
)

(deffunction points> (?p1-fact ?p2-fact)
	(return (> (fact-slot-value ?p1-fact game-time) (fact-slot-value ?p2-fact game-time)))
)

(deffunction game-summary ()
  (game-print-points)
  (assert (attention-message (text "Game Over") (time 60)))
  (printout t "===  Game Over  ===" crlf)
	(print-sep "Field Layout")
	; print relevant machine
	(bind ?machines (find-all-facts ((?m machine)) TRUE))
	(bind ?machines (sort machine> ?machines))
	(print-fact-list (fact-indices ?machines) (create$ name team zone rotation down-period))

	; print relevant order info
	(print-sep "Orders")
	(bind ?orders (find-all-facts ((?o order)) TRUE))
	(bind ?orders (sort id> ?orders))
	(print-fact-list (fact-indices ?orders)
	                 (create$ id complexity competitive base-color ring-colors
	                          cap-color quantity-requested quantity-delivered
	                          delivery-period activate-at))
	(foreach ?phase (deftemplate-slot-allowed-values points phase)
		(print-sep (str-cat ?phase " points"))
		(bind ?point (find-all-facts ((?h points)) (eq ?h:phase ?phase)))
		(bind ?point (sort points> ?point))
		(print-fact-list (fact-indices ?point) (create$))
	)

	(print-sep "Game configuration")
	(bind ?game-cfg (find-all-facts ((?confval confval))
	                (and (str-index "/llsfrb/game/" ?confval:path)
	                     (not (str-index "/llsfrb/game/crypto-keys" ?confval:path)))))
	(print-fact-list (fact-indices ?game-cfg) (create$ path value list-value))
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
  (do-for-all-facts ((?cfg confval) (?m machine))
      (and (eq ?m:mtype RS)
           (eq ?cfg:path (str-cat "/llsfrb/mps/stations/" ?m:name "/connection"))
           (eq ?cfg:value "mockup"))
    (printout warn "Please add points for remaining bases in slide of "
                   (str-cat ?m:name) " manually." crlf))
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
	; Retract all delivery periods
	(delayed-do-for-all-facts ((?dp delivery-period)) TRUE
		(retract ?dp)
	)
	; Reset all down periods
	(delayed-do-for-all-facts ((?m machine)) TRUE
		(modify ?m (down-period (deftemplate-slot-default-value machine down-period)))
	)
	; Reset game parameterization
	(delayed-do-for-all-facts ((?gp game-parameters)) TRUE
		(retract ?gp)
	)
	(assert (game-parameters))
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
