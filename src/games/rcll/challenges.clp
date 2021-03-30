
;---------------------------------------------------------------------------
;  challenges.clp - LLSF RefBox CLIPS technical challenges
;
;  Created: Thu Jun 13 13:12:44 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
	?*BASE_STATION* = 1
	?*CAP1_STATION* = 2
	?*CAP2_STATION* = 3
	?*RING1_STATION* = 4
	?*RING2_STATION* = 5
	?*STORAGE_STATION* = 6
	?*DELIVERY_STATION* = 7
	?*PRIORITY_CHALLENGE_OVERRIDE* = 100
)


(deffunction machine-to-id (?mps)
	(switch ?mps
		(case BS then (return ?*BASE_STATION*))
		(case CS1 then (return ?*CAP1_STATION*))
		(case CS2 then (return ?*CAP2_STATION*))
		(case RS1 then (return ?*RING1_STATION*))
		(case RS2 then (return ?*RING2_STATION*))
		(case SS then (return ?*STORAGE_STATION*))
		(case DS then (return ?*DELIVERY_STATION*))
	)
	(printout error "machine-to-id: unsupported machine: "
	  ?mps " Expected BS|CS1|CS2|RS1|RS2|SS|DS" crlf)
	(return 0)
)

(defrule challenges-configure-machine-ground-truth
" Send the ground truth for machine positions only for specified phases "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/send-mps-ground-truth") (is-list TRUE) (list-value $?gt))
	?s <- (send-mps-positions (phases ~$?gt))
=>
	(modify ?s (phases $?gt))
)

(defrule challenges-customize-orders
" Send the ground truth for machine positions only for specified phases "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/orders/customize") (type BOOL) (value true))
	(confval (path "/llsfrb/challenges/production-time") (type UINT) (value ?max-time))
=>
	(bind ?id 1)
	(do-for-all-facts ((?o order)) TRUE (retract ?o))
	(bind ?path-prefix "/llsfrb/challenges/orders/")
	(bind ?base-suffix "/base-color")
	(bind ?ring-suffix "/ring-colors")
	(bind ?cap-suffix "/cap-color")
	(do-for-all-facts ((?bc confval) (?rc confval) (?cc confval))
		(and (str-index ?path-prefix ?bc:path)
		     (str-index ?path-prefix ?rc:path)
		     (str-index ?path-prefix ?cc:path)
		     (str-index ?base-suffix ?bc:path)
		     (str-index ?ring-suffix ?rc:path)
		     (str-index ?cap-suffix ?cc:path)
		     (eq (sub-string 1 (- (str-index ?base-suffix ?bc:path) 1) ?bc:path)
		         (sub-string 1 (- (str-index ?ring-suffix ?rc:path) 1) ?rc:path)
		         (sub-string 1 (- (str-index ?cap-suffix ?cc:path) 1) ?cc:path)
		     )
		)
		(bind ?id (string-to-field (sub-string (+ (length$ ?path-prefix) 2)
		                               (- (str-index ?base-suffix ?bc:path) 1)
		                               ?bc:path)))
		(bind ?ring-colors (create$))
		(progn$ (?str-color ?rc:list-value) (bind ?ring-colors (append$ ?ring-colors (sym-cat ?str-color))))
		(printout error ?id crlf)
		(bind ?delivery-period (create$ 0 ?max-time))
		(assert (order (id ?id) (complexity (sym-cat C (length$ ?rc:list-value)))
		                        (base-color (sym-cat ?bc:value))
		                        (ring-colors ?ring-colors)
		                        (cap-color (sym-cat ?cc:value))
		                        (delivery-period ?delivery-period)))
	)
)

(defrule challenges-configure-machines
" adjust the machines placed on the field "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/machines") (is-list TRUE) (list-value $?machines))
	(confval (path "/llsfrb/challenges/field/width") (type UINT) (value ?x))
	(confval (path "/llsfrb/challenges/field/height") (type UINT) (value ?y))
	?mg <- (machine-generation (state NOT-STARTED))
	(game-parameters (is-parameterized FALSE) (machine-positions RANDOM))
	(not (machine-generation-triggered))
=>
	(foreach ?m (create$ ?*BASE_STATION*
	                     ?*CAP1_STATION*
	                     ?*CAP2_STATION*
	                     ?*RING1_STATION*
	                     ?*RING2_STATION*
	                     ?*STORAGE_STATION*
	                     ?*DELIVERY_STATION*)
		(mps-generator-remove-machine ?m)
	)
	(foreach ?m ?machines
		(mps-generator-add-machine (machine-to-id (sym-cat ?m)))
	)
	(if (config-get-bool "/llsfrb/challenges/machine-setup/customize")
	 then
		(delayed-do-for-all-facts ((?m machine) (?rc confval))
			(and (eq ?m:mtype RS)
			     (str-index "/llsfrb/challenges/machine-setup/rs" ?rc:path)
			     (str-index "-colors" ?rc:path)
			     (eq (sub-string 5 5 ?m:name)
			         (sub-string (+ (length$ "/llsfrb/challenges/machine-setup/rs") 1)
			                     (- (str-index "-colors" ?rc:path) 1)
			                     ?rc:path))
			)
			(bind ?ring-colors (create$))
			(progn$ (?str-color ?rc:list-value)
				(bind ?ring-colors (append$ ?ring-colors (sym-cat ?str-color))))
			(modify ?m (rs-ring-colors ?ring-colors))
		)
		(bind ?spec-prefix "/llsfrb/challenges/machine-setup/ring-specs/")
		(delayed-do-for-all-facts ((?spec confval))
			(str-index ?spec-prefix ?spec:path)
			(bind ?color (sym-cat (sub-string (+ (length$ ?spec-prefix) 1)
			                                  (length$ ?spec:path)
			                                  ?spec:path)))

			(do-for-fact ((?rs ring-spec)) (eq ?rs:color ?color) (retract ?rs))

			(assert (ring-spec (color ?color) (req-bases ?spec:value)))
		)
	)
	(mps-generator-set-field ?x ?y)
	(assert (machine-generation-triggered))
)

