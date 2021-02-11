
;---------------------------------------------------------------------------
;  mongodb.clp - LLSF RefBox CLIPS MongoDB logging
;
;  Created: Mon Jun 10 19:06:19 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;             2020  Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------
(defglobal
	; Mongodb Game Report Version,
	?*MONGODB-REPORT-VERSION* = 1.0
	; Update rate in seconds
	?*MONGODB-REPORT-UPDATE-FREQUENCY* = 10
)

(deftemplate mongodb-game-report
	(multislot start (type INTEGER) (cardinality 2 2) (default 0 0))
	(multislot last-updated (type INTEGER) (cardinality 2 2) (default 0 0))
	(multislot end (type INTEGER) (cardinality 2 2) (default 0 0))
	(slot name (type STRING) (default ""))
	(multislot points (type INTEGER) (cardinality 2 2) (default 0 0))
)

(deffunction mongodb-time-as-ms (?time)
	(return (+ (* (nth$ 1 ?time) 1000) (div (nth$ 2 ?time) 1000)))
)

(deffunction mongodb-fact-to-bson (?fact-id)
" Create a bson document from a fact.
  @param ?fact-id Fact to encode as bson document
  @return ?doc bson document containing all slots of ?fact-id
"
	(bind ?doc (bson-create))
	(bind ?slots (fact-slot-names ?fact-id))
	(foreach ?slot ?slots
		(bind ?is-multislot (deftemplate-slot-multip (fact-relation ?fact-id) ?slot))
		(bind ?slot-value (fact-slot-value ?fact-id ?slot))
		(if ?is-multislot
		 then
			(bson-append-array ?doc (str-cat ?slot) ?slot-value)
		 else
			(bson-append ?doc (str-cat ?slot) ?slot-value)
		)
	)
	(return ?doc)
)

(deffunction mongodb-pack-value-to-string (?value ?type)
" Convert a value or list of values to a string, such that CLIPS can retrieve
  the type later on.
  Useful helper when using 'assert-string'.
  @param ?value Value or list of values
  @param ?type Type of the value(s) of ?value
  @return string packed with the value(s)
"
	(bind ?raw-val ?value)
	(bind ?is-list (eq (type ?value) MULTIFIELD))
	(if ?is-list
	 then
		(bind ?tmp ?raw-val)
		(progn$ (?f ?tmp) (bind ?raw-val (replace$ ?raw-val ?f-index ?f-index (type-cast ?f ?type))))
		(bind ?typed-string (implode$ ?raw-val))
	 else
		(bind ?typed-string (str-cat (type-cast ?value ?type)))
		(if (eq ?type STRING) then (bind ?typed-string (str-cat "\"" ?typed-string "\"")))
	)
	(return ?typed-string)
)

(deffunction mongodb-retrieve-value-from-doc (?doc ?field ?type ?is-list)
" Retrieve the value(s) with a given type from a field of a bson document.
  @param ?doc bson document
  @param ?field Name of the field within ?doc that holds the desired value
  @param ?type Type of the value that is retrieved
  @param ?is-list TRUE if the field contains an array, FALSE otherwise
  @return Value(s) of ?field with type ?type
"
	(if ?is-list
	 then
		(bind ?raw-val (bson-get-array ?doc ?field))
		(return (type-cast-list ?raw-val ?type))
	 else
		(bind ?raw-val (bson-get ?doc ?field))
		(return (type-cast ?raw-val ?type))
	)
)

