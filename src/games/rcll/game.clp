
;---------------------------------------------------------------------------
;  game.clp - RCLL RefBox CLIPS game maintenance
;
;  Created: Tue Jun 11 15:19:25 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;             2019       Mostafa Gomaa
;             2020       Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------
(deffunction is-standing-order (?order-id)
  (return (<= ?order-id 2))
)

(deffunction randomize-ring-colors (?complexity ?ring-colors)
  (bind ?order-ring-colors (create$))
  (bind ?choices (create$ 1 2 3 4))
  (bind ?ring1-choice (pick-random$ ?choices))
  (bind ?ring2-choice (pick-random$ (delete$ ?choices ?ring1-choice ?ring1-choice)))
  (bind ?ring3-choice (pick-random$ (delete$ ?choices ?ring2-choice ?ring2-choice)))
  (bind ?ring1-color (nth$ ?ring1-choice ?ring-colors))
  (bind ?ring2-color (nth$ ?ring2-choice ?ring-colors))
  (bind ?ring3-color (nth$ ?ring3-choice ?ring-colors))
  (switch ?complexity
    ;(case C0 then) ; for C0 we have nothing to do, no ring color
    (case C1 then (bind ?order-ring-colors (create$ ?ring1-color)))
    (case C2 then (bind ?order-ring-colors (create$ ?ring1-color ?ring2-color)))
    (case C3 then (bind ?order-ring-colors (create$ ?ring1-color ?ring2-color ?ring3-color)))
  )
  (return ?order-ring-colors)
)

