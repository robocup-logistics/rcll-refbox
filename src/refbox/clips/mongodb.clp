
;---------------------------------------------------------------------------
;  mongodb.clp - LLSF RefBox CLIPS MongoDB logging
;
;  Created: Mon Jun 10 19:06:19 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate mongodb-wrote-gamereport (multislot implied))

(defrule mongodb-reset
  (reset-game)
  =>
  (delayed-do-for-all-facts ((?r mongodb-wrote-gamereport)) TRUE (retract ?r))
)

(deffunction mongodb-write-game-report (?teams ?stime ?etime)
  (bind ?doc (bson-create))

  (bson-append-array ?doc "start-timestamp" ?stime)
  (bson-append-time  ?doc "start-time" ?stime)
  (bson-append-array ?doc "teams" ?teams)

  (if (time-nonzero ?etime) then
    (bson-append-time ?doc "end-time" ?etime)
  )

  (bind ?points-arr (bson-array-start ?doc "points"))
  (bind ?phase-points-doc-cyan (bson-create))
  (bind ?phase-points-doc-magenta (bson-create))

  (bind ?points-cyan 0)
  (bind ?points-magenta 0)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (bind ?phase-points-cyan 0)
    (bind ?phase-points-magenta 0)
    (do-for-all-facts ((?p points)) (eq ?p:phase ?phase)
      (bind ?point-doc (bson-create))
      (bson-append ?point-doc "game-time" ?p:game-time)
      (bson-append ?point-doc "team"   ?p:team)
      (bson-append ?point-doc "points" ?p:points)
      (bson-append ?point-doc "reason" ?p:reason)
      (if (eq ?p:team CYAN)
        then (bind ?phase-points-cyan (+ ?phase-points-cyan ?p:points))
        else (bind ?phase-points-magenta (+ ?phase-points-magenta ?p:points))
      )
      (bson-array-append ?points-arr ?point-doc)
      (bson-destroy ?point-doc)
    )
    (bson-append ?phase-points-doc-cyan ?phase ?phase-points-cyan)
    (bson-append ?phase-points-doc-magenta ?phase ?phase-points-magenta)
    (bind ?points-cyan (+ ?points-cyan ?phase-points-cyan))
    (bind ?points-magenta (+ ?points-magenta ?phase-points-magenta))
  )
  (bson-array-finish ?points-arr)
  (bson-append ?doc "phase-points-cyan" ?phase-points-doc-cyan)
  (bson-append ?doc "phase-points-magenta" ?phase-points-doc-magenta)
  (bson-append-array ?doc "total-points" (create$ ?points-cyan ?points-magenta))
  (bson-destroy ?phase-points-doc-cyan)
  (bson-destroy ?phase-points-doc-magenta)

  ;(printout t "Storing game report" crlf (bson-tostring ?doc) crlf)

  (mongodb-upsert "llsfrb.game_report" ?doc
  		  (str-cat "{\"start-timestamp\": [" (nth$ 1 ?stime) ", " (nth$ 2 ?stime) "]}"))
  (bson-destroy ?doc)
)

(defrule mongodb-game-report-begin
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-game-report begin $?stime))
  =>
  (mongodb-write-game-report ?teams ?stime ?etime)
  (assert (mongodb-game-report begin ?stime))
)

(defrule mongodb-game-report-end
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (phase POST_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-game-report end $?stime))
  =>
  (mongodb-write-game-report ?teams ?stime ?etime)
  (assert (mongodb-game-report end ?stime))
)

(defrule mongodb-game-report-finalize
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime))
  (finalize)
  =>
  (mongodb-write-game-report ?teams ?stime ?etime)
)

(defrule mongodb-net-client-connected
  (declare (salience ?*PRIORITY_HIGH*))
  (protobuf-server-client-connected ?client-id ?host ?port)
  =>
  (bind ?client-doc (bson-create))
  (bson-append-time ?client-doc "session" ?*START-TIME*)
  (bson-append-time ?client-doc "connect-time" (now))
  (bson-append ?client-doc "client-id" ?client-id)
  (bson-append ?client-doc "host" ?host)
  (bson-append ?client-doc "port" ?port)
  (mongodb-insert "llsfrb.clients" ?client-doc)
  (bson-destroy ?client-doc)
)

(defrule mongodb-net-client-disconnected
  (declare (salience ?*PRIORITY_HIGH*))
  (protobuf-server-client-disconnected ?client-id)
  =>
  (bind ?client-update-doc (bson-create))
  (bson-append-time ?client-update-doc "disconnect-time" (now))

  (bind ?update-query (bson-create))
  (bson-append-time ?update-query "session" ?*START-TIME*)
  (bson-append ?update-query "client-id" ?client-id)

  (mongodb-update "llsfrb.clients" ?client-update-doc ?update-query)
  (bson-destroy ?client-update-doc)
  (bson-destroy ?update-query)
)
