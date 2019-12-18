
;---------------------------------------------------------------------------
;  mongodb.clp - LLSF RefBox CLIPS MongoDB logging
;
;  Created: Mon Jun 10 19:06:19 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate mongodb-last-game-report
  (multislot points (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deffunction mongodb-time-as-ms (?time)
	(return (+ (* (nth$ 1 ?time) 1000) (div (nth$ 2 ?time) 1000)))
)

(defrule mongodb-init
  (init)
  =>
  (assert (mongodb-last-game-report))
)

(defrule mongodb-reset
  (reset-game)
  ?f1 <- (mongodb-wrote-game-report $?)
  ?f2 <- (mongodb-last-game-report)
  =>
  (retract ?f1)
  (modify ?f2 (points 0 0))
)

(deffunction mongodb-write-game-report (?teams ?stime ?etime)
  (bind ?doc (bson-create))

  (bson-append-array ?doc "start-timestamp" ?stime)
  (bson-append-time  ?doc "start-time" ?stime)
  (bson-append-array ?doc "teams" ?teams)

  (if (time-nonzero ?etime) then
    (bson-append-time ?doc "end-time" ?etime)
  )

  (bind ?points-arr (bson-array-start))
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
      (bson-builder-destroy ?point-doc)
    )
    (bson-append ?phase-points-doc-cyan ?phase ?phase-points-cyan)
    (bson-append ?phase-points-doc-magenta ?phase ?phase-points-magenta)
    (bind ?points-cyan (+ ?points-cyan ?phase-points-cyan))
    (bind ?points-magenta (+ ?points-magenta ?phase-points-magenta))
  )

  (bson-array-finish ?doc "points" ?points-arr)
  (bson-append ?doc "phase-points-cyan" ?phase-points-doc-cyan)
  (bson-append ?doc "phase-points-magenta" ?phase-points-doc-magenta)
  (bson-append-array ?doc "total-points" (create$ ?points-cyan ?points-magenta))
  (bson-builder-destroy ?phase-points-doc-cyan)
  (bson-builder-destroy ?phase-points-doc-magenta)

  (bind ?m-arr (bson-array-start))
  (do-for-all-facts ((?m machine)) TRUE
     (bind ?m-doc (bson-create))
     (bson-append ?m-doc "name" ?m:name)
     (bson-append ?m-doc "team" ?m:team)
     (bson-append ?m-doc "type" ?m:mtype)
     (bson-append ?m-doc "zone" ?m:zone)
     (bson-append-array ?m-doc "pose" ?m:pose)
     (bson-append ?m-doc "orientation" ?m:rotation)
     (bson-append ?m-doc "productions" ?m:productions)
     (bson-append ?m-doc "proc-time" ?m:proc-time)
     (bson-append-array ?m-doc "down-period" ?m:down-period)

     (if (eq ?m:mtype RS) then
       (bson-append-array ?m-doc "rs-ring-colors" ?m:rs-ring-colors)
     )
     (bson-array-append ?m-arr ?m-doc)
  )
  (bson-array-finish ?doc "machines" ?m-arr)

  (bind ?o-arr (bson-array-start))
  (do-for-all-facts ((?o order)) TRUE
     (bind ?o-doc (bson-create))
     (bson-append ?o-doc "id" ?o:id)
     (bson-append ?o-doc "complexity" ?o:complexity)
     (bson-append ?o-doc "quantity-requested" ?o:quantity-requested)
     (bson-append ?o-doc "base-color" ?o:base-color)
     (bson-append-array ?o-doc "ring-colors" ?o:ring-colors)
     (bson-append ?o-doc "cap-color" ?o:cap-color)
     (bson-append-array ?o-doc "quantity-delivered" ?o:quantity-delivered)
     (bson-append-array ?o-doc "delivery-period" ?o:delivery-period)
     (bson-append ?o-doc "delivery-gate" ?o:delivery-gate)
     (bson-append ?o-doc "activate-at" ?o:activate-at)
     (bson-array-append ?o-arr ?o-doc)
  )
  (bson-array-finish ?doc "orders" ?o-arr)

  ;(printout t "Storing game report" crlf (bson-tostring ?doc) crlf)

  (mongodb-upsert "game_report" ?doc
  		  (str-cat "{\"start-timestamp\": [" (nth$ 1 ?stime) ", " (nth$ 2 ?stime) "]}"))
  (bson-builder-destroy ?doc)
)

(defrule mongodb-game-report-begin
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME) (phase ~PRE_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-wrote-game-report begin $?stime))
  ?f <- (mongodb-last-game-report)
  =>
  (mongodb-write-game-report ?teams ?stime ?etime)
  (assert (mongodb-wrote-game-report begin ?stime))
  (modify ?f (points 0 0))
)

(defrule mongodb-game-report-end
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
						 (phase POST_GAME) (start-time $?stime) (end-time $?etime))
  (not (mongodb-wrote-game-report end $?stime))
  =>
	(printout t "Writing game report to MongoDB" crlf)
  (mongodb-write-game-report ?teams ?stime ?etime)
  (assert (mongodb-wrote-game-report end ?stime))
)