(deffunction mongodb-update-fact-from-bson (?doc ?template ?id-slots $?only-slots)
" Update a fact by the content of a bson document.
  @param ?doc bson document
  @param ?template Template name of the fact that is encoded in ?doc
  @param ?id-slots Name of the slot(s) that uniquely determines the ?template fact
                  that is updated
  @param $?only-slots optional list of slots within ?template that are updated
                      by the values contained in ?doc. If unspecified all slots
                      are updated.
"
	; determine all slots that are updated
	(bind ?slots ?only-slots)
	(if (eq (length$ ?slots) 0)
	 then
		(bind ?slots (deftemplate-slot-names ?template))
	)

	; retrieve fact matching the document
	(bind ?f-list (find-all-facts ((?x ?template)) TRUE))
	(if (neq (type ?id-slots) MULTIFIELD) then (bind ?id-slots (create$ ?id-slots)))
	(loop-for-count (?i 1 (length$ ?id-slots))
		(bind ?curr-slot (nth$ ?i ?id-slots))
		(bind ?type-candidates (deftemplate-slot-types ?template ?curr-slot))
		(if (neq (length$ ?type-candidates) 1)
		 then
			(printout error "mongodb-update-fact-from-bson: Type of identifier slot "
			                ?curr-slot " not uniquely determined: " ?type-candidates crlf)
			(return)
		 else
			(bind ?type (nth$ 1 ?type-candidates))
			(bind ?curr-value (mongodb-retrieve-value-from-doc
			                    ?doc
			                    ?curr-slot
			                    ?type
			                    (deftemplate-slot-multip ?template ?curr-slot)))
			; filter out candidates that do not match the current id field
			(bind ?j 1)
			(loop-for-count (length$ ?f-list)
				(bind ?f-entry (nth$ ?j ?f-list))
				(if (neq (fact-slot-value ?f-entry ?curr-slot) ?curr-value)
				 then
					(bind ?f-pos (member$ ?f-entry ?f-list))
					(bind ?f-list (delete$ ?f-list ?f-pos ?f-pos))
				 else
					(bind ?j (+ ?j 1))
				)
			)
		)
	)
	; check if a single fact is identified for modification
	(if (<= 1 (length$ ?f-list))
		then (bind ?f (nth$ 1 ?f-list))
		(if (< 1 (length$ ?f-list)) then
			(printout warn "mongodb-update-fact-from-bson: Ambiguous Fact match" crlf)
		)
	 else
		(printout error "mongodb-update-fact-from-bson: No " ?template
		                " fact with identifiers " ?id-slots
		                " found that matches doc values" crlf)
		(return)
	)

	; build string of the updated fact
	(bind ?update-str (str-cat "(" ?template))
	(foreach ?slot (deftemplate-slot-names ?template)
		(bind ?is-multislot (deftemplate-slot-multip ?template ?slot))
		(bind ?types (deftemplate-slot-types ?template ?slot))
		(if (neq (length$ ?types) 1)
		 then
			(printout error "mongodb-bson-to-fact: type of slot " ?slot
			                " of template "  ?template " cannot be determined, skipping." crlf)
		 else
			(bind ?type (nth$ 1 ?types))
			(if (member$ ?slot ?slots)
			 then
				(bind ?value (mongodb-retrieve-value-from-doc ?doc ?slot ?type ?is-multislot))
			 else
				(bind ?value (fact-slot-value ?f ?slot))
			)
			(bind ?value (mongodb-pack-value-to-string ?value ?type))
			(bind ?update-str (str-cat ?update-str " (" ?slot " " ?value ")"))
		)
	)
	(bind ?update-str (str-cat ?update-str ")"))

	; update the fact
	(retract ?f)
	(assert-string ?update-str)
)

(deffunction mongodb-write-game-report(?doc ?stime)
" Upsert a game report to mongodb.
  @param ?doc bson document storing the game report
  @param ?stime start time of the report
"
	(mongodb-upsert "game_report" ?doc
	  (str-cat "{\"start-timestamp\": [" (nth$ 1 ?stime) ", " (nth$ 2 ?stime) "]}"))
	(bson-builder-destroy ?doc)
)

(deffunction mongodb-load-fact-from-game-report (?report-name ?fact ?template ?id-slot $?only-slots)
" Update fact with values from a game report.
  @param ?report-name Name of the report from which data is loaded. In case
                      Multiple reports have the same name the newest one is
                      chosen.
  @param ?fact field name within game reports that contains the fact
  @param ?template Template name of the fact that is encoded in ?facts
  @param ?slot-id Name of the slot that uniquely determines the ?template fact
                  that is updated
  @param $?only-slots optional list of slots within ?template that are updated
                      by the values contained in ?doc. If unspecified all slots
                      are updated.
  @return TRUE if the game report has a field name ?facts, FALSE otherwise.
"
	(bind ?success FALSE)
	(bind ?t-query (bson-parse "{}"))
	(if (neq ?report-name "")
	 then
		(bind ?t-query (bson-parse (str-cat "{\"report-name\": \"" ?report-name "\"}")))
	)
	(bind ?t-sort  (bson-parse "{\"start-timestamp\": -1}"))
	(bind ?t-cursor (mongodb-query-sort "game_report" ?t-query ?t-sort))
	(bind ?t-doc (mongodb-cursor-next ?t-cursor))
	(if (neq ?t-doc FALSE) then
	 then
		(bind ?m-p (bson-get ?t-doc ?fact))
		(mongodb-update-fact-from-bson ?m-p ?template ?id-slot ?only-slots)
		(bson-destroy ?m-p)
		(bind ?success TRUE)
	 else
		(printout error "Empty result in mongoDB from game_report for fact " ?template crlf)
	)
	(bson-destroy ?t-doc)
	(mongodb-cursor-destroy ?t-cursor)
	(bson-builder-destroy ?t-query)
	(bson-builder-destroy ?t-sort)
	(return ?success)
)