(deffunction game-randomize-orders ()
  (bind ?ring-colors (create$))
  (do-for-all-facts ((?rs ring-spec)) TRUE
    (bind ?ring-colors (append$ ?ring-colors ?rs:color))
  )
  (bind ?ring-colors (randomize$ ?ring-colors))

  (bind ?low-complexity (create$ C0 C1))
  (bind ?high-complexity (create$ C2 C3))
  (bind ?complexities (create$))
  (loop-for-count (?current-order 1 5)
       (bind ?low-complexity (randomize$ ?low-complexity))
       (bind ?high-complexity (randomize$ ?high-complexity))
       (bind ?complexities (append$ ?complexities
             (create$ (nth$ 1 ?low-complexity)
                      (nth$ 1 ?high-complexity))))
  )
  ; Standing Orders
  (delayed-do-for-all-facts ((?order order)) (is-standing-order ?order:id)
    (bind ?complexity (nth$ 1 ?complexities))
    (bind ?complexities (rest$ ?complexities))
    (modify ?order (complexity ?complexity) (start-range 0 0)
                   (activation-range ?*PRODUCTION-TIME* ?*PRODUCTION-TIME*)
                   (duration-range ?*PRODUCTION-TIME* ?*PRODUCTION-TIME*))
  )
  (bind ?activate-at-center 180)
  (bind ?complexities (randomize$ ?complexities))
  ; Set start activation and duration ranges
  (delayed-do-for-all-facts ((?order order))
    (and (not (is-standing-order ?order:id)) (not ?order:allow-overtime))
    (bind ?complexity (nth$ 1 ?complexities))
    (bind ?complexities (rest$ ?complexities))
    (switch ?complexity
      (case C0 then
        (modify ?order (complexity ?complexity)
                       (start-range (- ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*)
                                         (+ ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*))
                       (activation-range ?*ORDER-PRODUCTION-WINDOW-START-C0*
                                         ?*ORDER-PRODUCTION-WINDOW-END-C0*)
                       (duration-range ?*ORDER-DELIVERY-WINDOW-START-C0* ?*ORDER-DELIVERY-WINDOW-END-C0*)
        )
      )
      (case C1 then
       (modify ?order (complexity ?complexity)
                      (start-range (- ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*)
                                         (+ ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*))
                      (activation-range ?*ORDER-PRODUCTION-WINDOW-START-C1*
                                         ?*ORDER-PRODUCTION-WINDOW-END-C1*)
                      (duration-range ?*ORDER-DELIVERY-WINDOW-START-C1* ?*ORDER-DELIVERY-WINDOW-END-C1*)
        )
      )
      (case C2 then
        (modify ?order (complexity ?complexity)
                       (start-range (- ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*)
                                         (+ ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*))
                       (activation-range ?*ORDER-PRODUCTION-WINDOW-START-C2*
                                         ?*ORDER-PRODUCTION-WINDOW-END-C2*)
                       (duration-range ?*ORDER-DELIVERY-WINDOW-START-C2* ?*ORDER-DELIVERY-WINDOW-END-C2*)
        )
      )
      (case C3 then
        (modify ?order (complexity ?complexity)
                       (start-range (- ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*)
                                         (+ ?activate-at-center ?*ORDER-ACTIVATION-DEVIATION*))
                       (activation-range ?*ORDER-PRODUCTION-WINDOW-START-C3*
                                         ?*ORDER-PRODUCTION-WINDOW-END-C3*)
                       (duration-range ?*ORDER-DELIVERY-WINDOW-START-C3* ?*ORDER-DELIVERY-WINDOW-END-C3*)
        )
      )
      (default then (printout t "Unknown complexity " ?complexity crlf))
    )
    (bind ?activate-at-center (+ ?activate-at-center ?*ORDER-ACTIVATION-DISTANCE*))
  )
  ; reset orders, assign random times
  (delayed-do-for-all-facts ((?order order)) TRUE
    (bind ?deliver-start (random (nth$ 1 ?order:start-range)
                                 (nth$ 2 ?order:start-range)))
    (bind ?deliver-end
      (+ ?deliver-start (random (nth$ 1 ?order:duration-range)
                                (nth$ 2 ?order:duration-range))))
    (bind ?activation-pre-time
          (random (nth$ 1 ?order:activation-range) (nth$ 2 ?order:activation-range)))
    (bind ?activate-at (max (- ?deliver-start ?activation-pre-time) 0))
    (if ?*RANDOMIZE-ACTIVATE-ALL-AT-START* then (bind ?activate-at 0))
    ; keep the activation time between ?*EXPLORATION-TIME* and
    ; ?*ORDER-ACTIVATE-LATEST-TIME*
    (if (and (not (is-standing-order ?order:id)) (< ?activate-at ?*EXPLORATION-TIME*)) then
      (bind ?shift-time (- ?*EXPLORATION-TIME* ?activate-at))
      (bind ?activate-at   (+ ?activate-at ?shift-time))
      (bind ?deliver-start (+ ?deliver-start ?shift-time))
      (bind ?deliver-end   (+ ?deliver-end ?shift-time))
    )
    (if (and (> ?activate-at ?*ORDER-ACTIVATE-LATEST-TIME*)
             (not ?order:allow-overtime)) then
      (bind ?shift-time (- ?activate-at ?*ORDER-ACTIVATE-LATEST-TIME*))
      (bind ?activate-at   (- ?activate-at ?shift-time))
      (bind ?deliver-start (- ?deliver-start ?shift-time))
      (bind ?deliver-end   (- ?deliver-end ?shift-time))
    )
   ; keep the delivery time within the ?*PRODUCTION-TIME*
    (if (and (> ?deliver-end ?*PRODUCTION-TIME*) (not ?order:allow-overtime))
     then
      (printout t "Revising deliver time (" ?deliver-start "-" ?deliver-end ") to ("
                  (- ?deliver-start (- ?deliver-end ?*PRODUCTION-TIME*)) "-" ?*PRODUCTION-TIME* "), "
                  "time shift: " (- ?deliver-end ?*PRODUCTION-TIME*) crlf)
      (bind ?deliver-start (- ?deliver-start (- ?deliver-end ?*PRODUCTION-TIME*)))
      (bind ?deliver-end ?*PRODUCTION-TIME*)
    )

    (bind ?gate (random 1 3))

    (bind ?order-base-color (pick-random$ (deftemplate-slot-allowed-values order base-color)))
    (bind ?order-ring-colors (randomize-ring-colors ?order:complexity ?ring-colors))
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
  (bind ?free-ring-colors (create$))
  (delayed-do-for-all-facts ((?ring ring-spec))
    (or (eq ?ring:color (nth$ 2 ?ring-colors)) (eq ?ring:color (nth$ 4 ?ring-colors)))
    (bind ?free-ring-colors (append$ ?free-ring-colors ?ring:color))
    (modify ?ring (req-bases 0))
  )

  ; Randomly assign an order to be a competitive order
  (bind ?potential-competitive-orders (create$))
  (do-for-all-facts ((?order order))
                    (and (eq ?order:quantity-requested 1) ; must not request more than 1 product
                         (eq ?order:allow-overtime FALSE) ; no overtime order
                         (or (neq (nth$ 1 ?order:delivery-period) 0) ; no standing order
                             (neq (nth$ 2 ?order:delivery-period) ?*PRODUCTION-TIME*))
                    )
                    (bind ?potential-competitive-orders (insert$ ?potential-competitive-orders 1 ?order:id))
  )
  (bind ?competitive-order-id (nth$ (random 1 (length$ ?potential-competitive-orders)) ?potential-competitive-orders))
  (do-for-fact ((?order order)) (eq ?order:id ?competitive-order-id)
    (modify ?order (competitive TRUE)))
  (bind ?conflict TRUE)
  ; Check if the assigned colors are correct according to the rules
  (while ?conflict
    (bind ?conflict FALSE)
    (do-for-fact ((?order1 order) (?order2 order))
                     ; ... standing orders should have a first ring without costs
                 (or (and (is-standing-order ?order1:id)
                          (> (length$ ?order1:ring-colors) 0)
                          (not (member$ (nth$ 1 ?order1:ring-colors) ?free-ring-colors)))
                     ; ... orders should have unique base+ring combinations
                     ; such that one can not start one order and finish it as
                     ; another one
                     (and (eq ?order1:base-color ?order2:base-color)
                          (> (length$ ?order1:ring-colors) 0)
                          (> (length$ ?order2:ring-colors) 0)
                          (> ?order1:id ?order2:id)
                          (eq (nth$ 1 ?order1:ring-colors) (nth$ 1 ?order2:ring-colors)))
                 )
      (modify ?order1 (ring-colors (randomize-ring-colors ?order1:complexity ?ring-colors))
                      (base-color (pick-random$ (deftemplate-slot-allowed-values order base-color))))
      (bind ?conflict TRUE)
    )
  )
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
	(confval (path "/llsfrb/game/default-storage") (type BOOL) (value ?default-storage))
	(confval (path "/llsfrb/mongodb/enable") (type BOOL) (value ?cfg-mongodb-enabled))
	=>
	(bind ?m-positions PENDING)
	(bind ?m-setup PENDING)
	(bind ?orders PENDING)
	(bind ?s-status PENDING)
	(bind ?mongodb-enabled (eq ?cfg-mongodb-enabled TRUE))
	(if (and (not ?mongodb-enabled) (member$ FALSE (create$ ?random-field ?random-machine-setup ?random-orders ?default-storage )))
	 then
		(printout warn "Mongodb disabled, randomize all parameters despite configured static settings." crlf)
	)
	(if (or (not ?mongodb-enabled) (eq ?random-field  TRUE)) then (bind ?m-positions RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?random-machine-setup TRUE)) then (bind ?m-setup RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?random-orders TRUE)) then (bind ?orders RANDOM))
	(if (or (not ?mongodb-enabled) (eq ?default-storage TRUE)) then (bind ?s-status DEFAULT))
	(modify ?gt (machine-positions ?m-positions)
	            (machine-setup ?m-setup)
	            (orders ?orders)
	            (storage-status ?s-status)
	            (is-initialized TRUE))
)