(defrule challenges-parameterize
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
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
	  (modify ?machine (productions 0) (state IDLE)
	             (proc-start 0.0) (desired-lights GREEN-ON YELLOW-ON RED-ON))
	)
	(if (eq ?m-positions RANDOM)
	 then
		(machine-retrieve-generated-mps false)
	)
	(if (not (config-get-bool "/llsfrb/challenges/machine-setup/customize"))
	 then
		(machine-randomize-machine-setup)
	)
	(if (not (config-get-bool "/llsfrb/challenges/orders/customize"))
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


; ***** WHACK-A-MOLE challenge *****
;(defrule techal-wam-next
;  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING) (prev-state ~RUNNING))
;  ?wf <- (whac-a-mole-light ?old-machine)
;  =>
;  (retract ?wf)
;  (modify ?gs (prev-state RUNNING))
;
;  (bind ?old-idx (member$ ?old-machine ?*TECHCHALL-WAM-MACHINES*))
;  (bind ?candidates ?*TECHCHALL-WAM-MACHINES*)
;  (if ?old-idx then (bind ?candidates (delete$ ?candidates ?old-idx ?old-idx)))
;
;  (bind ?new-idx (random 1 (length$ ?candidates)))
;  (bind ?new-machine (nth$ ?new-idx ?candidates))
;
;  (if (neq ?old-machine NONE)
;  then
;    (printout t "Machine " ?old-machine " reached, next is " ?new-machine crlf)
;  else
;    (printout t "Starting with machine " ?new-machine crlf)
;  )
;  (assert (whac-a-mole-light ?new-machine))
;)
;
;(defrule techal-wam-deactivate
;  (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING))
;  (whac-a-mole-light ?m)
;  ?mf <- (machine (name ~?m) (state ~DOWN))
;  =>
;  (modify ?mf (state DOWN) (desired-lights))
;)
;
;(defrule techal-wam-activate
;  (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING))
;  (whac-a-mole-light ?m)
;  ?mf <- (machine (name ?m) (state ~IDLE))
;  =>
;  (modify ?mf (state IDLE) (desired-lights RED-ON YELLOW-ON GREEN-ON))
;)
;
;(defrule techal-wam-continue
;  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state PAUSED) (prev-state RUNNING)
;		    (game-time ?gtime))
;  (whac-a-mole-light ?m)
;  =>
;  (assert (points (game-time ?gtime) (points 1) (phase WHACK_A_MOLE_CHALLENGE)
;		  (reason (str-cat "Machine " ?m " reached (referee)"))))
;
;  (modify ?gs (state RUNNING) (prev-state PAUSED))
;)
;
;(defrule techal-wam-reached
;  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING) (game-time ?gtime))
;  (whac-a-mole-light ?m)
;  ?mf <- (machine (name ?m) (pose $?m-pose))
;  (robot (vision-pose $?r-pose&:(in-box ?r-pose ?m-pose ?*TECHCHALL-WAM-BOX-SIZE*)))
;  =>
;  (assert (points (game-time ?gtime) (points 1) (phase WHACK_A_MOLE_CHALLENGE)
;		  (reason (str-cat "Machine " ?m " reached (vision)"))))
;  (modify ?gs (state RUNNING) (prev-state PAUSED))
;)
;
;(defrule techal-wam-game-over
;  ?gs <- (gamestate (phase WHACK_A_MOLE_CHALLENGE) (state RUNNING)
;		    (game-time ?game-time&:(>= ?game-time ?*TECHCHALL-WAM-TIME*)))
;  =>
;  (modify ?gs (phase POST_GAME) (prev-phase WHACK_A_MOLE_CHALLENGE) (state PAUSED)
;	  (end-time (now)))
;)
;
;
;; ***** NAVIGATION challenge *****
;(defrule techal-navigation-game-over
;  ?gs <- (gamestate (phase NAVIGATION_CHALLENGE) (state RUNNING)
;		    (game-time ?game-time&:(>= ?game-time ?*TECHCHALL-NAVIGATION-TIME*)))
;  =>
;  (modify ?gs (phase POST_GAME) (prev-phase NAVIGATION_CHALLENGE) (state PAUSED) (end-time (now)))
;)
