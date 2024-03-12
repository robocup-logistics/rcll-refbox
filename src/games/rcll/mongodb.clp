
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
	; 1.1 -> includes config
	; 1.2 -> mps meta facts, workpiece and agent-task info
	?*MONGODB-REPORT-VERSION* = 2.0
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

(deffunction mongodb-create-doc-from-key-val (?key-val)
  (bind ?doc (bson-create))
  (bind ?index 1)
  (while (< ?index (length$ ?key-val)) do
     (bind ?key (nth$ ?index ?key-val))
     (bind ?value (nth$ (+ ?index 1) ?key-val))
	 (bson-append ?doc (snake-case ?key) ?value)
	 (bind ?index (+ ?index 2))
  )
  (return ?doc)
)

(deffunction mongodb-time-as-ms (?time)
	(return (+ (* (nth$ 1 ?time) 1000) (div (nth$ 2 ?time) 1000)))
)

(deffunction mongodb-fact-to-bson-append (?doc ?fact-id $?only-slots)
" Update a bson document from a fact.
  @param ?doc document to update
  @param ?fact-id Fact to encode as bson document
  @param $?only-slots optional list of slots within ?fact that are stored.
                      If unspecified all slots are stored.
  @return ?doc bson document containing all slots of ?fact-id
"
	(bind ?slots ?only-slots)
	(if (eq (length$ ?slots) 0)
	 then
		(bind ?slots (fact-slot-names ?fact-id))
	)
	(foreach ?slot ?slots
		(bind ?is-multislot (deftemplate-slot-multip (fact-relation ?fact-id) ?slot))
		(bind ?slot-value (fact-slot-value ?fact-id ?slot))
		(if ?is-multislot
		 then
			(bson-append-array ?doc (snake-case ?slot) ?slot-value)
		 else
			(bson-append ?doc (snake-case ?slot) ?slot-value)
		)
	)
	(return ?doc)
)