(defrule mongodb-game-report-update
  (declare (salience ?*PRIORITY_HIGH*))
  ?gr <- (mongodb-last-game-report (points $?gr-points))
  (gamestate (state RUNNING)
	     (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime)
	     (points $?points&:(neq ?points ?gr-points)))
  =>
  (mongodb-write-game-report ?teams ?stime ?etime)
  (modify ?gr (points ?points))
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
  (mongodb-insert "clients" ?client-doc)
  (bson-builder-destroy ?client-doc)
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

  (mongodb-update "clients" ?client-update-doc ?update-query)
  (bson-builder-destroy ?client-update-doc)
  (bson-builder-destroy ?update-query)
)


(deffunction mongodb-store-machine-zones (?time)
  (bind ?doc (bson-create))

  (bson-append-array ?doc "timestamp" ?time)
  (bson-append-time  ?doc "time" ?time)
  (bind ?m-arr (bson-array-start))
	
	(do-for-all-facts ((?m machine)) TRUE
    (bind ?m-doc (bson-create))
		(bson-append ?m-doc "name" ?m:name)
		(bson-append ?m-doc "zone" ?m:zone)
		(bson-append ?m-doc "rotation" ?m:rotation)
		(bson-array-append ?m-arr ?m-doc)
		(bson-builder-destroy ?m-doc)
  )

	(bson-array-finish ?doc "machines" ?m-arr)
  (mongodb-upsert "machine_zones" ?doc
  		  (str-cat "{\"timestamp\": [" (nth$ 1 ?time) ", " (nth$ 2 ?time) "]}"))
  (bson-builder-destroy ?doc)

)

(deffunction mongodb-load-machine-zones ()
	; retrieve time range of latest completed game
  ;(bind ?t-query (bson-parse "{\"end-time\": { \"$exists\": 1 }}"))
  (bind ?t-query (bson-parse "{}"))
  (bind ?t-sort  (bson-parse "{\"start-timestamp\": -1}"))
	(bind ?t-cursor (mongodb-query-sort "game_report" ?t-query ?t-sort))
  (bind ?t-doc (mongodb-cursor-next ?t-cursor))
  (if (neq ?t-doc FALSE) then
	  (bind ?stime (bson-get-time ?t-doc "start-time"))
    (bson-destroy ?t-doc)
;	  (bind ?etime (bson-get-time ?t-doc "end-time"))

	  ; retrieve machine config
;		(bind ?qs (str-cat "{\"$and\": [{time: { \"$gte\": { \"$date\": "
;											 (mongodb-time-as-ms ?stime) "}}},"
;											 "{time: { \"$lte\": { \"$date\": "
;											 (mongodb-time-as-ms ?etime) "}}}]}}"))
		;(bind ?qs (str-cat "{\"time\": ISODate(\"" (mongodb-time-as-ms ?stime) "\"}"))
		(bind ?qs (str-cat "{}"))
		(bind ?query (bson-parse ?qs))
		(bind ?sort  (bson-parse "{\"time\": -1}"))
		(bind ?cursor (mongodb-query-sort "machine_zones" ?query ?sort))
		(bind ?doc (mongodb-cursor-next ?cursor))
		(if (neq ?doc FALSE) then
		 then
			(bind ?fn (bson-field-names ?doc))
			(bind ?m-arr (bson-get-array ?doc "machines"))
			(foreach ?m-p ?m-arr
				(bind ?m-name (sym-cat (bson-get ?m-p "name")))
				(bind ?m-zone (sym-cat (bson-get ?m-p "zone")))
				(bind ?m-rotation (sym-cat (bson-get ?m-p "rotation")))
				;(printout t "Machine " ?m-name " is in zone " ?m-zone " with rotation " ?m-rotation crlf)
				(do-for-fact ((?m machine)) (eq ?m:name ?m-name)
					(modify ?m (zone ?m-zone) (rotation (integer (eval ?m-rotation))))
			  )
				(bson-destroy ?m-p)
			)
			(bson-destroy ?doc)
     else
	    (printout error "Empty result in mongoDB from machine_zones" crlf)
    )
    (mongodb-cursor-destroy ?cursor)
	  (bson-builder-destroy ?query)
	  (bson-builder-destroy ?sort)
   else
	  (printout error "Empty result in mongoDB from game_report" crlf)
    
  )
  (mongodb-cursor-destroy ?t-cursor)
	(bson-builder-destroy ?t-query)
	(bson-builder-destroy ?t-sort)
)

(defrule mongodb-store-machine-zones
  (game-parameterized)
	(mongodb-wrote-game-report begin $?stime)
	=>
	(printout t "Storing machine zones to database" crlf)
	(mongodb-store-machine-zones ?stime)
)

(defrule mongodb-load-machine-zones
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(not (confval (path "/llsfrb/game/random-field") (type BOOL) (value true)))
	; load only once
	(not (loaded-machine-zones-from-db))
	=>
	(printout t "Loading machine positions from database" crlf)
	(assert (loaded-machine-zones-from-db))
	(mongodb-load-machine-zones)
)
