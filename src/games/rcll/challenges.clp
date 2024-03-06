
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
	?*FIELD-WIDTH*  = (config-get-int "/llsfrb/challenges/field/width")
	?*FIELD-HEIGHT* = (config-get-int "/llsfrb/challenges/field/height")
	?*VISIT-ZONE-DURATION* = (config-get-int "/llsfrb/challenges/publish-routes/pause-duration")
)
(deftemplate challenges-field
	(multislot free-zones (type SYMBOL))
	(multislot occupied-zones (type SYMBOL))
)

(deftemplate challenges-zone-visit
	(slot robot-number (type INTEGER))
	(slot team-color (type SYMBOL) (allowed-values CYAN MAGENTA))
	(slot visited-zone (type SYMBOL))
	(slot route-id (type INTEGER))
	(slot visit-start (type FLOAT))
)

(deftemplate challenges-route
	(slot id (type INTEGER))
	(slot team-color (type SYMBOL) (allowed-values CYAN MAGENTA) (default CYAN))
	(multislot way-points (type SYMBOL))
	(multislot reached (type SYMBOL) (default (create$)))
	(multislot remaining (type SYMBOL) (default (create$)))
)

(defrule challenges-print-essential-info
	(declare (salience (- ?*PRIORITY_HIGH* 1)))
	(finalize)
	(confval (path "/llsfrb/challenges/publish-routes/enable") (type BOOL) (value ?routes-enabled))
	(confval (path "/llsfrb/challenges/publish-routes/num-points") (type UINT) (value ?points))
	(confval (path "/llsfrb/challenges/publish-routes/num-routes") (type UINT) (value ?routes))
	(not (challenges-info-printed))
	=>
	(print-sep (str-cat " Challenge routes with " ?points " waypoints and " ?*VISIT-ZONE-DURATION* "s of stay"))
	; print relevant machine
	(bind ?routes (find-all-facts ((?m challenges-route)) TRUE))
	(bind ?routes (sort id> ?routes))
	(print-fact-list (fact-indices ?routes) (create$))

	(print-sep "Challenge configuration ")
	(bind ?challenge-cfg (find-all-facts ((?confval confval)) (str-index "/llsfrb/challenges/" ?confval:path)))
	(print-fact-list (fact-indices ?challenge-cfg) (create$ path value list-value))
	(assert (challenges-info-printed))
)


(defrule challenges-init
	(not (challenges-initalized))
=>
	(assert (signal (type navigation-routes-bc) (time (create$ 0 0)) (seq 1)))
	(assert (challenges-initiaized))
)