(deffunction mongodb-fact-to-bson (?fact-id $?only-slots)
" Create a bson document from a fact.
  @param ?fact-id Fact to encode as bson document
  @param $?only-slots optional list of slots within ?fact that are stored.
                      If unspecified all slots are stored.
  @return ?doc bson document containing all slots of ?fact-id
"
	(bind ?doc (bson-create))
	(return (mongodb-fact-to-bson-append ?doc ?fact-id $?only-slots))
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


(deffunction mongodb-get-fact-from-bson (?doc ?template ?id-slots)
" Get a unique fact by the content of a bson document, if it exists.
  @param ?doc bson document
  @param ?template Template name of the fact that is encoded in ?doc
  @param ?id-slots Name of the slot(s) that uniquely determines the ?template fact
                  that is updated
  @return: Unique fact matching the document or FALSE if it cannot be found.
"
	; retrieve fact matching the document
	(bind ?f-list (find-all-facts ((?x ?template)) TRUE))
	(if (neq (type ?id-slots) MULTIFIELD) then (bind ?id-slots (create$ ?id-slots)))
	(loop-for-count (?i 1 (length$ ?id-slots))
		(bind ?curr-slot (nth$ ?i ?id-slots))
		(bind ?type-candidates (deftemplate-slot-types ?template ?curr-slot))
		(if (neq (length$ ?type-candidates) 1)
		 then
			(printout error "mongodb-get-fact-from-bson: Type of identifier slot "
			                ?curr-slot " not uniquely determined: " ?type-candidates crlf)
			(return FALSE)
		 else
			(bind ?type (nth$ 1 ?type-candidates))
			(bind ?curr-value (mongodb-retrieve-value-from-doc
			                    ?doc
			                    (snake-case ?curr-slot)
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
	then
		(bind ?f (nth$ 1 ?f-list))
		(if (< 1 (length$ ?f-list)) then
			(printout debug "mongodb-get-fact-from-bson: Ambiguous Fact match" crlf)
			(return FALSE)
		)
		(return ?f)
	 else
		(printout debug "mongodb-get-fact-from-bson: No " ?template
		                " fact with identifiers " ?id-slots
		                " found that matches doc values" crlf)
		(return FALSE)
	)
)

(deffunction mongodb-update-fact-from-bson (?doc ?fact $?only-slots)
" Update a fact by the content of a bson document.
  @param ?doc bson document
  @param ?fact fact to be updated
  @param $?only-slots optional list of slots within ?fact that are updated
                      by the values contained in ?doc. If unspecified all slots
                      are updated.
"
	; determine all slots that are updated
	(bind ?slots ?only-slots)
	(if (eq (length$ ?slots) 0)
	 then
		(bind ?slots (fact-slot-names ?fact))
	)
	(bind ?doc-slots (bson-field-names ?doc))
	(foreach ?slot ?slots
		(if (not (member$ (str-cat (snake-case ?slot)) ?doc-slots))
		then
			(printout error "mongodb-update-fact-from-bson: Skipping document, missing slot " (snake-case ?slot) crlf)
			(return FALSE)
		)
	)
	(bind ?template (fact-relation ?fact))

	; build string of the updated fact
	(bind ?update-str (str-cat "(" ?template))
	(foreach ?slot (deftemplate-slot-names ?template)
		(bind ?is-multislot (deftemplate-slot-multip ?template ?slot))
		(bind ?types (deftemplate-slot-types ?template ?slot))
		(if (neq (length$ ?types) 1)
		 then
			(printout error "mongodb-update-fact-from-bson: type of slot " ?slot
			                " of template "  ?template " cannot be determined, skipping." crlf)
		 else
			(bind ?type (nth$ 1 ?types))
			(if (member$ ?slot ?slots)
			 then
				(bind ?value (mongodb-retrieve-value-from-doc ?doc (snake-case ?slot) ?type ?is-multislot))
			 else
				(bind ?value (fact-slot-value ?fact ?slot))
			)
			(bind ?value (pack-value-to-string ?value ?type))
			(bind ?update-str (str-cat ?update-str " (" ?slot " " ?value ")"))
		)
	)
	(bind ?update-str (str-cat ?update-str ")"))

	; update the fact
	(retract ?fact)
	(assert-string ?update-str)
)

(deffunction mongodb-write-game-report(?doc ?stime ?report-name)
" Upsert a game report to mongodb.
  @param ?doc bson document storing the game report
  @param ?stime start time of the report
"
	(mongodb-upsert "game_report" ?doc
	  (str-cat "{\"start_timestamp\": [" (nth$ 1 ?stime) ", " (nth$ 2 ?stime) "], \"report_name\": \"" ?report-name "\"}"))
	(bson-builder-destroy ?doc)
)

(deffunction mongodb-retrieve-report (?report-name)
" Get the latest game report matching the given name.
  The caller of this function is responsible to call bson-destroy on the
  returned doc.
  @param ?report-name: report name to fetch
  @return: bson document of the latest report matching this name or FALSE if it
           does not exist
"
	(bind ?t-query (bson-parse "{}"))
	(if (neq ?report-name "")
	 then
		(bind ?t-query (bson-parse (str-cat "{\"report_name\": \"" ?report-name "\"}")))
	)
	(bind ?t-sort  (bson-parse "{\"start_timestamp\": -1}"))
	(bind ?t-cursor (mongodb-query-sort "game_report" ?t-query ?t-sort))
	(bind ?t-doc (mongodb-cursor-next ?t-cursor))
	(bson-builder-destroy ?t-query)
	(mongodb-cursor-destroy ?t-cursor)
	(bson-builder-destroy ?t-sort)
	(return ?t-doc)
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
	(bind ?t-doc (mongodb-retrieve-report ?report-name))
	(if (neq ?t-doc FALSE) then
	 then
		(if (bind ?m-p (bson-get ?t-doc ?fact))
		 then
			(mongodb-update-fact-from-bson ?m-p (mongodb-get-fact-from-bson ?m-p ?template ?id-slot) ?only-slots)
			(bson-destroy ?m-p)
			(bind ?success TRUE)
		 else
			(printout error "Specified game report does not contain field " ?fact crlf)
		)
	(bson-destroy ?t-doc)
	 else
		(printout error "Empty result in mongoDB from game_report for fact " ?template crlf)
	)
	(return ?success)
)

(deffunction mongodb-load-some-facts-from-game-report (?report-name ?facts ?template ?id-slot $?only-slots)
" Update facts with values from a game report.
  Expects for least one document in the ?facts array that there exists a
  corresponding fact that should be updated.

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
  @return TRUE if at least one fact was updated, FALSE otherwise.
"
	(bind ?success FALSE)
	(bind ?t-doc (mongodb-retrieve-report ?report-name))
	(if (neq ?t-doc FALSE) then
	 then
		(if (bind ?m-arr (bson-get-array ?t-doc ?facts))
		 then
			(bind ?success FALSE)
			(foreach ?m-p ?m-arr
				(bind ?f (mongodb-get-fact-from-bson ?m-p ?template ?id-slot))
				(if ?f
					then
					(if (mongodb-update-fact-from-bson ?m-p ?f ?only-slots)
					then
						(bind ?success TRUE)
					)
				)
				(bson-destroy ?m-p)
			)
		 else
			(printout error "Specified game report does not contain field " ?facts crlf)
		)
		(bson-destroy ?t-doc)
	 else
		(printout error "Empty result in mongoDB from game_report" crlf)
	)
	(return ?success)
)

(deffunction mongodb-load-all-facts-from-game-report (?report-name ?facts ?template ?id-slot $?only-slots)
" Update facts with values from a game report.
  Expects for each document in the ?facts array that there exists a
  corresponding fact that should be updated.

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
  @return TRUE if each fact from the database is updated, FALSE otherwise.
"
	(bind ?success FALSE)
	(bind ?t-doc (mongodb-retrieve-report ?report-name))
	(if (neq ?t-doc FALSE) then
	 then
		(if (bind ?m-arr (bson-get-array ?t-doc ?facts))
		 then
			(bind ?success TRUE)
			(foreach ?m-p ?m-arr
				(bind ?f (mongodb-get-fact-from-bson ?m-p ?template ?id-slot))
				(if ?f
				then
					(if (not (mongodb-update-fact-from-bson ?m-p ?f ?only-slots))
					then
						(bind ?success FALSE)
					)
				else
					(bind ?success FALSE)
				)
				(bson-destroy ?m-p)
			)
		 else
			(printout error "Specified game report does not contain field " ?facts crlf)
		)
		(bson-destroy ?t-doc)
	 else
		(printout error "Empty result in mongoDB from game_report" crlf)
	)
	(return ?success)
)

(deffunction get-sorted-history (?fact-list)
	(bind ?return-arr (bson-array-start))
	(bind ?fact-list (sort history> ?fact-list))
	(progn$ (?fact ?fact-list)
		(bind ?history-doc (mongodb-fact-to-bson ?fact))
		(bson-append-time ?history-doc "time" (fact-slot-value ?fact time))
		(bson-array-append ?return-arr ?history-doc)
		(bson-builder-destroy ?history-doc)
	)
	(return ?return-arr)
)

(deffunction mongodb-update-game-report (?doc ?teams ?stime ?etime ?report-name)
	(if (time-nonzero ?etime) then
		(bson-append-time ?doc "end_time" ?etime)
	)

	(do-for-fact ((?p gamestate) (?ti time-info)) TRUE
		(bind ?gamestate-doc (mongodb-fact-to-bson ?p))
		(bind ?gamestate-doc (mongodb-fact-to-bson-append ?gamestate-doc ?ti))
		(bson-append ?doc "gamestate" ?gamestate-doc)
		(bson-builder-destroy ?gamestate-doc)
	)
    (bind ?gamestate-arr (get-sorted-history (find-all-facts ((?h gamestate-history)) TRUE)))
	(bson-array-finish ?doc "gamestate_history" ?gamestate-arr)

	(bind ?points-arr (bson-array-start))

	(do-for-all-facts ((?p points)) TRUE
		(bind ?point-doc (mongodb-fact-to-bson ?p))
		(bson-array-append ?points-arr ?point-doc)
		(bson-builder-destroy ?point-doc)
	)

	(bson-array-finish ?doc "points" ?points-arr)

	(bind ?o-arr (bson-array-start))
	(do-for-all-facts ((?o order)) TRUE
		(bind ?order-doc (mongodb-fact-to-bson ?o))
		(bson-array-append ?o-arr ?order-doc)
		(bson-builder-destroy ?order-doc)
	)
	(bson-array-finish ?doc "orders" ?o-arr)
	(bind ?cfg-arr (bson-array-start))
	(do-for-all-facts ((?cfg confval)) TRUE
		(bind ?cfg-doc (mongodb-fact-to-bson ?cfg))
		(bson-array-append ?cfg-arr ?cfg-doc)
		(bson-builder-destroy ?cfg-doc)
	)
	(bson-array-finish ?doc "config" ?cfg-arr)
	(bind ?machine-history-arr (bson-array-start))
	(unwatch facts machine bs-meta cs-meta rs-meta ds-meta ss-meta)
	(bind ?machine-histories (find-all-facts ((?mh machine-history)) TRUE))
	(progn$ (?mh ?machine-histories)
		(bind ?history-doc (mongodb-fact-to-bson ?mh))
		(bson-append-time ?history-doc "time" (fact-slot-value ?mh time))
		(bind ?temp-fact FALSE)
		(bind ?temp-fact (assert-string (fact-slot-value ?mh meta-fact-string)))
		(bind ?machine-meta-doc FALSE)
		(bind ?m-name (fact-slot-value ?mh name))
		(bind ?m-type (sym-cat (sub-string 3 4 ?m-name)))
		(bind ?meta-fact FALSE)
		(if ?temp-fact
		 then
			(bind ?meta-fact ?temp-fact)
		 else
			; for some reason clips crashes, if the meta-fact-name is passed
			; on-the-fly. Therefore, store it via bind first.
			(bind ?meta-fact-name (sym-cat (lowcase ?m-type) -meta))
			(bind ?machine-meta-facts (find-fact ((?m ?meta-fact-name)) (eq ?m-name ?m-name)))
			(if ?machine-meta-facts then
			  (bind ?meta-fact (nth$ 1 ?machine-meta-facts))
			)
		)
		(if ?meta-fact then
		  (if (eq ?m-type SS) then
		    (bind ?machine-meta-doc (mongodb-fact-to-bson-append ?history-doc ?meta-fact (remove$ (fact-slot-names ?meta-fact) current-shelf-slot)))
		    (bind ?positions (fact-slot-value ?meta-fact current-shelf-slot))
		    (bson-append ?history-doc "shelf" (nth$ 1 ?positions))
		    (bson-append ?history-doc "slot" (nth$ 2 ?positions))

		   else
		    (bind ?machine-meta-doc (mongodb-fact-to-bson-append ?history-doc ?meta-fact))
		  )
		 else
		  (printout warn "mongodb: machine history fact " ?m-name " without machine meta fact!" crlf)
		)
		(if ?temp-fact then
			(retract ?temp-fact)
		)
		(bson-array-append ?machine-history-arr ?history-doc)
		(bson-builder-destroy ?history-doc)
	)
	(watch facts machine bs-meta cs-meta rs-meta ds-meta ss-meta)
	(bson-array-finish ?doc "machine_history" ?machine-history-arr)

    (bind ?shelf-slot-history-arr (get-sorted-history (find-all-facts ((?h shelf-slot-history)) TRUE)))
	(bson-array-finish ?doc "shelf_slot_history" ?shelf-slot-history-arr)

	(bind ?workpiece-arr (bson-array-start))
	(bind ?fact-list (find-all-facts ((?wp workpiece)) TRUE))
	(bind ?fact-list (sort start-time> ?fact-list))
	(progn$ (?fact ?fact-list)
		(bind ?history-doc (mongodb-fact-to-bson ?fact))
		(bson-array-append ?workpiece-arr ?history-doc)
		(bson-builder-destroy ?history-doc)
	)
	(bson-array-finish ?doc "workpiece_history" ?workpiece-arr)

	(bind ?agent-task-arr (bson-array-start))
	(bind ?agent-tasks (find-all-facts ((?at agent-task)) TRUE))
	(progn$ (?at ?agent-tasks)
		(bind ?task-doc (mongodb-fact-to-bson ?at (remove$ (fact-slot-names ?at) task-parameters)))
		(bind ?task-param-doc (mongodb-create-doc-from-key-val (fact-slot-value ?at task-parameters)))
		(bson-append ?task-doc "task_parameters" ?task-param-doc)
		(bson-array-append ?agent-task-arr ?task-doc)
	)
	(bson-array-finish ?doc "agent_task_history" ?agent-task-arr)

    (bind ?robot-history-arr (get-sorted-history (find-all-facts ((?h robot-history)) TRUE)))
	(bson-array-finish ?doc "robot_history" ?robot-history-arr)

	(return ?doc)
)

(deffunction mongodb-create-game-report (?teams ?stime ?etime ?report-name)
  (bind ?doc (bson-create))
  (return (mongodb-update-game-report ?doc ?teams ?stime ?etime ?report-name))
)

(deftemplate mongodb-phase-change
	(multislot registered-phases (type SYMBOL) (default (create$)))
)

(deffunction mongodb-init-report (?teams ?stime ?etime ?report-name)
	(do-for-all-facts ((?game-report mongodb-game-report)) TRUE
		(retract ?game-report)
	)
	(do-for-all-facts ((?phase-change mongodb-phase-change)) TRUE
		(retract ?phase-change)
	)
	(do-for-all-facts ((?machine-history machine-history)) TRUE
		(retract ?machine-history)
	)
	(assert (mongodb-game-report (start ?stime) (name ?report-name)))
	(bind ?doc (bson-create))

	(bson-append-array ?doc "start_timestamp" ?stime)
	(bson-append-time  ?doc "start_time" ?stime)
	(bson-append-array ?doc "teams" ?teams)
	(bson-append ?doc "report_name" ?report-name)
	(bson-append ?doc "report_version" ?*MONGODB-REPORT-VERSION*)
	; store information describing the game setup only once
	(bind ?ring-spec-arr (bson-array-start))
	(do-for-all-facts ((?rs ring-spec)) TRUE
		(bind ?rs-doc (mongodb-fact-to-bson ?rs))
		(bson-array-append ?ring-spec-arr ?rs-doc)
		(bson-builder-destroy ?rs-doc)
	)
	(bson-array-finish ?doc "ring_specs" ?ring-spec-arr)

	(bind ?m-arr (bson-array-start))
	(do-for-all-facts ((?m machine)) TRUE
		(bind ?machine-doc (mongodb-fact-to-bson ?m (create$ name team mtype rotation pose zone down-period)))
		(if (eq ?m:mtype RS) then
	      (do-for-fact ((?rs-meta rs-meta)) (eq ?rs-meta:name ?m:name)
		    (mongodb-fact-to-bson-append ?machine-doc ?rs-meta (create$ available-colors))
		  )
		)
		(bson-array-append ?m-arr ?machine-doc)
		(bson-builder-destroy ?machine-doc)
	)
	(bson-array-finish ?doc "machines" ?m-arr)
	(bind ?doc (mongodb-update-game-report ?doc ?teams ?stime ?etime ?report-name))
	(mongodb-write-game-report ?doc ?stime ?report-name)
	(assert (mongodb-phase-change))
)

(defrule mongodb-start-new-report
" After restarting a game, a new report should be created"
	(declare (salience ?*PRIORITY_HIGH*))
	?t <- (mongodb-new-report)
	(game-parameters (is-parameterized TRUE))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME|SETUP) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
	=>
	(retract ?t)
	(delayed-do-for-all-facts ((?hist machine-history)) TRUE
	  (retract ?hist)
	)
	(mongodb-init-report ?teams ?stime ?etime ?report-name)
)

(defrule mongodb-game-report-begin
	(declare (salience ?*PRIORITY_HIGH*))
	?gp <- (game-parameters (is-parameterized TRUE))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME) (phase ~PRE_GAME) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
	(not (mongodb-game-report (start $?stime) (name ?report-name)))
	=>
	(mongodb-init-report ?teams ?stime ?etime ?report-name)
)

(defrule mongodb-start-new-report
" After restarting a game, a new report should be created"
	(declare (salience ?*PRIORITY_HIGH*))
	?t <- (mongodb-new-report)
	?gp <- (game-parameters (is-parameterized TRUE))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME|SETUP) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
	=>
	(retract ?t)
	(delayed-do-for-all-facts ((?hist machine-history)) TRUE
	  (retract ?hist)
	)
	(mongodb-init-report ?teams ?stime ?etime ?report-name)
)

(defrule mongodb-game-report-begin
	(declare (salience ?*PRIORITY_HIGHER*))
	?gp <- (game-parameters (is-parameterized TRUE))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (prev-phase PRE_GAME) (phase ~PRE_GAME) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
	(not (mongodb-game-report (start $?stime) (name ?report-name)))
	=>
	(mongodb-init-report ?teams ?stime ?etime ?report-name)
)

(defrule mongodb-game-report-end
	(declare (salience ?*PRIORITY_HIGHER*))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	  (phase POST_GAME) (start-time $?stime) (end-time $?etime))
	(confval (path "/llsfrb/game/store-to-report") (type STRING) (value ?report-name))
	?gr <- (mongodb-game-report (start $?stime) (name ?report-name) (end $?end&:(neq ?end ?etime)))
	=>
	(printout t "Writing game report to MongoDB" crlf)
	(modify ?gr (end ?etime))
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime ?report-name)
)

