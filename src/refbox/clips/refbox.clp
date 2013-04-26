
;---------------------------------------------------------------------------
;  refbox.clp - LLSF RefBox CLIPS main file
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; LLSF RefBox Version
; This information is send to newly connected clients over the network and
; broadcasted whenever a new client is detected.
(defglobal
  ?*VERSION-MAJOR* = 0
  ?*VERSION-MINOR* = 5
  ?*VERSION-MICRO* = 0
)

(load* (resolve-file net.clp))
(load* (resolve-file machines.clp))
(load* (resolve-file orders.clp))
(load* (resolve-file production.clp))
(load* (resolve-file exploration.clp))

(defrule update-gametime-points
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  ?gf <- (gamestate (phase PRODUCTION|EXPLORATION) (state RUNNING) (points ?old-points)
		    (game-time ?game-time) (last-time $?last-time&:(neq ?last-time ?now)))
  =>
  (bind ?points 0)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (bind ?phase-points 0)
    (do-for-all-facts ((?p points)) (eq ?p:phase ?phase)
      (bind ?phase-points (+ ?phase-points ?p:points))
    )
    (bind ?points (+ ?points (max ?phase-points 0)))
  )
  (modify ?gf (game-time (+ ?game-time (time-diff-sec ?now ?last-time))) (last-time ?now)
	  (points ?points))
)

(defrule update-last-time
  (declare (salience ?*PRIORITY_FIRST*))
  (time $?now)
  (or (gamestate (phase ~PRODUCTION&~EXPLORATION))
      (gamestate (state ~RUNNING)))
  ?gf <- (gamestate (last-time $?last-time&:(neq ?last-time ?now)))
  =>
  (modify ?gf (last-time ?now))
)

(defrule rfid-input-cleanup
  (declare (salience ?*PRIORITY_CLEANUP*))
  ?f <- (rfid-input (machine ?m) (has-puck ?hp) (id ?id))
  =>
  (retract ?f)
  ;(if (debug 1) then
  ;  (printout t "Clearing unused RFID input (" ?m ", " ?hp ", " ?id ")" crlf))
)


(defrule silence-debug-facts
  (declare (salience -1000))
  (init)
  (confval (path "/llsfrb/clips/debug") (type BOOL) (value true))
  (confval (path "/llsfrb/clips/unwatch-facts") (type STRING) (is-list TRUE) (list-value $?lv))
  =>
  (printout t "Disabling watching of the following facts: " ?lv crlf)
  (foreach ?v ?lv (unwatch facts (sym-cat ?v)))
)

(defrule silence-debug-rules
  (declare (salience -1000))
  (init)
  (confval (path "/llsfrb/clips/debug") (type BOOL) (value true))
  (confval (path "/llsfrb/clips/unwatch-rules") (type STRING) (is-list TRUE) (list-value $?lv))
  =>
  (printout t "Disabling watching of the following rules: " ?lv crlf)
  (foreach ?v ?lv (unwatch rules (sym-cat ?v)))
)

(defrule reset-game
  ?gs <- (gamestate (state INIT) (prev-state ~INIT))
  =>
  (bind ?nc-id (create$))
  (bind ?nc-host (create$))
  (bind ?nc-port (create$))
  ; Remember network clients
  (do-for-all-facts ((?client network-client)) TRUE
    (bind ?nc-id   (create$ ?nc-id ?client:id))
    (bind ?nc-host (create$ ?nc-id ?client:host))
    (bind ?nc-port (create$ ?nc-id ?client:port))
  )
  ; reset the CLIPS environment
  (reset)
  (assert (init))
  ; restore network clients
  (foreach ?cid ?nc-id
    (assert (network-client (id ?cid)
			    (host (nth$ ?cid-index ?nc-host))
			    (port (nth$ ?cid-index ?nc-host))))
  )
)


(defrule goto-pre-game
  ?gs <- (gamestate (phase PRE_GAME) (prev-phase ~PRE_GAME))
  =>
  (modify ?gs (prev-phase PRE_GAME) (game-time 0.0) (state WAIT_START))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights GREEN-ON YELLOW-ON RED-ON))
  )
  (assert (reset-game))
)

(defrule retract-reset-game
  (declare (salience ?*PRIORITY_CLEANUP*))
  ?rf <- (reset-game)
  =>
  (retract ?rf)
)

(defrule start-game
  ?gs <- (gamestate (phase PRE_GAME) (state RUNNING))
  =>
  (modify ?gs (phase EXPLORATION) (prev-phase PRE_GAME))
  (assert (attention-message "Starting game" 5))
)

(defrule switch-to-production
  (time $?now)
  ?gs <- (gamestate (phase EXPLORATION) (state RUNNING)
		    (game-time ?game-time&:(>= ?game-time ?*EXPLORATION-TIME*)))
  =>
  (modify ?gs (phase PRODUCTION) (prev-phase EXPLORATION))
  (assert (attention-message "Switching to production phase" 5))
)

(defrule game-over
  (time $?now)
  ?gs <- (gamestate (phase PRODUCTION) (state RUNNING) (points ?points)
		    (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  =>
  (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION) (state PAUSED))
  (assert (attention-message "Game Over" 60))
  (printout t "-- Awarded Points --" crlf)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (printout t ?phase crlf)
    (bind ?phase-points 0)
    (do-for-all-facts ((?p points)) (eq ?p:phase ?phase)
      (printout t
        (format nil "  %s  %2d  %s" (time-sec-format ?p:game-time) ?p:points ?p:reason) crlf)
      (bind ?phase-points (+ ?phase-points ?p:points))
    )
    (printout t ?phase " TOTAL: " ?phase-points crlf)
  )
  (printout t "OVERALL TOTAL POINTS: " ?points crlf)

  (printout t "===  Game Over  ===" crlf)
)

(defrule goto-post-game
  ?gs <- (gamestate (phase POST_GAME) (prev-phase ~POST_GAME))
  =>
  (modify ?gs (prev-phase POST_GAME))
  (delayed-do-for-all-facts ((?machine machine)) TRUE
    (modify ?machine (desired-lights RED-BLINK))
  )
)

; Sort pucks by ID, such that do-for-all-facts on the puck deftemplate
; iterates in a nice order, e.g. for net-send-PuckInfo
(defrule sort-pucks
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (puck (id ?id-a))
  ?ob <- (puck (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?ob)
  (modify ?oa)
)

(defrule init-game
  ?gf <- (gamestate (state INIT))
  =>
  (modify ?gf (state WAIT_START) (last-time (now)))
)