(defrule game-mps-solver-start
  "start the solver"
  (time-info (game-time ?gt))
  ?mg <- (machine-generation (state NOT-STARTED))
  (game-parameters (is-parameterized FALSE) (machine-positions RANDOM))
  =>
  (printout t "starting the solver for the generation of the machine positions" crlf)
  (mps-generator-set-field ?*FIELD-WIDTH* ?*FIELD-HEIGHT*)
  (mps-generator-start)
  (modify ?mg (state STARTED) (generation-state-last-checked ?gt))
)

(defrule game-mps-solver-check
  "check if the solver is finished"
  (gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
  (time-info (game-time ?gt))
  ?mg <- (machine-generation (state STARTED) (generation-state-last-checked ?gs&:(timeout-sec ?gt ?gs ?*MACHINE-GENERATION-TIMEOUT-CHECK-STATE*)))
  =>
  (if (and (not (mps-generator-running)) (mps-generator-field-generated))
   then
    (printout t "the machine generation is finished" crlf)
    (modify ?mg (state FINISHED))
   else
    (printout warn "time-out for the generation of the machine positions, resetting game" crlf)
    (assert (game-reset))
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

	(delayed-do-for-all-facts ((?bs-meta bs-meta)) TRUE (retract ?bs-meta))
	; do not delete rs-meta as the ring-colors are set later in this rule
	(delayed-do-for-all-facts ((?rs-meta rs-meta)) TRUE
	  (modify ?rs-meta (slide-counter 0)
	                   (bases-added 0)
	                   (bases-used 0)
	  )
	)
	(delayed-do-for-all-facts ((?cs-meta cs-meta)) TRUE (retract ?cs-meta))
	(delayed-do-for-all-facts ((?ds-meta ds-meta)) TRUE (retract ?ds-meta))
	(delayed-do-for-all-facts ((?ss-meta ss-meta)) TRUE (retract ?ss-meta))
	; reset machines
	(delayed-do-for-all-facts ((?machine machine)) TRUE
	  (if (eq ?machine:mtype RS) then (mps-reset-base-counter (str-cat ?machine:name)))
	  (modify ?machine (state IDLE))
	)

	(delayed-do-for-all-facts ((?ml machine-lights)) TRUE
	  (modify ?ml (desired-lights GREEN-ON YELLOW-ON RED-ON))
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
	; Shelves are not shuffled in default games
	(if (eq ?s-status DEFAULT)
	 then
		;(ss-shuffle-shelves)
		(printout t "Using default setup for pre-stored products at SS." crlf)
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
  (do-for-all-facts ((?m machine) (?meta rs-meta))
    (and (eq ?m:mtype RS) (eq ?m:name ?meta:name))
    (printout t "RS " ?meta:name " has colors " ?meta:available-colors crlf)
  )

	; Print machine swapping info
	(do-for-all-facts ((?zs zone-swap)) TRUE
	  (printout warn "Swap " ?zs:m1-name " to zone " ?zs:m1-new-zone
							" and " ?zs:m2-name " to zone " ?zs:m2-new-zone crlf)
  )
)

(defrule game-update-gametime-points
  (declare (salience ?*PRIORITY-FIRST*))
  (time $?now)
  ?gf <- (gamestate (phase SETUP|EXPLORATION|PRODUCTION) (points $?old-points)
		    (state RUNNING))
  ?ti <- (time-info (game-time ?game-time) (cont-time ?cont-time)
		    (last-time $?last-time&:(neq ?last-time ?now)))
  ?st <- (sim-time (enabled ?sts) (estimate ?ste) (now $?sim-time)
		(speedup ?speedup) (real-time-factor ?rtf) (last-recv-time $?lrt))
  (or (sim-time (enabled FALSE))
      (time-info (last-time $?last-time&:(neq ?last-time ?sim-time))))
  =>
  (bind ?points-cyan (game-calc-points CYAN))
  (bind ?points-magenta (game-calc-points MAGENTA))
  (bind ?now (get-time ?sts ?ste ?now ?sim-time ?lrt ?rtf))
  (bind ?timediff (* (time-diff-sec ?now ?last-time) ?speedup))
  (modify ?ti (game-time (+ ?game-time ?timediff)) (cont-time (+ ?cont-time ?timediff))
    (last-time ?now))
  (if (neq ?old-points (create$ ?points-cyan ?points-magenta)) then
    (modify ?gf (points ?points-cyan ?points-magenta))
  )
)

(defrule game-update-last-time
  (declare (salience ?*PRIORITY-FIRST*))
  (time $?now)
  (or (gamestate (phase ~PRODUCTION&~EXPLORATION&~SETUP))
      (gamestate (state ~RUNNING)))
  ?st <- (sim-time (enabled ?sts) (estimate ?ste) (now $?sim-time)
		   (real-time-factor ?rtf) (last-recv-time $?lrt))
  ?ti <- (time-info (last-time $?last-time&:(neq ?last-time ?now)))
  (or (sim-time (enabled FALSE))
      (time-info (last-time $?last-time&:(neq ?last-time ?sim-time))))
  (not (finalize))
  =>
  (bind ?now (get-time ?sts ?ste ?now ?sim-time ?lrt ?rtf))
  (modify ?ti (last-time ?now))
)

(defrule game-start-training
  ?gs <- (gamestate (teams "" "") (phase PRE_GAME) (state RUNNING))
  ?ti <- (time-info)
  =>
  (modify ?gs (phase SETUP) (prev-phase PRE_GAME) (start-time (now))
    (field-height ?*FIELD-HEIGHT*) (field-width ?*FIELD-WIDTH*) (field-mirrored ?*FIELD-MIRRORED*))
  (modify ?ti (game-time 0.0))
  (assert (attention-message (text "Starting  *** TRAINING ***  game")))
)

(defrule game-start
  ?gs <- (gamestate (teams ?team_cyan ?team_magenta) (phase PRE_GAME) (state RUNNING))
  ?ti <- (time-info)
  =>
  (modify ?gs (phase SETUP) (prev-phase PRE_GAME) (start-time (now))
    (field-height ?*FIELD-HEIGHT*) (field-width ?*FIELD-WIDTH*) (field-mirrored ?*FIELD-MIRRORED*))
  (modify ?ti (game-time 0.0))
  (assert (attention-message (text "Starting setup phase") (time 15)))
)

(defrule game-set-start-time-if-unset
  ?gs <- (gamestate (teams ?team_cyan ?team_magenta) (phase ~PRE_GAME) (start-time 0 0))
  =>
  (modify ?gs (start-time (now)))
  (assert (attention-message (text "Start time unset, despite not in PRE_GAME. Setting it now!") (time 15)))
)

(defrule game-setup-warn-end-near
  (gamestate (phase SETUP) (state RUNNING))
  (time-info (game-time ?game-time&:(>= ?game-time (* ?*SETUP-TIME* .9))))
  (not (setup-warned))
  =>
  (assert (setup-warned))
  (assert (attention-message (text "Setup phase is about to end")))
)

(defrule game-switch-from-setup-to-production
  ?gs <- (gamestate (phase SETUP) (state RUNNING))
  ?ti <- (time-info (game-time ?game-time&:(>= ?game-time ?*SETUP-TIME*)))
  =>
  (modify ?gs (phase PRODUCTION) (prev-phase SETUP))
  (modify ?ti (game-time 0.0))
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

(deffunction cont-time> (?o1-fact ?o2-fact)
	(return (> (fact-slot-value ?o1-fact cont-time) (fact-slot-value ?o2-fact cont-time)))
)

(deffunction points> (?p1-fact ?p2-fact)
	(return (> (fact-slot-value ?p1-fact game-time) (fact-slot-value ?p2-fact game-time)))
)

(deffunction start-time> (?p1-fact ?p2-fact)
	(return (> (fact-slot-value ?p1-fact start-time) (fact-slot-value ?p2-fact start-time)))
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
		    (over-time FALSE) (points ?p-cyan ?p-magenta&:(<> ?p-cyan ?p-magenta)))
  (time-info (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION) (state PAUSED))
)

(defrule game-enter-overtime
  ?gs <- (gamestate (refbox-mode STANDALONE) (phase PRODUCTION) (state RUNNING)
		    (over-time FALSE) (points ?p-cyan ?p-magenta&:(= ?p-cyan ?p-magenta)))
  (time-info (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  =>
  (assert (attention-message (text "Entering over-time") (time 15)))
  (modify ?gs (over-time TRUE))
)

(defrule game-over-after-overtime
  ?gs <- (gamestate (refbox-mode STANDALONE) (phase PRODUCTION) (state RUNNING)
		    (over-time TRUE))
  (time-info (game-time ?gt&:(>= ?gt (+ ?*PRODUCTION-TIME* ?*PRODUCTION-OVERTIME*))))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION) (state PAUSED) (end-time (now)))
)

(defrule game-goto-post-game
  (declare (salience ?*PRIORITY-FIRST*))
  ?gs <- (gamestate (phase POST_GAME) (prev-phase ~POST_GAME))
  =>
  (modify ?gs (prev-phase POST_GAME) (end-time (now)))
  (delayed-do-for-all-facts ((?ml machine-lights)) TRUE
    (modify ?ml (desired-lights RED-BLINK))
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

(defrule game-quit-after-finalize
  (declare (salience ?*PRIORITY-HIGH*))
  (gamestate (phase POST_GAME))
  (finalize)
  =>
  (exit)
)

(defrule game-over-on-finalize
	"Switch to post-game if the refbox is stopped"
	(declare (salience ?*PRIORITY-HIGH*))
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

(deffunction game-restart ()
	; Retract all delivery periods
	(delayed-do-for-all-facts ((?dp delivery-period)) TRUE
		(retract ?dp)
	)
	; reset machine progression
	(delayed-do-for-all-facts ((?bs-meta bs-meta)) TRUE (retract ?bs-meta))
	; do not delete rs-meta as the ring-colors are set later in this rule
	(delayed-do-for-all-facts ((?rs-meta rs-meta)) TRUE
	  (modify ?rs-meta (slide-counter 0)
	                   (bases-added 0)
	                   (bases-used 0)
	  )
	)
	(delayed-do-for-all-facts ((?cs-meta cs-meta)) TRUE (retract ?cs-meta))
	(delayed-do-for-all-facts ((?ds-meta ds-meta)) TRUE (retract ?ds-meta))
	(delayed-do-for-all-facts ((?ss-meta ss-meta)) TRUE (retract ?ss-meta))

	; delete workpieces
	(delayed-do-for-all-facts ((?wp workpiece)) TRUE (retract ?wp))

	; retract points
	(delayed-do-for-all-facts ((?points points)) TRUE (retract ?points))

	; reset orders
	(delayed-do-for-all-facts ((?o order)) TRUE
	  (modify ?o (active FALSE) (quantity-delivered (create$ 0 0)))
	)

	; create a fresh game report if mongodb is active
	(assert (mongodb-new-report))
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

	(game-restart)
)

(defrule game-switch-back-to-setup
  ?gs <- (gamestate (phase SETUP) (prev-phase ~PRE_GAME))
  ?ti <- (time-info)
  =>
  (modify ?gs (prev-phase PRE_GAME) (state WAIT_START))
  (modify ?ti (game-time 0.0))
  (delayed-do-for-all-facts ((?ml machine-lights)) TRUE
    (modify ?ml (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )
  (game-restart)
)

(defrule game-switch-back-to-pre-game
  ?gs <- (gamestate (phase PRE_GAME) (prev-phase ~PRE_GAME))
  ?ti <- (time-info)
  =>
  (modify ?gs (prev-phase PRE_GAME) (state WAIT_START))
  (modify ?ti (game-time 0.0))
  (delayed-do-for-all-facts ((?ml machine-lights)) TRUE
    (modify ?ml (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )
  (assert (reset-game))
)

(defrule game-reset
	"Allow calling the function via fact to avoid file loading order issues"
	?gr <- (game-reset)
	=>
	(retract ?gr)
	(game-reset)
)

(defrule game-create-first-machine-history
  (declare (salience ?*PRIORITY-FIRST*))
	?m <- (machine (name ?n) (state ?s))
	(or ?mf <- (bs-meta (name ?n))
	    ?mf <- (rs-meta (name ?n))
	    ?mf <- (cs-meta (name ?n))
	    ?mf <- (ds-meta (name ?n))
	    ?mf <- (ss-meta (name ?n))
	)
	(time-info (game-time ?gt))
	(time $?now)
	(not (machine-history (name ?n)))
	=>
	(assert (machine-history (name ?n) (game-time ?gt) (time ?now)
	          (state ?s) (fact-string (fact-to-string ?m))
	          (meta-fact-string (fact-to-string ?mf))
	))
)

(defrule game-create-next-machine-history
  (declare (salience ?*PRIORITY-FIRST*))
	?m <- (machine (name ?n) (state ?s))
	?hist <- (machine-history (name ?n) (state ?s-last&:(neq ?s ?s-last))
	           (time $?last) (is-latest TRUE))
	(or ?mf <- (bs-meta (name ?n))
	    ?mf <- (rs-meta (name ?n))
	    ?mf <- (cs-meta (name ?n))
	    ?mf <- (ds-meta (name ?n))
	    ?mf <- (ss-meta (name ?n))
	)
	(time-info (game-time ?gt))
	(time $?now)
	=>
	(modify ?hist (is-latest FALSE))
	(assert (machine-history (name ?n) (game-time ?gt)
	          (time ?now) (state ?s) (fact-string (fact-to-string ?m))
	          (meta-fact-string (fact-to-string ?mf))
	))
)

(defrule game-create-first-gamestate-history
	(not (gamestate-history))
	?gs <- (gamestate (state ?state) (phase ?phase) (over-time ?ot))
	(time-info (game-time ?gt) (cont-time ?ct))
	(time $?now)
	=>
	(assert (gamestate-history (state ?state) (phase ?phase)
	  (game-time ?gt) (time $?now)
	  (cont-time ?ct) (over-time ?ot) (fact-string (fact-to-string ?gs))))
)

(defrule game-create-next-gamestate-history
  (declare (salience ?*PRIORITY-FIRST*))
	?gsh <- (gamestate-history (is-latest TRUE) (state ?state)
	  (phase ?phase) (over-time ?ot) )
	?gs <- (gamestate (state ?curr-state) (phase ?curr-phase) (over-time ?curr-ot))
	?ti <- (time-info (game-time ?gt) (cont-time ?ct))
	(test (or (neq ?curr-state ?state) (neq ?curr-phase ?phase)
	  (neq ?curr-ot ?ot)))
	(time $?now)
	=>
	(modify ?gsh (is-latest FALSE))
	(assert (gamestate-history (state ?curr-state) (phase ?curr-phase)
	  (game-time ?gt) (cont-time ?ct) (time $?now) (over-time ?curr-ot)
	  (fact-string (fact-to-string ?gs)) (time-fact-string (fact-to-string ?ti))))
)

(defrule game-create-first-robot-history
	?robot <- (robot (number ?n) (state ?s) (warning-sent ?ws) (has-pose ?hp) (team ?t) (team-color ?team-color) (pose ?x ?y ?ori) (pose-time $?pt) (maintenance-start-time ?mst) (maintenance-cycles ?mc) (maintenance-warning-sent ?mws))
	(not (robot-history (number ?n) (team ?team)))
	(time $?now)
	(time-info (game-time ?gt))
	=>
	(assert (robot-history
	  (state ?s)
      (warning-sent ?ws)
      (has-pose ?hp)
      (number ?n)
	  (team ?t)
	  (team-color ?team-color)
      (pose ?x ?y ?ori)
      (maintenance-start-time ?mst)
      (maintenance-cycles ?mc)
      (maintenance-warning-sent ?mws)
      (pose-time ?pt)
      (time ?now)
      (game-time ?gt)
      (fact-string (fact-to-string ?robot))
	  )
	)
)

(defrule game-create-next-robot-history
  (declare (salience ?*PRIORITY-FIRST*))
	?rh <- (robot-history (state ?s) (warning-sent ?ws) (has-pose ?hp)
      (number ?n) (team ?t) (team-color ?tc) (pose $?pose)
      (maintenance-start-time ?mst) (maintenance-cycles ?mc)
      (maintenance-warning-sent ?mws) (pose-time $?pt) (is-latest TRUE)
	)
	?robot <- (robot (number ?n) (state ?n-s) (warning-sent ?n-ws) (has-pose ?n-hp) (team ?t) (team-color ?tc) (pose $?n-pose) (pose-time $?n-pt) (maintenance-start-time ?n-mst) (maintenance-cycles ?n-mc) (maintenance-warning-sent ?n-mws))
	(time $?now)
	(time-info (game-time ?gt))
	(test (or (neq
	(create$ ?n-s ?n-ws ?n-hp ?n-mst ?n-mc ?n-ws)
	(create$ ?s ?ws ?hp ?mst ?mc ?ws))
	  (and (neq ?n-pose ?pose)
	       (> (time-diff-sec ?n-pt ?pt) 5))))
	=>
	(bind ?rh (modify ?rh (is-latest FALSE)))
	(duplicate ?rh (state ?n-s) (warning-sent ?n-ws) (has-pose ?n-hp) (pose ?n-pose) (pose-time ?n-pt) (maintenance-start-time ?n-mst) (maintenance-cycles ?n-mc) (maintenance-warning-sent ?n-mws) (time ?now) (game-time ?gt) (is-latest TRUE))
)

(defrule game-create-first-shelf-slot-history
    ?sf <- (machine-ss-shelf-slot (position ?shelf ?slot) (name ?name) (is-filled ?filled) (description  ?desc))
	(not (shelf-slot-history (name ?name) (shelf ?shelf) (slot ?slot)))
	(time $?now)
	(time-info (game-time ?gt))
	=>
	(assert (shelf-slot-history (shelf ?shelf) (slot ?slot) (name ?name) (is-filled ?filled) (description ?desc)
	  (time $?now) (game-time ?gt) (fact-string (fact-to-string ?sf))))
)

(defrule game-create-next-shelf-slot-history
  (declare (salience ?*PRIORITY-FIRST*))
    ?hf <- (shelf-slot-history (shelf ?shelf) (slot ?slot) (name ?name) (is-filled ?filled) (description  ?desc) (is-latest TRUE))
    ?sf <- (machine-ss-shelf-slot (position ?shelf ?slot) (name ?name) (is-filled ?new-filled) (description  ?new-desc))
	(test (or (neq ?filled ?new-filled) (neq ?desc ?new-desc)))
	(time $?now)
	(time-info (game-time ?gt))
	=>
	(bind ?hf (modify ?hf (is-latest FALSE)))
    (duplicate ?hf (is-filled ?new-filled) (description ?new-desc) (fact-string (fact-to-string ?sf)) (time ?now) (game-time ?gt))
)

(defrule silence-debug
	(confval (path "/llsfrb/clips/debug") (type BOOL) (value TRUE))
	(confval (path "/llsfrb/clips/debug-level") (type UINT) (value ?v&:(< ?v 3)))
	=>
	(unwatch facts machine-history gamestate-history robot-history shelf-slot-history)
	(unwatch rules game-create-next-machine-history game-create-next-gamestate-history game-create-next-robot-history game-create-next-shelf-slot-history)
)

(defrule game-print-machine-history
	(declare (salience ?*PRIORITY-HIGHER*))
	(finalize)
	=>
	; machine history
	(foreach ?curr-mps (deftemplate-slot-allowed-values machine name)
		(print-sep (str-cat ?curr-mps " states"))
		(bind ?history (find-all-facts ((?h machine-history)) (eq ?h:name ?curr-mps)))
		(bind ?history (sort history> ?history))
		(print-fact-list (fact-indices ?history) (create$ game-time time name state))
	)
)

(defrule game-print-gamestates-from-mongodb-report
	(declare (salience ?*PRIORITY-HIGHER*))
	(finalize)
	=>
	(bind ?history (find-all-facts ((?h gamestate-history)) TRUE))
	(bind ?history (sort cont-time> ?history))
	(print-fact-list (fact-indices ?history)
	                 (create$ phase prev-phase game-time cont-time))
)