(defrule mongodb-game-report-new-phase-update
	(declare (salience ?*PRIORITY_HIGHER*))
	(time $?now)
	(gamestate (phase ?p) (state RUNNING)
	     (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime))
	?pc <- (mongodb-phase-change (registered-phases $?phases&:(not (member$ ?p ?phases))))
	?gr <- (mongodb-game-report (points $?gr-points) (name ?report-name))
	=>
	(modify ?pc (registered-phases (append$ ?phases ?p)))
	(modify ?gr (last-updated $?now))
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime ?report-name)
)


(defrule mongodb-game-report-update
	(declare (salience ?*PRIORITY_HIGHER*))
	(time $?now)
	(gamestate (state RUNNING)
	     (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime)
	     (points $?points))
	?gr <- (mongodb-game-report (points $?gr-points) (name ?report-name)
	     (last-updated $?last-updated&:(or
	       (neq $?points $?gr-points)
	       (timeout $?now $?last-updated ?*MONGODB-REPORT-UPDATE-FREQUENCY*))))
	=>
	(modify ?gr (points $?points) (last-updated $?now))
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime ?report-name)
)

(defrule mongodb-game-report-finalize
	(declare (salience ?*PRIORITY_HIGHER*))
	(gamestate (teams $?teams&:(neq ?teams (create$ "" "")))
	     (start-time $?stime) (end-time $?etime))
	?gr <- (mongodb-game-report (points $?gr-points) (name ?report-name))
	(finalize)
	=>
	(mongodb-write-game-report (mongodb-create-game-report ?teams ?stime ?etime ?report-name) ?stime ?report-name)
)