(deffunction mongodb-load-facts-from-game-report (?report-name ?facts ?template ?id-slot $?only-slots)
" Update facts with values from a game report.
  @param ?report-name Name of the report from which data is loaded. In case
                      Multiple reports have the same name the newest one is
                      chosen.
  @param ?facts field name within game reports that contains the facts
  @param ?template Template name of the fact that is encoded in ?facts
  @param ?slot-id Name of the slot that uniquely determines the ?template fact
                  that is updated
  @param $?only-slots optional list of slots within ?template that are updated
                      by the values contained in ?doc. If unspecified all slots
                      are updated.
  @return TRUE if the game report has a field name ?facts, FALSE otherwise.
"
	(bind ?success FALSE)
	(bind ?t-query (bson-parse "{}"))
	(if (neq ?report-name "")
	 then
		(bind ?t-query (bson-parse (str-cat "{\"report-name\": \"" ?report-name "\"}")))
	)
	(bind ?t-sort  (bson-parse "{\"start-timestamp\": -1}"))
	(bind ?t-cursor (mongodb-query-sort "game_report" ?t-query ?t-sort))
	(bind ?t-doc (mongodb-cursor-next ?t-cursor))
	(if (neq ?t-doc FALSE) then
	 then
		(bind ?m-arr (bson-get-array ?t-doc ?facts))
		(foreach ?m-p ?m-arr
			(mongodb-update-fact-from-bson ?m-p ?template ?id-slot ?only-slots)
			(bson-destroy ?m-p)
		)
		(bind ?success TRUE)
	 else
		(printout error "Empty result in mongoDB from game_report" crlf)
	)
	(bson-destroy ?t-doc)
	(mongodb-cursor-destroy ?t-cursor)
	(bson-builder-destroy ?t-query)
	(bson-builder-destroy ?t-sort)
	(return ?success)
)

(deffunction mongodb-create-game-report (?teams ?stime ?etime ?report-name)
  (bind ?doc (bson-create))

  (bson-append-array ?doc "start-timestamp" ?stime)
  (bson-append-time  ?doc "start-time" ?stime)
  (bson-append-array ?doc "teams" ?teams)
  (bson-append ?doc "report-name" ?report-name)
	(bson-append ?doc "report-version" ?*MONGODB-REPORT-VERSION*)

  (if (time-nonzero ?etime) then
    (bson-append-time ?doc "end-time" ?etime)
  )

	(do-for-fact ((?p gamestate)) TRUE
		(bind ?gamestate-doc (mongodb-fact-to-bson ?p))
		(bson-append ?doc (str-cat "gamestate/" ?p:phase) ?gamestate-doc)
		(bson-builder-destroy ?gamestate-doc)
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
      (bind ?point-doc (mongodb-fact-to-bson ?p))
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

	(bind ?o-arr (bson-array-start))
	(do-for-all-facts ((?o order)) TRUE
		(bind ?order-doc (mongodb-fact-to-bson ?o))
		(bson-array-append ?o-arr ?order-doc)
		(bson-builder-destroy ?order-doc)
	)
	(bson-array-finish ?doc "orders" ?o-arr)

  ;(printout t "Storing game report" crlf (bson-tostring ?doc) crlf)
	(return ?doc)
)


(defrule mongodb-reset
	(reset-game)
	?f1 <- (mongodb-game-report)
	=>
	(modify ?f1 (points 0 0) (end 0 0))
)


(defrule mongodb-game-report-begin
  (declare (salience ?*PRIORITY_HIGH*))
	?gp <- (game-parameters (is-parameterized TRUE))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME) (phase ~PRE_GAME) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
  (not (mongodb-game-report (start $?stime) (name ?report-name)))
  =>
  (assert (mongodb-game-report (start ?stime) (name ?report-name)))
  (bind ?doc (mongodb-create-game-report ?teams ?stime ?etime ?report-name))
	; store information describing the game setup only once
	(bind ?m-arr (bson-array-start))
	(do-for-all-facts ((?m ring-spec)) TRUE
		(bind ?ring-spec-doc (mongodb-fact-to-bson ?m))
		(bson-array-append ?m-arr ?ring-spec-doc)
		(bson-builder-destroy ?ring-spec-doc)
	)
	(bson-array-finish ?doc "ring-specs" ?m-arr)
	(bind ?m-arr (bson-array-start))
	(do-for-all-facts ((?m machine-ss-shelf-slot)) TRUE
		(bind ?ss-doc (mongodb-fact-to-bson ?m))
		(bson-array-append ?m-arr ?ss-doc)
		(bson-builder-destroy ?ss-doc)
	)
	(bson-array-finish ?doc "machine-ss-shelf-slots" ?m-arr)
	(bind ?m-arr (bson-array-start))
	(do-for-all-facts ((?m machine)) TRUE
		(bind ?machine-doc (mongodb-fact-to-bson ?m))
		(bson-array-append ?m-arr ?machine-doc)
		(bson-builder-destroy ?machine-doc)
	)
	(bson-array-finish ?doc "machines" ?m-arr)
	(mongodb-write-game-report ?doc ?stime)
)