(deffunction challenges-init-field (?width ?height ?mirror)
	(bind ?free (create$))
	; all possible fields
	(loop-for-count (?x 1 ?width)
		(loop-for-count (?y 1 ?height)
			(bind ?free (append$ ?free (sym-cat M_Z ?x ?y)))
			(if ?mirror then
				(bind ?free (append$ ?free (sym-cat C_Z ?x ?y)))
			)
		)
	)
	(bind ?occupied (create$))
	(loop-for-count (?x (- ?width 2) ?width)
			(bind ?occupied (append$ ?occupied (sym-cat M_Z ?x 1)))
			(if ?mirror then
				(bind ?occupied (append$ ?occupied (sym-cat C_Z ?x 1)))
			)
	)
	(do-for-all-facts ((?m machine)) TRUE
		(if (neq ?m:zone TBD) then
			(bind ?occupied (append$ ?occupied ?m:zone))
		)
	)
	; remove occupied zones
	(foreach ?zone ?occupied
		(bind ?pos (member$ ?zone ?free))
		(if ?pos then
			(bind ?free (delete$ ?free ?pos ?pos))
		)
	)
	(assert (challenges-field (free-zones ?free) (occupied-zones ?occupied)))
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

(defrule challenges-create-routes
" Create some routes with waypoints in the field as a navigation challenge "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/publish-routes/enable") (type BOOL) (value TRUE))
	(confval (path "/llsfrb/challenges/publish-routes/num-points") (type UINT) (value ?points))
	(confval (path "/llsfrb/challenges/publish-routes/num-routes") (type UINT) (value ?routes))
	(challenges-field (free-zones $?free) (occupied-zones $?occupied))
	(not (challenges-route))
=>
	(bind ?mirror (config-get-bool "/llsfrb/challenges/field/mirror"))
	(loop-for-count (?r 1 ?routes)
		(bind ?route-candidates (randomize$ ?free) 1 ?points)
		(bind ?route (create$))
		(bind ?route-mirror (create$))
		(loop-for-count (?r 1 ?points)
			(bind ?zone (nth$ 1 ?route-candidates))
			(bind ?route-candidates (delete$ ?route-candidates 1 1))
			(if ?mirror then
				(bind ?mirror-zone (member$ (mirror-zone ?zone) ?route-candidates))
				(if ?mirror-zone then
					(bind ?route-candidates (delete$ ?route-candidates ?mirror-zone ?mirror-zone))
					(bind ?route-mirror (append$ ?route-mirror (mirror-zone ?zone)))
				)
				 else
					(bind ?route-mirror (append$ ?route ?zone))
			)
			(bind ?route (append$ ?route ?zone))
		)
		(assert (challenges-route (id ?r) (way-points ?route) (remaining ?route)
		        (team-color MAGENTA)))
		(printout t "Route MAGENTA created" crlf)
		(assert (challenges-route (id ?r) (way-points ?route-mirror)
		        (remaining ?route-mirror) (team-color CYAN)))
		(printout t "Route CYAN created" crlf)
	)
)

(defrule challenges-configure-machine-ground-truth
" Send the ground truth for machine positions only for specified phases "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/send-mps-ground-truth") (is-list TRUE) (list-value $?gt))
	?s <- (send-mps-positions (phases $?old-phases&:(neq ?old-phases (type-cast-list ?gt SYMBOL))))
=>
	(modify ?s (phases (type-cast-list ?gt SYMBOL)))
)

(defrule challenges-customize-orders
" Delete all existing orders and then load orders from configuration "
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(confval (path "/llsfrb/challenges/orders/customize") (type BOOL) (value TRUE))
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
	(game-parameters (is-parameterized FALSE) (machine-setup RANDOM))
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
			(do-for-fact ((?rs-meta rs-meta)) (eq ?rs-meta:name ?m:name)
				(modify ?rs-meta (available-colors ?ring-colors))
			)
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

(defrule challenges-reset-flush-reset-flag
	?c <- (challanges-reset-back-in-setup)
	(gamestate (phase ~SETUP))
	=>
	(retract ?c)
)

(defrule challenges-reset-field-back-to-setup
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(gamestate (phase SETUP) (prev-phase PRODUCTION|POST_GAME|EXPLORATION))
	(not (challanges-reset-back-in-setup))
	=>
	(delayed-do-for-all-facts ((?machine machine)) TRUE
	  (if (eq ?machine:mtype RS) then (mps-reset-base-counter (str-cat ?machine:name)))
	  (modify ?machine (productions 0) (state IDLE) (operation-mode RETRIEVE_CAP)
	             (current-operation STORE)
	             (proc-start 0.0))
	)
	(delayed-do-for-all-facts ((?ml machine-lights)) TRUE
	  (modify ?ml (desired-lights GREEN-ON YELLOW-ON RED-ON))
	)
	(delayed-do-for-all-facts ((?r challenges-route)) (retract ?r))
	(assert (challanges-reset-back-in-setup))
)

(defrule challenges-parameterize
	(declare (salience ?*PRIORITY_CHALLENGE_OVERRIDE*))
	(gamestate (phase SETUP|EXPLORATION|PRODUCTION) (prev-phase PRE_GAME))
	?gp <- (game-parameters (machine-positions ?m-positions&~PENDING)
	                        (machine-setup ?m-setup&~PENDING)
	                        (orders ?orders&~PENDING)
	                        (storage-status ?s-status&~PENDING)
	                        (is-parameterized FALSE))
	(confval (path "/llsfrb/challenges/field/width") (type UINT) (value ?width))
	(confval (path "/llsfrb/challenges/field/height") (type UINT) (value ?height))
	(machine-generation (state ?s&:(or (eq ?s FINISHED) (eq ?m-positions STATIC))))
	=>
	(bind ?mirror (config-get-bool "/llsfrb/challenges/field/mirror"))

	; reset machines
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
	(delayed-do-for-all-facts ((?machine machine)) TRUE
	(if (eq ?machine:mtype RS) then (mps-reset-base-counter (str-cat ?machine:name)))
		(modify ?machine (state IDLE))
	)
	(delayed-do-for-all-facts ((?ml machine-lights)) TRUE
	  (modify ?ml (desired-lights GREEN-ON YELLOW-ON RED-ON))
	)
	(if (eq ?m-positions RANDOM)
	 then
		(machine-retrieve-generated-mps ?mirror)
		(challenges-init-field ?width ?height ?mirror)
	 else
		(challenges-init-field ?width ?height ?mirror)
		(delayed-do-for-all-facts ((?m machine)) (eq ?m:zone TBD)
			(printout t ?m:name " not set in mongodb log, skipping" crlf)
			(retract ?m)
		)
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
	(modify ?gp (is-parameterized TRUE))
)

(deffunction challenges-net-create-broadcast-NavigationRoutes (?team-color)
	(bind ?s (pb-create "llsf_msgs.NavigationRoutes"))
	(pb-set-field ?s "team_color" ?team-color)
	(do-for-all-facts ((?route challenges-route)) (eq ?route:team-color ?team-color)
		(bind ?m (pb-create "llsf_msgs.Route"))
		(pb-set-field ?m "id" (fact-slot-value ?route id))
		(foreach ?z (fact-slot-value ?route way-points)
			(pb-add-list ?m "route" ?z)
		)
		(foreach ?z (fact-slot-value ?route reached)
			(pb-add-list ?m "reached" ?z)
		)
		(foreach ?z (fact-slot-value ?route remaining)
			(pb-add-list ?m "remaining" ?z)
		)
		(pb-add-list ?s "routes" ?m)
	)
	(return ?s)
)

(defrule challenges-net-send-navigation-messages
	(time $?now)
	(gamestate (phase PRODUCTION))
	(confval (path "/llsfrb/challenges/publish-routes/enable") (type BOOL) (value TRUE))
	?sf <- (signal (type navigation-routes-bc) (seq ?seq) (count ?count)
	  (time $?t&:(timeout ?now ?t
	    (if (> ?count ?*BC-MACHINE-INFO-BURST-COUNT*)
	      then ?*BC-MACHINE-INFO-PERIOD*
	      else ?*BC-MACHINE-INFO-BURST-PERIOD*))))
	(network-peer (group CYAN) (id ?peer-id-cyan))
	(network-peer (group MAGENTA) (id ?peer-id-magenta))
=>
	(modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

	(bind ?s (challenges-net-create-broadcast-NavigationRoutes CYAN))
	(pb-broadcast ?peer-id-cyan ?s)
	(pb-destroy ?s)

	(bind ?s (challenges-net-create-broadcast-NavigationRoutes MAGENTA))
	(pb-broadcast ?peer-id-magenta ?s)
	(pb-destroy ?s)
)

(defrule challenges-zone-visit-start
	(robot (number ?n) (team-color ?col) (pose ?x ?y ?z) (state ACTIVE))
	(challenges-route (id ?r-id) (team-color ?col) (remaining $?zones&:(member$ (pose-to-zone ?x ?y) ?zones)))
	(not (challenges-zone-visit (robot-number ?n) (team-color ?col)))
	(not (challenges-zone-visit (team-color ?col) (route-id ?r-id)))
	(gamestate (game-time ?gt) (phase PRODUCTION) (state RUNNING))
=>
	(assert (challenges-zone-visit (route-id ?r-id) (team-color ?col)
	(robot-number ?n) (visited-zone (pose-to-zone ?x ?y)) (visit-start ?gt)))
)
(defrule challenges-zone-visit-abort
	(robot (number ?n) (team-color ?col) (pose ?x ?y ?z) (state ACTIVE))
	?zv <- (challenges-zone-visit (robot-number ?n) (team-color ?col)
	         (visited-zone ?zone&:(neq ?zone (pose-to-zone ?x ?y))))
	(gamestate (game-time ?gt) (phase PRODUCTION) (state RUNNING))
=>
	(retract ?zv)
)

(defrule challenges-zone-visit-success
	(gamestate (game-time ?gt) (phase PRODUCTION) (state RUNNING))
	?route <- (challenges-route (team-color ?col) (id ?r-id)
	  (remaining $?remaining) (reached $?reached))
	?zv <- (challenges-zone-visit (team-color ?col) (route-id ?r-id)
	  (visit-start ?start&:(> (- ?gt ?start) ?*VISIT-ZONE-DURATION*))
	  (visited-zone ?zone))

=>
	(retract ?zv)
	(bind ?z-index (member$ ?zone ?remaining))
	(if ?z-index then
		(modify ?route (remaining (delete$ ?remaining ?z-index ?z-index))
		               (reached (append$ ?reached ?zone)))
		(printout t "Visited zone " ?zone " of route " ?r-id " team " ?col crlf)
	 else
		(printout error "Visited zone " ?zone " of route " ?r-id " team " ?col
		                "that was not marked as 'remaining', ignoring" crlf)
	)
)

(defrule challenges-sync-config-with-global-field-width
  ?cv <- (confval (path "/llsfrb/challenges/field/width") (value ?v&:(neq  ?v ?*FIELD-WIDTH*)))
  =>
  (modify ?cv (value ?*FIELD-WIDTH*))
)

(defrule challenges-sync-config-with-global-field-height
  ?cv <- (confval (path "/llsfrb/challenges/field/height") (value ?v&:(neq  ?v ?*FIELD-HEIGHT*)))
  =>
  (modify ?cv (value ?*FIELD-HEIGHT*))
)

(defrule challenges-sync-config-with-global-field-mirrored
  ?cv <- (confval (path "/llsfrb/challenges/field/mirrored") (value ?v&:(neq  ?v ?*FIELD-MIRRORED*)))
  =>
  (modify ?cv (value ?*FIELD-MIRRORED*))
)

(defrule challenges-add-pb-conf-field-width
  (confval (path ?p&"/llsfrb/challenges/field/width") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_width") (value ?v))
  )
)

(defrule challenges-add-pb-conf-field-height
  (confval (path ?p&"/llsfrb/challenges/field/height") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_height") (value ?v))
  )
)

(defrule challenges-add-pb-conf-field-mirrored
  (confval (path ?p&"/llsfrb/challenges/field/mirrored") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_mirrored") (value ?v))
  )
)

(defrule challenges-add-pb-conf-challenge-name
  (confval (path ?p&"/llsfrb/challenges/name") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "challenge_name") (value ?v))
  )
)