(defrule mongodb-net-client-connected
  (declare (salience ?*PRIORITY_HIGHER*))
  (protobuf-server-client-connected ?client-id ?host ?port)
  =>
  (bind ?client-doc (bson-create))
  (bson-append-time ?client-doc "session" ?*START-TIME*)
  (bson-append-time ?client-doc "connect_time" (now))
  (bson-append ?client-doc "client_id" ?client-id)
  (bson-append ?client-doc "host" ?host)
  (bson-append ?client-doc "port" ?port)
  (mongodb-insert "clients" ?client-doc)
  (bson-builder-destroy ?client-doc)
)

(defrule mongodb-net-client-disconnected
  (declare (salience ?*PRIORITY_HIGHER*))
  (protobuf-server-client-disconnected ?client-id)
  =>
  (bind ?client-update-doc (bson-create))
  (bson-append-time ?client-update-doc "disconnect_time" (now))

  (bind ?update-query (bson-create))
  (bson-append-time ?update-query "session" ?*START-TIME*)
  (bson-append ?update-query "client_id" ?client-id)

  (mongodb-update "clients" ?client-update-doc ?update-query)
  (bson-builder-destroy ?client-update-doc)
  (bson-builder-destroy ?update-query)
)


(defrule mongodb-restore-gamestate
	(declare (salience ?*PRIORITY_FIRST*))
	(time $?now)
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/restore-gamestate/enable") (type BOOL) (value TRUE))
	(confval (path "/llsfrb/game/restore-gamestate/phase") (type STRING) (value ?p))
	?gp <- (game-parameters (gamestate PENDING) (is-parameterized TRUE))
	=>
	(printout t "Loading gamestate from database" crlf)
	(if (mongodb-load-fact-from-game-report ?report-name
	                                         (sym-cat "gamestate/" ?p)
	                                         gamestate
	                                         (create$))
	 then
		(printout t "Loading gamestate finished" crlf)
		(modify ?gp (gamestate RECOVERED))
		(bind ?team-colors (create$ CYAN MAGENTA))
		(do-for-fact ((?g gamestate)) TRUE
			; ensure that time elapses from now on
			(modify ?g (last-time $?now))
			; setup the team peer
			(foreach ?team ?g:teams
				(if (neq ?team "")
				 then
					(assert (net-SetTeamName (nth$ ?team-index ?team-colors) ?team))
				)
			)
		)
	 else
		(printout error "Loading gamestate from database failed, fallback to fresh one." crlf)
		(modify ?gp (gamestate FRESH))
	)
)