(defrule mongodb-game-report-end
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
						 (phase POST_GAME) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
  ?gr <- (mongodb-game-report (start $?stime) (name ?report-name) (end $?end&:(neq ?end ?etime)))
  =>
	(printout t "Writing game report to MongoDB" crlf)
	(modify ?gr (end ?etime))
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime)
)

(defrule mongodb-game-report-update
  (declare (salience ?*PRIORITY_HIGH*))
	?gr <- (mongodb-game-report (points $?gr-points) (name ?report-name))
  (gamestate (state RUNNING)
	     (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime)
	     (points $?points&:(neq ?points ?gr-points)))
  =>
  (modify ?gr (points ?points))
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime)
)

(defrule mongodb-game-report-finalize
  (declare (salience ?*PRIORITY_HIGH*))
  (gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime))
  ?gr <- (mongodb-game-report (points $?gr-points) (name ?report-name))
  (finalize)
  =>
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime)
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


(defrule mongodb-restore-gamestate
	(declare (salience ?*PRIORITY_FIRST*))
	(time $?now)
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/restore-gamestate/enable") (type BOOL) (value true))
	(confval (path "/llsfrb/game/restore-gamestate/phase") (type STRING) (value ?p))
	?gp <- (game-parameters (gamestate PENDING))
	=>
	(printout t "Loading gamestate from database" crlf)
	(if (mongodb-load-fact-from-game-report ?report-name
	                                         (sym-cat "gamestate/" ?p)
	                                         gamestate
	                                         (create$))
	 then
		(printout t "Loading gamestate finished" crlf)
		(modify ?gp (gamestate RECOVERED))
		; ensure that time elapses from now on
		(do-for-fact ((?g gamestate)) TRUE
			(modify ?g (last-time $?now))
		)
	 else
		(printout error "Loading gamestate from database failed, fallback to fresh one." crlf)
		(modify ?gp (gamestate FRESH))
	)
)

(defrule mongodb-load-storage-status
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/random-storage") (type BOOL) (value false))
	?gp <- (game-parameters (storage-status PENDING))
	=>
	(printout t "Loading storage from database" crlf)
	(if (mongodb-load-facts-from-game-report ?report-name
	                                         "machine-ss-shelf-slots"
	                                         machine-ss-shelf-slot
	                                         (create$ name position))
	 then
		(printout t "Loading storage status finished" crlf)
		(modify ?gp (storage-status STATIC))
	 else
		(printout error "Loading storage status from database failed, fallback to random generation." crlf)
		(modify ?gp (storage-status RANDOM))
	)
)

(defrule mongodb-load-orders
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/random-orders") (type BOOL) (value false))
	?gp <- (game-parameters (orders PENDING))
	=>
	(printout t "Loading orders from database" crlf)
	(if (mongodb-load-facts-from-game-report ?report-name
	                                         "orders"
	                                          order
	                                          id
	                                          (create$ complexity competitive
	                                                   quantity-requested base-color
	                                                   ring-colors cap-color
	                                                   delivery-period delivery-gate
	                                                   activate-at
	                                                   allow-overtime))
	 then
		(printout t "Loading orders finished" crlf)
		(modify ?gp (orders STATIC))
	 else
		(printout error "Loading orders from database failed, fallback to random generation." crlf)
		(modify ?gp (orders RANDOM))
	)
)

(defrule mongodb-load-machine-setup
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/random-machine-setup") (type BOOL) (value false))
	?gp <- (game-parameters (machine-setup PENDING))
	=>
	(printout t "Loading machine setup from database" crlf)
	(if (and (mongodb-load-facts-from-game-report ?report-name
	                                              "machines"
	                                              machine
	                                              name
	                                              (create$ down-period rs-ring-colors))
	         (mongodb-load-facts-from-game-report ?report-name
	                                              "ring-specs"
	                                              ring-spec
	                                              color))
	 then
		(printout t "Loading machine-setup finished" crlf)
		(modify ?gp (machine-setup STATIC))
	 else
		(printout error "Loading machines from database failed, fallback to random generation." crlf)
		(modify ?gp (machine-setup RANDOM))
	)
)

(defrule mongodb-load-machine-zones
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(not (confval (path "/llsfrb/game/random-field") (type BOOL) (value true)))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	?gp <- (game-parameters (machine-positions PENDING))
	=>
		(printout t "Loading machine positions from database" crlf)
	(if (mongodb-load-facts-from-game-report ?report-name
	                                         "machines"
	                                         machine
	                                         name
	                                         (create$ zone rotation))
	 then
		(printout t "Loading machine positions finished" crlf)
		(modify ?gp (machine-positions STATIC))
	 else
		(printout error "Loading machines from database failed, fallback to random generation." crlf)
		(modify ?gp (machine-positions RANDOM))
	)
)
