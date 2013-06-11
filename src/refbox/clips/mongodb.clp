
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

(deffunction mongodb-write-game-report (?stime ?etime)
  (bind ?doc (bson-create))

  (bson-append-array ?doc "start-timestamp" ?stime)
  (bson-append-time ?doc "start-time" (nth$ 1 ?stime) (nth$ 2 ?stime))

  (if (time-nonzero ?etime) then
    (bson-append-time ?doc "end-time" (nth$ 1 ?etime) (nth$ 2 ?etime))
  )

  (bind ?points-arr (bson-array-start ?doc "points"))
  (bind ?phase-points-doc (bson-create))

  (bind ?points 0)
  (foreach ?phase (deftemplate-slot-allowed-values points phase)
    (bind ?phase-points 0)
    (do-for-all-facts ((?p points)) (eq ?p:phase ?phase)
      (bind ?point-doc (bson-create))
      (bson-append ?point-doc "game-time" ?p:game-time)
      (bson-append ?point-doc "points" ?p:points)
      (bson-append ?point-doc "reason" ?p:reason)
      (bind ?phase-points (+ ?phase-points ?p:points))
      (bson-array-append ?points-arr ?point-doc)
      (bson-destroy ?point-doc)
    )
    (bson-append ?phase-points-doc ?phase ?phase-points)
    (bind ?points (+ ?points ?phase-points))
  )
  (bson-array-finish ?points-arr)
  (bson-append ?doc "phase-points" ?phase-points-doc)
  (bson-append ?doc "total-points" ?points)

  ;(printout t "Storing game report" crlf (bson-tostring ?doc) crlf)

  (mongodb-upsert "llsfrb.game_report" ?doc
  		  (str-cat "{\"start-timestamp\": [" (nth$ 1 ?stime) ", " (nth$ 2 ?stime) "]}"))
)

(defrule mongodb-game-report-begin
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (team ?team&~"") (prev-phase PRE_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-game-report begin $?stime))
  =>
  (mongodb-write-game-report ?stime ?etime)
  (assert (mongodb-game-report begin ?stime))
)

(defrule mongodb-game-report-end
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (team ?team&~"") (phase POST_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-game-report end $?stime))
  =>
  (mongodb-write-game-report ?stime ?etime)
  (assert (mongodb-game-report end ?stime))
)

(defrule mongodb-game-report-finalize
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (team ?team&~"") (start-time $?stime) (end-time $?etime))
  (finalize)
  =>
  (mongodb-write-game-report ?stime ?etime)
)