(defrule mongodb-load-gamephase-points
	(declare (salience ?*PRIORITY_FIRST*))
	?gf <- (gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/restore-gamestate/enable") (type BOOL) (value TRUE))
	(confval (path "/llsfrb/game/restore-gamestate/phase") (type STRING) (value ?p))
	=>
	(bind ?success FALSE)
	(bind ?t-doc (mongodb-retrieve-report ?report-name))
	(if (neq ?t-doc FALSE) then
	 then
		(if (bind ?points-arr (bson-get-array ?t-doc "points"))
		 then
			(foreach ?point ?points-arr
				(assert (points (points (bson-get ?point "points")) (team (bson-get ?point "team")) (game-time (bson-get ?point "game_time")) (phase (bson-get ?point "phase")) (reason (bson-get ?point "reason"))))
				(bson-destroy ?point)
			)
		 else
			(printout error "Couldn't read points array" crlf)
		)
		(bson-destroy ?t-doc)
	 else
		(printout error "Empty result in mongoDB from game_report" crlf)
	)
)

(defrule mongodb-load-storage-status
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/default-storage") (type BOOL) (value FALSE))
	?gp <- (game-parameters (storage-status PENDING))
	=>
	(printout t "Loading storage from database" crlf)
	(if (mongodb-load-all-facts-from-game-report ?report-name
	                                         "machiness_shelf_slots"
	                                         machine-ss-shelf-slot
	                                         (create$ name position))
	 then
		(printout t "Loading storage status finished" crlf)
		(modify ?gp (storage-status STATIC))
	 else
		(printout error "Loading storage status from database failed, fallback to default assignment." crlf)
		(modify ?gp (storage-status DEFAULT))
	)
)

(defrule mongodb-load-orders
	(declare (salience ?*PRIORITY_FIRST*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	(confval (path "/llsfrb/game/random-orders") (type BOOL) (value FALSE))
	?gp <- (game-parameters (orders PENDING))
	=>
	(printout t "Loading orders from database" crlf)
	(if (mongodb-load-all-facts-from-game-report ?report-name
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
	(confval (path "/llsfrb/game/random-machine-setup") (type BOOL) (value FALSE))
	?gp <- (game-parameters (machine-setup PENDING))
	=>
	(printout t "Loading machine setup from database" crlf)
	(if (and (mongodb-load-all-facts-from-game-report ?report-name
	                                              "machines"
	                                              machine
	                                              name
	                                              (create$ down-period))
	         (mongodb-load-all-facts-from-game-report ?report-name
	                                              "ring_specs"
	                                              ring-spec
	                                              color)
	         ; the machine_meta array contains meta facts for all machines,
	         ; loading all facts to rs-meta facts would fail
	         (mongodb-load-some-facts-from-game-report ?report-name
	                                              "machines"
	                                              rs-meta
	                                              name
	                                              (create$ available-colors)))
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
	(not (confval (path "/llsfrb/game/random-field") (type BOOL) (value TRUE)))
	(confval (path "/llsfrb/game/load-from-report") (type STRING) (value ?report-name))
	?gp <- (game-parameters (machine-positions PENDING))
	?mg <- (machine-generation (state NOT-STARTED))
	=>
	(modify ?mg (state FINISHED))
	(printout t "Loading machine positions from database" crlf)
	(if (mongodb-load-all-facts-from-game-report ?report-name
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

