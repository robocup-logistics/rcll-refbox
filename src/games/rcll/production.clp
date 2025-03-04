
; Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

;---------------------------------------------------------------------------
;  production.clp - LLSF RefBox CLIPS production phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;             2019       Mostafa Gomaa
;             2020       Tarik Viehmann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

; Functions to process instructions for the different machine types.
; @param ?p: Pointer to PrepareMachine message
; @param ?m: machine fact index
; @param ?mname: name of the machine corresponding to ?m
; @param ?gt: current game tim
(deffunction production-prepare-bs (?p ?m ?meta ?mname ?gt)
	(if (pb-has-field ?p "instruction_bs")
	 then
		(bind ?prepmsg (pb-field-value ?p "instruction_bs"))
		(bind ?side (sym-cat (pb-field-value ?prepmsg "side")))
		(bind ?color (sym-cat (pb-field-value ?prepmsg "color")))
		(printout t "Prepared " ?mname " (side: " ?side ", color: " ?color ")" crlf)
		(modify ?meta (current-side ?side) (current-base-color ?color))
		(modify ?m (state PREPARED) (proc-start ?gt))
	 else
		(modify ?m (state BROKEN)
		(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	)
)

(deffunction production-prepare-ds (?p ?m ?meta ?mname ?gt)
	(if (pb-has-field ?p "instruction_ds")
	 then
		(bind ?prepmsg (pb-field-value ?p "instruction_ds"))
		(bind ?order-id (pb-field-value ?prepmsg "order_id"))
		(if (not (do-for-fact ((?order order)) (eq ?order:id ?order-id)
			(printout t "Prepared " ?mname " (order: " ?order-id ")" crlf)
			(modify ?meta (order-id ?order-id) (gate ?order:delivery-gate))
			(modify ?m (state PREPARED) (proc-start ?gt))
			))
		 then
			(if (eq (pb-field-value ?prepmsg "order_id") 0) then
				(printout t "Prepared " ?mname " to consume product not belonging to any order (id 0)" crlf)
				(modify ?meta (order-id ?order-id) (gate 1))
				(modify ?m (state PREPARED) (proc-start ?gt))
			 else
				(modify ?m (state BROKEN)
				  (broken-reason (str-cat "Prepare received for " ?mname " with invalid order ID")))
			)
		 else
			(return TRUE)
		)
	 else
		(modify ?m (state BROKEN)
		           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
	)
)

(deffunction production-prepare-ss (?p ?m ?meta ?mname ?gt)
	(if (pb-has-field ?p "instruction_ss")
	 then
		(bind ?prepmsg (pb-field-value ?p "instruction_ss"))
		(bind ?operation (sym-cat (pb-field-value ?prepmsg "operation")))
		(bind ?shelf (integer (pb-field-value ?prepmsg "shelf")))
		(bind ?slot (integer (pb-field-value ?prepmsg "slot")))
		(bind ?filled-status FALSE)
		(bind ?description-provided (pb-has-field ?prepmsg "wp_description"))
		(do-for-fact  ((?ss-f machine-ss-shelf-slot))
		                      (and (eq ?ss-f:position (create$ ?shelf ?slot))
		                           (eq ?ss-f:name (fact-slot-value ?m name)))
		                      (bind ?filled-status ?ss-f))
		(if ?filled-status
		 then
			(bind ?description (fact-slot-value ?filled-status description))
			(bind ?new-description ?description)
			(if ?description-provided
				 then
					(bind ?new-description (pb-field-value ?prepmsg "wp_description"))
			)
			(if (eq ?operation RETRIEVE)
			 then
				(if (fact-slot-value ?filled-status is-filled)
				 then
					(printout t "Prepared " ?mname " to RETRIEVE (" ?shelf "," ?slot ")" crlf)
					(modify ?m (state PREPARED) (proc-start ?gt))
					(modify ?meta (current-operation ?operation)
					              (current-shelf-slot ?shelf ?slot)
					              (current-wp-description ?description))
				 else
					(modify ?m (state BROKEN)
					           (broken-reason (str-cat "Prepare received for " ?mname
					                           ", but station is empty (" ?shelf ", " ?slot ")")))
				)
			 else
				(if (eq ?operation STORE)
				 then
					(if (fact-slot-value ?filled-status is-filled)
					 then
						(modify ?m (state BROKEN)
						           (broken-reason (str-cat "Prepare received for " ?mname
						            " with STORE operation for occupied shelf slot (" ?shelf ", " ?slot ")")))
					 else
						(printout t "Prepared " ?mname " to STORE (" ?shelf "," ?slot ")" crlf)
						(modify ?m (state PREPARED) (proc-start ?gt))
						(modify ?meta (current-operation ?operation)
						              (current-shelf-slot ?shelf ?slot)
						              (current-wp-description ?new-description))
					)
				)
				(if (eq ?operation CHANGE_INFO)
				 then
					(if ?description-provided
					 then
					 (modify ?filled-status (description ?new-description))
					 else
					(printout t "Change Description for " ?mname " at (" ?shelf "," ?slot
					            ") but no description provided, ignoring.")
					)
				)
			)
		 else
			(modify ?m (state BROKEN)
			           (broken-reason (str-cat "Prepare received for " ?mname
			            " with invalid shelf or slot (" ?shelf " [0, " ?*SS-MAX-SHELF*
			            "], " ?slot " [0, " ?*SS-MAX-SLOT*"])" )))
		)
	 else
		(modify ?m (state BROKEN)
		           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
	)
)

(deffunction production-prepare-rs (?p ?m ?meta ?mname ?gt)
	(if (pb-has-field ?p "instruction_rs")
	 then
		(bind ?prepmsg (pb-field-value ?p "instruction_rs"))
		(bind ?ring-color (sym-cat (pb-field-value ?prepmsg "ring_color")))
		(if (member$ ?ring-color (fact-slot-value ?meta available-colors))
		 then
			(printout t "Prepared " ?mname " (ring color: " ?ring-color ")" crlf)
			(modify ?m (state PREPARED) (proc-start ?gt))
			(modify ?meta (current-ring-color ?ring-color))
		 else
			(modify ?m (state BROKEN)
			           (broken-reason (str-cat "Prepare received for " ?mname " for invalid ring color (" ?ring-color ")")))
		)
	 else
		(modify ?m (state BROKEN)
		(broken-reason (str-cat "Prepare received for " ?mname " without data")))
	)
)

(deffunction production-prepare-cs (?p ?m ?meta ?mname ?gt)
	(if (pb-has-field ?p "instruction_cs")
	 then
		(bind ?prepmsg (pb-field-value ?p "instruction_cs"))
		(bind ?cs-op (sym-cat (pb-field-value ?prepmsg "operation")))
		(switch ?cs-op

		 (case RETRIEVE_CAP then
			(if (not (fact-slot-value ?meta has-retrieved))
			 then
				(printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
				(modify ?meta (operation-mode ?cs-op))
				(modify ?m (state PREPARED) (proc-start ?gt))
			 else
				(modify ?m (state BROKEN)
				           (broken-reason (str-cat "Prepare received for " ?mname ": "
				                                   "cannot retrieve while already holding")))
		 ))
		 (case MOUNT_CAP then
			(if (fact-slot-value ?meta has-retrieved)
			 then
				(printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
				(modify ?meta (operation-mode ?cs-op))
				(modify ?m (state PREPARED) (proc-start ?gt))
			 else
				(modify ?m (state BROKEN)
				           (broken-reason (str-cat "Prepare received for " ?mname
				                                   ": cannot mount without cap")))
		)))
	 else
		(modify ?m (state BROKEN)
		           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
	)
)

(defrule production-start
  ?gs <- (gamestate (phase PRODUCTION) (prev-phase ~PRODUCTION))
  ?ti <- (time-info)
  =>
  (modify ?gs (prev-phase PRODUCTION))
  (modify ?ti (game-time 0.0))

  ; trigger machine info burst period
  (do-for-fact ((?sf signal)) (eq ?sf:type machine-info-bc)
    (modify ?sf (count 1) (time 0 0))
  )
  ;(assert (attention-message (text "Entering Production Phase")))
)

(defrule production-machine-down
  (gamestate (phase PRODUCTION) (state RUNNING))
  (time-info (game-time ?gt))
  ?mf <- (machine (name ?name) (mtype ?mtype)
                  (state ?state&~DOWN) (proc-start ?proc-start)
                  (wait-for-product-since ?wait-since)
                  (down-period $?dp&:(<= (nth$ 1 ?dp) ?gt)&:(>= (nth$ 2 ?dp) ?gt)))
  =>
  (bind ?down-time (- (nth$ 2 ?dp) (nth$ 1 ?dp)))
  (printout t "Machine " ?name " down for " ?down-time " sec" crlf)
  (modify ?mf (state DOWN) (prev-state ?state)
              (proc-start (+ ?proc-start ?down-time))
              (wait-for-product-since (+ ?wait-since ?down-time)))
  (assert (send-machine-update))
)

(defrule production-machine-up
  (gamestate (phase PRODUCTION) (state RUNNING))
  (time-info (game-time ?gt))
  ?mf <- (machine (name ?name) (state DOWN) (prev-state ?prev-state&~DOWN)
		  (down-period $?dp&:(< (nth$ 2 ?dp) ?gt)))
  =>
	(printout t "Machine " ?name " is up again" crlf)
	(modify ?mf (state ?prev-state))
  (assert (send-machine-update))
)

(defrule production-machine-prepare
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?pf <- (protobuf-msg (type "llsf_msgs.PrepareMachine") (ptr ?p)
	       (rcvd-from ?from-host ?from-port) (client-type ?ct) (client-id ?cid))
	(network-peer (id ?cid) (group ?group))
	(time $?now)
	=>
	(retract ?pf)
	(bind ?mname (sym-cat (pb-field-value ?p "machine")))
	(bind ?team (sym-cat (pb-field-value ?p "team_color")))
	(if (and (eq ?ct PEER) (neq ?team ?group))
	 then
		; message received for a team over the wrong channel, deny
		(assert (attention-message (team ?group)
		        (text (str-cat "Invalid prepare for team " ?team " of team " ?group))))
			(printout warn Invalid prepare for team " ?team " of team " ?group" crlf)
	 else
		(if (not (any-factp ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))))
		 then
			(printout warn "Prepare received for invalid machine " ?mname " of team " ?team crlf)
			(assert (attention-message (team ?team)
			        (text (str-cat "Prepare received for invalid machine " ?mname))))
		 else
		 	(printout t "Received prepare for " ?mname crlf)
			(if (and (pb-has-field ?p "sent_at")
			         (timeout ?now (time-from-sec (pb-field-value ?p "sent_at")) ?*PRODUCTION-PREPARE-TIMEOUT*))
			 then
				(printout t "Prepare message received that is older than " ?*PRODUCTION-PREPARE-TIMEOUT* " sec (" (time-diff-sec ?now (time-from-sec (pb-field-value ?p "sent_at"))) ") ignoring it" crlf)
			 else
				(if (not (any-factp ((?exp exploration-report)) (and (eq ?exp:name ?mname)
				                                                     (eq ?exp:rtype RECORD)
				                                                     (eq ?exp:correctly-reported TRUE))))
				 then
						(assert (attention-message (team ?team)
						        (text (str-cat "Prepare received for machine that was not correctly reported yet: " ?mname))))
						(return)
				)
				(do-for-fact ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))
					(if (eq ?m:state IDLE) then
						(printout t ?mname " is IDLE, processing prepare" crlf)
						(bind ?success FALSE)
						(switch ?m:mtype
						 (case BS then (bind ?success
							(do-for-fact ((?f bs-meta)) (eq ?f:name ?mname)
								(production-prepare-bs ?p ?m ?f ?mname ?gt))))
						 (case DS then (bind ?success
							(do-for-fact ((?f ds-meta)) (eq ?f:name ?mname)
								(production-prepare-ds ?p ?m ?f ?mname ?gt))))
						 (case SS then (bind ?success
							(do-for-fact ((?f ss-meta)) (eq ?f:name ?mname)
								(production-prepare-ss ?p ?m ?f ?mname ?gt))))
						 (case RS then (bind ?success
							(do-for-fact ((?f rs-meta)) (eq ?f:name ?mname)
								(production-prepare-rs ?p ?m ?f ?mname ?gt))))
						 (case CS then (bind ?success
							(do-for-fact ((?f cs-meta)) (eq ?f:name ?mname)
								(production-prepare-cs ?p ?m ?f ?mname ?gt))))
						)
						(if (not ?success) then
							(assert (attention-message (team ?team)
							        (text (str-cat "Prepare received for machine that has no meta information: " ?mname))))
						)
					 else
						(printout debug "Ignoring prepare for " ?mname " (should be IDLE, is " ?m:state")" crlf)
					)
				)
			)
		)
	)
  (assert (send-machine-update))
)

(defrule production-machine-reset-by-team
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.ResetMachine") (ptr ?p)
		     (rcvd-from ?from-host ?from-port) (client-type ?ct) (client-id ?cid))
  (network-peer (id ?cid) (group ?group))
  =>
  (retract ?pf)
  (bind ?mname (sym-cat (pb-field-value ?p "machine")))
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (if (and (eq ?ct PEER) (neq ?team ?group))
   then
    ; message received for a team over the wrong channel, deny
    (assert (attention-message (team ?group)
	      (text (str-cat "Invalid reset for team " ?team " of team " ?group))))
   else
    (if (not (any-factp ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))))
     then
      (assert (attention-message (team ?team)
		(text (str-cat "Reset received for invalid machine " ?mname))))
     else
      (printout t "Received reset for " ?mname crlf)
      (do-for-fact ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))
	(modify ?m (state BROKEN)
                   (broken-reason (str-cat "Machine " ?mname " resetted by the team " ?team)))
      )
    )
  )
  (assert (send-machine-update))
)


; **** MPS status feedback processing
(defrule production-mps-feedback-state-ready
	?m <- (machine (name ?n))
	?mps-status <- (mps-status-feedback ?n READY ?ready)
	=>
	(retract ?mps-status)
	(modify ?m (mps-ready ?ready))
)

(defrule production-mps-feedback-state-busy
	?m <- (machine (name ?n))
	?mps-status <- (mps-status-feedback ?n BUSY ?busy)
	=>
	(retract ?mps-status)
	(modify ?m (mps-busy ?busy))
)

(defrule production-mps-feedback-rs-new-base-on-slide
	"Process a SLIDE-COUNTER event sent by the PLC. Do not directly increase the
	 counter but assert a transient mps-add-base-on-slide fact instead."
	?m <- (rs-meta (name ?n) (slide-counter ?mps-counter))
	?fb <- (mps-status-feedback ?n SLIDE-COUNTER ?new-counter&:(> ?new-counter ?mps-counter))
	=>
	(retract ?fb)
	(modify ?m (slide-counter ?new-counter))
	(assert (mps-add-base-on-slide ?n))
)

(defrule production-mps-feedback-barcode
	"Process a BARCODE event sent by the PLC. Do not directly update the
     workpiece but assert a transient mps-read-barcode fact instead."
	?m <- (machine (name ?n))
	?fb <- (mps-status-feedback ?n BARCODE ?barcode)
	=>
	(retract ?fb)
	(assert (mps-read-barcode ?n ?barcode))
)

(defrule production-bs-dispense
  "Start dispensing a base from a prepared BS"
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS) (state PREPARED))
	(bs-meta (name ?n) (current-base-color ?color))
  =>
  (printout t "Machine " ?n " dispensing " ?color " base" crlf)
  (modify ?m (state PROCESSING) (proc-start ?gt) (task DISPENSE) (mps-busy WAIT))
  (mps-bs-dispense (str-cat ?n) (str-cat ?color))
  (assert (send-machine-update))
)

(defrule production-bs-move-conveyor
  "The BS has dispensed a base. We now need to move the conveyor"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype BS) (state PROCESSING) (task DISPENSE)
	               (mps-busy FALSE) (team ?team))
	(bs-meta (name ?n) (current-side ?side) (current-base-color ?current-base-color))
	=>
	(printout t "Machine " ?n " moving base to " ?side crlf)
	(assert (product-processed (at-machine ?n) (mtype BS) (team ?team)
	                           (game-time ?gt) (base-color ?current-base-color)))
	(modify ?m (task MOVE-OUT) (proc-start ?gt) (state PROCESSED) (mps-busy WAIT))
	(if (eq ?side INPUT)
	 then
		(mps-move-conveyor (str-cat ?n) "INPUT" "BACKWARD")
	 else
		(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
	)
)

(defrule production-rs-insufficient-bases
  "The RS has been prepared but it does not have sufficient additional material; switch to BROKEN."
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED))
  (rs-meta (name ?n) (current-ring-color ?ring-color) (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color)
	     (req-bases ?req-bases&:(> ?req-bases (- ?ba ?bu))))
	(confval (path ?p&:(eq ?p (str-cat"/llsfrb/mps/stations/" ?n "/connection")))
	         (type STRING) (is-list FALSE) (value ~"mockup"))
  (not (referee-for-missing-payment ?n ?ring-color ?ba ?bu))
  =>
  (assert (referee-for-missing-payment ?n ?ring-color ?ba ?bu))
  ; wait for referee to confirm broken machine
  (modify ?m (referee-required TRUE)
    (broken-reason (str-cat ?n ": insufficient bases ("(- ?ba ?bu) " < " ?req-bases ")")))
)

(defrule production-rs-cleanup-referee-for-missing-payment
  (rs-meta (name ?n) (current-ring-color ?ring-color) (bases-added ?ba) (bases-used ?bu))
  ?r-f <- (referee-for-missing-payment ?n ?ring-color ?ba2 ?bu2)
  (test (or (neq ?ba2 ?ba) (neq ?bu2 ?bu)))
  =>
  (retract ?r-f)
)

(defrule production-rs-ignore-insufficient-bases
  "The RS has been prepared but it does not have sufficient additional material.
   However, slide payment checks are disabled so is assumed that rings were
   payed beforehand, but no mps feedback was received. Hence, simulate the
   feedback once."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (machine (name ?n) (mtype RS) (state PREPARED))
  (rs-meta (name ?n) (slide-counter ?mps-counter)
           (current-ring-color ?ring-color) (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color)
             (req-bases ?req-bases&:(> ?req-bases (- ?ba ?bu))))
  (not (mps-status-feedback ?n SLIDE-COUNTER ?))
	(confval (path ?p&:(eq ?p (str-cat"/llsfrb/mps/stations/" ?n "/connection")))
	         (type STRING) (is-list FALSE) (value "mockup"))
	(not (mps-add-base-on-slide ?n))
  =>
  (printout warn "Simulating "(str-cat ?n) " base payment feedback. "
									"Please verify that the payment was actually made" crlf)
  (assert (mps-status-feedback ?n SLIDE-COUNTER (+ 1 ?mps-counter)))]
)

(defrule production-rs-move-to-mid
  "The RS is PREPARED. Start moving the workpiece to the middle to mount a ring."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED) (task nil))
  (rs-meta (name ?n) (current-ring-color ?ring-color) (available-colors $?ring-colors)
           (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color) (req-bases ?req-bases&:(>= (- ?ba ?bu) ?req-bases)))
  =>
  (printout t "Machine " ?n " of type RS switching to PREPARED state" crlf)
  (modify ?m (task MOVE-MID) (mps-busy WAIT))
  (mps-move-conveyor (str-cat ?n) "MIDDLE" "FORWARD")
)

(defrule production-rs-mount-ring
  "Workpiece is in the middle of the conveyor belt of the RS, mount a ring."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype RS) (state PREPARED) (task MOVE-MID) (mps-busy FALSE))
	?meta <- (rs-meta (name ?n) (current-ring-color ?ring-color)
	                  (available-colors $?ring-colors) (bases-used ?bu))
  (ring-spec (color ?ring-color) (req-bases ?req-bases))
	=>
	(printout t "Machine " ?n ": mount ring" crlf)
	(modify ?m (state PROCESSING) (proc-start ?gt) (task MOUNT-RING) (mps-busy WAIT) (broken-reason ""))
	(modify ?meta (bases-used (+ ?bu ?req-bases)))
  (mps-rs-mount-ring (str-cat ?n) (member$ ?ring-color ?ring-colors) (str-cat ?ring-color))
  (assert (send-machine-update))
)

(defrule production-rs-move-to-output
	"Ring is mounted, move to output"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype RS) (state PROCESSING)
	               (task MOUNT-RING) (mps-busy FALSE) (team ?team))
	(rs-meta (name ?n) (current-ring-color ?ring-color))
	=>
	(printout t "Machine " ?n ": move to output" crlf)
	(assert (product-processed (at-machine ?n) (mtype RS) (team ?team)
		                       (game-time ?gt) (ring-color ?ring-color)))
	(modify ?m (state PROCESSED) (proc-start ?gt) (task MOVE-OUT) (mps-busy WAIT))
	(assert (send-machine-update))
	(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
)

(defrule production-rs-ignore-slide-counter-in-non-production
	"We are not in production phase, ignore slide events on the RS"
	(gamestate (phase ~PRODUCTION))
	?fb <- (mps-add-base-on-slide ?n)
	=>
	(retract ?fb)
)

(defrule production-rs-process-base-on-slide
	"Process the transient mps-add-base-on-slide, which is either created by a
	 SLIDE-COUNTER event or by receiving a protobuf message. Actually increment
	 the counter of the machine."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (state ?state) (mtype RS) (team ?team))
	?meta <- (rs-meta (name ?n) (bases-used ?bases-used)
	                  (bases-added ?old-num-bases))
	?fb <- (mps-add-base-on-slide ?n)
	(not (points (game-time ?gt)))
	=>
	(retract ?fb)
	(if (neq ?state BROKEN)
	 then
		(bind ?num-bases (+ 1 ?old-num-bases))
		(printout t "Machine " ?n " base added (count: " ?num-bases ")" crlf)
		(if (<= (- ?num-bases ?bases-used) ?*LOADED-WITH-MAX*)
		 then
			(assert (points (game-time ?gt) (points ?*PRODUCTION-POINTS-ADDITIONAL-BASE*)
			                (team ?team) (phase PRODUCTION)
			                (reason (str-cat "Added additional base to " ?n))))
			(modify ?meta (bases-added ?num-bases))
		 else
			(modify ?m (state BROKEN)
			           (broken-reason (str-cat ?n ": too many additional bases loaded")))
			(assert (send-machine-update))
		)
	)
)

(defrule production-cs-move-to-mid
  "Start moving the workpiece to the middle if the CS is PREPARED."
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?m <- (machine (name ?n) (mtype CS) (state PREPARED) (task nil))
  (cs-meta (name ?n) (operation-mode ?cs-op))
  =>
  (printout t "Machine " ?n " prepared for " ?cs-op crlf)
  (modify ?m (task MOVE-MID) (mps-busy WAIT))
  (mps-move-conveyor (str-cat ?n) "MIDDLE" "FORWARD")
)

(defrule production-cs-main-op
	"Workpiece is in the middle of the CS. MOUNT or RETRIEVE the cap, depending
	 on the instructed task."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype CS) (state PREPARED) (task MOVE-MID) (mps-busy FALSE))
	(cs-meta (name ?n) (operation-mode ?cs-op))
	=>
	(modify ?m (state PROCESSING) (proc-start ?gt) (task ?cs-op) (mps-busy WAIT))
	(assert (send-machine-update))
	(if (eq ?cs-op RETRIEVE_CAP)
	 then
		(mps-cs-retrieve-cap (str-cat ?n))
		(printout t "Machine " ?n " retrieving a cap" crlf)
	 else
		(mps-cs-mount-cap (str-cat ?n))
		(printout t "Machine " ?n " mounting a cap" crlf)
	)
)

(defrule production-cs-move-to-output
	"The CS has completed mounting/retrieving a cap. Move the workpiece to the output."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype CS) (state PROCESSING) (team ?team)
	                 (task ?cs-op&RETRIEVE_CAP|MOUNT_CAP) (mps-busy FALSE))
	?cs-meta <- (cs-meta (name ?n) (operation-mode ?cs-op))
	(workpiece-tracking (enabled ?wt-enabled))
	=>
	(printout t "Machine " ?n ": move to output" crlf)
	(if (eq ?cs-op RETRIEVE_CAP) then
		(assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
		                (points ?*PRODUCTION-POINTS-RETRIEVE-CAP*)
		                (reason (str-cat "Retrieved cap at " ?n))))
		(if (eq ?wt-enabled FALSE) then
			(assert (product-processed (at-machine ?n) (mtype CS)
			                           (team ?team) (game-time ?gt))))
	else
		(assert (product-processed (at-machine ?n) (mtype CS)
		                           (team ?team) (game-time ?gt)))
	)
	(modify ?m (state PROCESSED) (proc-start ?gt) (task MOVE-OUT) (mps-busy WAIT))
	(assert (send-machine-update))
	(modify ?cs-meta (has-retrieved (eq ?cs-op RETRIEVE_CAP)))
	(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
)

(defrule production-bs-cs-rs-ss-ready-at-output
	"Workpiece is in output, switch to READY-AT-OUTPUT"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype BS|CS|RS|SS) (state PROCESSED) (task MOVE-OUT)
	               (mps-busy FALSE) (mps-ready TRUE))
	(or (workpiece-tracking (enabled FALSE))
	    (workpiece (at-machine ?n) (state AVAILABLE)))
	=>
	(modify ?m (proc-start ?gt) (state READY-AT-OUTPUT) (task nil))
	(assert (send-machine-update))
)

(defrule production-mps-product-retrieved
	"The workpiece has been taken away, set the machine to IDLE and reset it."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (state READY-AT-OUTPUT) (mps-ready FALSE))
	=>
	(printout t "Machine " ?n ": workpiece has been picked up" crlf)
	(modify ?m (state WAIT-IDLE) (idle-since ?gt))
	(assert (send-machine-update))
)

(defrule production-ds-start-processing-consumption
  "DS is prepared for order id 0."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PREPARED) (task nil))
	(ds-meta (name ?n) (order-id 0) (gate ?gate))
	=>
  (printout t "Machine " ?n " processing consumption" crlf)
	(assert (send-machine-update))
	(modify ?m (state PROCESSING) (proc-start ?gt)
	  (task DELIVER) (mps-busy WAIT))
  (mps-ds-process (str-cat ?n) ?gate)
)

(defrule production-ds-start-processing
  "DS is prepared, start processing the delivered workpiece."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PREPARED) (task nil))
	(ds-meta (name ?n) (order-id ?order))
	(order (id ?order) (delivery-gate ?gate)
	  (delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))))
	=>
  (printout t "Machine " ?n " processing to gate " ?gate " for order " ?order crlf)
	(assert (send-machine-update))
	(modify ?m (state PROCESSING) (proc-start ?gt)
	  (task DELIVER) (mps-busy WAIT))
  (mps-ds-process (str-cat ?n) ?gate)
)

(defrule production-ds-order-delivered
	"The DS processed the workpiece, ask the referee for confirmation of the delivery."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PROCESSING) (task DELIVER)
	               (mps-busy FALSE) (team ?team))
	(ds-meta (name ?n) (order-id ?order))
  =>
	(bind ?p-id (gen-int-id))
	(assert (product-processed (at-machine ?n) (mtype DS) (team ?team) (game-time ?gt)
	                           (id ?p-id) (order ?order) (confirmed FALSE)))
	(if (neq ?order 0) then
		(assert (referee-confirmation (process-id ?p-id) (state REQUIRED)))
		(assert (attention-message (team ?team)
		                           (text (str-cat "Please confirm delivery for order " ?order))))
	)
	(modify ?m (state PROCESSED) (task nil))
	(assert (send-machine-update))
)

(defrule production-ds-processed
  "The DS finished processing the workpiece, set the machine to IDLE and reset it."
	(declare (salience ?*PRIORITY_LAST*))
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PROCESSED))
	=>
  (printout t "Machine " ?n " finished processing" crlf)
  (modify ?m (state WAIT-IDLE) (idle-since ?gt))
	(assert (send-machine-update))
)

(defrule production-ss-simple-relocate-start
	" Another workpiece blocks the requested storage/retrieval operation.
	  Hence  the blocking workpiece is relocated to a different position.
	"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PROCESSING|PREPARED) (task nil))
	(ss-meta (current-shelf-slot ?shelf ?slot&:(eq 1 (mod ?slot 2))))
	(machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-accessible FALSE))
	?s <- (machine-ss-shelf-slot (position ?shelf ?front-slot&:(ss-slot-blocked-by ?slot ?front-slot))
	                             (is-filled TRUE) (is-accessible TRUE))

	(machine-ss-shelf-slot (name ?n) (position ?other-shelf ?other-slot)
	                       (is-filled FALSE) (is-accessible TRUE))
	=>
	(bind ?relocate-options (find-all-facts
	  ((?free machine-ss-shelf-slot))
	  (and (eq ?free:name ?n)
	       (eq ?free:is-filled FALSE)
	       (eq ?free:is-accessible TRUE))))
	(bind ?relocate-options (randomize$ ?relocate-options))
	(bind ?o (fact-slot-value (nth$ 1 ?relocate-options) position))
	(bind ?o-shelf (nth$ 1 ?o))
	(bind ?o-slot (nth$ 2 ?o))
	(printout t ?n " position (" ?shelf "," ?slot ") in not accessible, relocate "
	            "(" ?shelf "," ?front-slot") to (" ?o-shelf "," ?o-slot ")" crlf)
	(modify ?m (task RELOCATE) (proc-start ?gt) (state PROCESSING) (mps-busy WAIT))
	(modify ?s (move-to ?o-shelf ?o-slot))
	(mps-ss-relocate (str-cat ?n) ?shelf ?front-slot ?o-shelf ?o-slot)
)

(defrule production-ss-store-move-to-mid
	"Start moving the workpiece to the middle if the SS is PREPARED."
	(gamestate (state RUNNING) (phase PRODUCTION))
	?m <- (machine (name ?n) (mtype SS) (state PREPARED|PROCESSING) (task nil))
	(ss-meta (name ?n) (current-shelf-slot ?shelf ?slot) (current-operation STORE))
	(machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-filled FALSE)
	                       (is-accessible TRUE))
	=>
	(printout t "Machine " ?n " prepared for STORE" crlf)
	(modify ?m (task MOVE-MID) (mps-busy WAIT))
	(mps-move-conveyor (str-cat ?n) "MIDDLE" "FORWARD")
)

(defrule production-ss-double-relocate-start
	"SS is is prepared, but the target position is blocked by a product in front
	 of it. No slot is accessible to relocate the blocking product to.
	 Either the machine is full or a some positions in the back are free which
	 can be filled to free a front row slot with a relocation operation.
	"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PREPARED) (task nil))
	(ss-meta (name ?n) (current-operation ?ss-op) (current-shelf-slot ?shelf ?slot))
	?s <- (machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-accessible FALSE))
	(not (machine-ss-shelf-slot (name ?n) (is-accessible TRUE) (is-filled FALSE)))
	=>
	(bind ?relocate-options (find-all-facts
	  ((?free machine-ss-shelf-slot) (?front machine-ss-shelf-slot))
	  (and (eq ?free:name ?n)
	       (eq ?free:is-filled FALSE)
	       (neq ?free:position
	           (create$ ?shelf ?slot))
	       (eq ?front:name ?n)
	       (eq (nth$ 1 ?front:position) (nth$ 1 ?free:position))
	       (ss-slot-blocked-by (nth$ 2 ?free:position) (nth 2 ?front:position)))))
	(if (> (length$ ?relocate-options) 0)
	 then
		(bind ?relocate-options (randomize-tuple-list$ ?relocate-options ?*SS-SHELF-DEPTH*))
		(bind ?free-pos (fact-slot-value (nth$ 1 ?relocate-options) position))
		(bind ?front (nth$ 2 ?relocate-options))
		(bind ?front-pos (fact-slot-value ?front position))
		(printout t ?n " position (" ?shelf "," ?slot ") in not accessible" crlf)
		(printout t "No free accessible spot at " ?n ", relocate "
		            "(" (nth$ 1 ?front-pos)"," (nth$ 2 ?front-pos) ") to "
		            "(" (nth$ 1 ?free-pos) "," (nth$ 2 ?free-pos) ")" crlf)
		(printout t  ?n " has to make  (" ?shelf "," ?slot ") accessible" crlf)
		(modify ?m (state PROCESSING) (proc-start ?gt) (task RELOCATE) (mps-busy WAIT))
		(modify ?front (move-to ?free-pos))
		(mps-ss-relocate (str-cat ?n) (nth$ 1 ?front-pos) (nth$ 2 ?front-pos)
		                                (nth$ 1 ?free-pos) (nth$ 2 ?free-pos))
	 else
		(printout t  ?n " has no free spot to access inaccessible position (" ?shelf "," ?slot ")" crlf)
		(modify ?m (state BROKEN)
		  (broken-reason (str-cat "Prepare received for " ?n " for inaccessible slot while the storage is full.")))
	)
	(assert (send-machine-update))
)

(defrule production-ss-relocate-completed
	" SS is done relocating, update the storage information.
	"
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (team ?team) (mtype SS) (state PREPARED|PROCESSING)
	               (task RELOCATE) (mps-busy FALSE))
	(ss-meta (name ?n) (current-shelf-slot ?shelf ?slot))
	?s <- (machine-ss-shelf-slot (name ?n) (position ?curr-shelf ?curr-slot)
	                             (move-to ?target-shelf ?target-slot)
	                             (description ?description)
	                             (is-accessible TRUE) (is-filled TRUE))
	?t <- (machine-ss-shelf-slot (name ?n) (position ?target-shelf ?target-slot)
	                             (is-filled FALSE)
	                             (num-payments ?np)
	                             (last-payed ?lp))
	=>
	(modify ?m (task nil) (proc-start ?gt))
	(modify ?s (is-filled FALSE) (move-to (create$ ) ) (description ""))
	(modify ?t (is-filled TRUE) (num-payments ?np) (last-payed ?lp)
	           (description ?description))
	(ss-update-accessible-slots ?n ?curr-shelf ?curr-slot TRUE)
	(ss-update-accessible-slots ?n ?target-shelf ?target-slot FALSE)
	(ss-assert-points-with-threshold
	  (str-cat "Payment for relocating "
	           "(" ?curr-shelf "," ?curr-slot ") to "
	           "(" ?target-shelf "," ?target-slot ") in " ?n)
	  ?*PRODUCTION-POINTS-SS-RELOCATION*
	  ?gt ?team)
)

(defrule production-ss-retrieve-start
	"SS is prepared for retrieval, get the product from the shelf."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PROCESSING|PREPARED) (task nil))
	(ss-meta (name ?n) (current-operation ?ss-op&RETRIEVE) (current-shelf-slot ?shelf ?slot))
	(machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-accessible TRUE) (is-filled TRUE))
	=>
	(modify ?m (state PROCESSING) (proc-start ?gt) (task ?ss-op) (mps-busy WAIT))
	(assert (send-machine-update))
	(mps-ss-retrieve (str-cat ?n) ?shelf ?slot)
)

(defrule production-ss-store-start
	"SS has moved the product to the middle, store it on the shelf."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PREPARED|PROCESSING)
	               (task MOVE-MID) (mps-busy FALSE))
	(ss-meta (name ?n) (current-operation ?ss-op&STORE) (current-shelf-slot ?shelf ?slot))
	(machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-accessible TRUE) (is-filled FALSE))
	=>
	(modify ?m (state PROCESSING) (proc-start ?gt) (task ?ss-op) (mps-busy WAIT))
	(assert (send-machine-update))
	(mps-ss-store (str-cat ?n) ?shelf ?slot)
)

(defrule production-ss-retrieve-completed-move-to-output
	"The SS has completed the retrieval. Move the workpiece to the output."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PROCESSING) (team ?team)
	               (task RETRIEVE) (mps-busy FALSE))
	(ss-meta (name ?n) (current-shelf-slot ?shelf ?slot))
	?s <- (machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-filled TRUE))
	=>
	(printout t "Machine " ?n ": move to output" crlf)
	(modify ?m (state PROCESSED) (proc-start ?gt) (task MOVE-OUT) (mps-busy WAIT))
	; TODO: Is this the correct side?
	(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
	(ss-update-accessible-slots ?n ?shelf ?slot TRUE)
	(ss-assert-points-with-threshold
	  (str-cat "Payment for retrieving (" ?shelf "," ?slot ") from " ?n)
	  ?*PRODUCTION-POINTS-SS-RETRIEVAL*
	  ?gt ?team)
	(assert (send-machine-update))
	(modify ?s (is-filled FALSE) (description ""))
)

(defrule production-ss-store-completed
	"The SS processed the workpiece. "
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PROCESSING) (task STORE)
	               (mps-busy FALSE) (team ?team))
	(ss-meta (current-shelf-slot ?shelf ?slot) (current-wp-description ?description))
	?s <- (machine-ss-shelf-slot (name ?n) (position ?shelf ?slot) (is-filled FALSE))
	=>
	(printout t "Machine " ?n " finished storage at (" ?shelf ", " ?slot ")" crlf)
	(modify ?m (state PROCESSED) (proc-start ?gt))
	(assert (send-machine-update))
	(modify ?s (is-filled TRUE) (num-payments 0) (description ?description)
	           (last-payed (+ ?gt ?*SS-STORAGE-GRACE-PERIOD*)))
	(ss-update-accessible-slots ?n ?shelf ?slot FALSE)
	(ss-assert-points-with-threshold
	  (str-cat "Payment for storing ("?shelf "," ?slot") at " ?n)
	  ?*PRODUCTION-POINTS-SS-STORAGE*
	  ?gt ?team)
)

(defrule production-ss-store-processed
  "The SS finished storing the workpiece, set the machine to IDLE and reset it."
	(declare (salience ?*PRIORITY_LAST*))
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (task STORE) (state PROCESSED))
	=>
	(printout t "Machine " ?n " finished processing" crlf)
	(modify ?m (state WAIT-IDLE) (idle-since ?gt) (task nil))
	(assert (send-machine-update))
)

(defrule production-mps-idle
	"The machine has been in WAIT-IDLE for the specified time, switch to IDLE."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (state WAIT-IDLE)
	               (idle-since ?it&:(timeout-sec ?gt ?it ?*WAIT-IDLE-TIME*)))
	=>
	(modify ?m (state IDLE))
	(assert (send-machine-update))
	(mps-reset (str-cat ?n))
)

(defrule production-mps-broken
	"The MPS is BROKEN. Inform the referee and reset the machine to stop the conveyor belt."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?m <- (machine (name ?n) (state BROKEN) (team ?team) (broken-reason ?reason) (broken-since 0.0))
                 ;(bases-added ?ba) (bases-used ?bu) (has-retrieved ?cap-loaded))
  =>
  (printout t "Machine " ?n " broken: " ?reason crlf)
  (assert (attention-message (team ?team) (text ?reason)))
  (modify ?m (broken-since ?gt))
  ; Revoke points awarded for unused bases at RS slide
  (do-for-fact ((?meta rs-meta)) (eq ?meta:name ?n)
    (bind ?unused-bases (- ?meta:bases-added ?meta:bases-used))
    (if (> ?unused-bases 0) then
     (bind ?deduct (* -1 ?unused-bases))
     (assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
                     (points (* ?deduct ?*PRODUCTION-POINTS-ADDITIONAL-BASE*))
                     (reason (str-cat "Deducting unused additional bases at " ?n))))
    )
  )
  (do-for-fact ((?meta cs-meta)) (eq ?meta:name ?n)
    ; Revoke points awarded for an unused cap at CS
    (if ?meta:has-retrieved then
      (assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
                      (points (* -1  ?*PRODUCTION-POINTS-RETRIEVE-CAP*))
                      (reason (str-cat "Deducting retrieved cap at " ?n))))
    )
    (mps-reset (str-cat ?n))
  )
)

(defrule production-mps-broken-recover
	"Reset a machine that was BROKEN after the down time has passed."
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?m <- (machine (name ?n) (state BROKEN)
	               (broken-since ?bs&~0.0&:(timeout-sec ?gt ?bs ?*BROKEN-DOWN-TIME*)))
	=>
	(printout t "Machine " ?n " recovered" crlf)
	(modify ?m (state IDLE) (broken-since 0.0) (task nil) (broken-reason ""))
	(assert (send-machine-update))
	(do-for-fact ((?meta rs-meta)) (eq ?meta:name ?n)
		(modify ?meta (bases-used ?meta:bases-added))
	)
	(do-for-fact ((?meta cs-meta)) (eq ?meta:name ?n)
		(modify ?meta (has-retrieved FALSE))
	)
)

(defrule production-potential-timeout-while-machine-operation
  "The machine got stuck while processing the workpiece. Consult referee.
	 This may be caused by a machine failure or by a misplaced workpiece."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?m <- (machine (name ?n) (state ?state&:(member$ ?state (create$ PREPARED PROCESSING PROCESSED)))
        (proc-start ?start&:(timeout-sec ?gt ?start ?*PROCESSING-WAIT-TILL-WARNING*)))
  (not (machine-warning-sent ?n ?state))
	=>
	(assert (attention-message (text (str-cat "Please check " ?n))))
	(assert (machine-warning-sent ?n ?state))
)

(defrule production-timeout-warning-cleared
  ?mws <- (machine-warning-sent ?n ?state)
  (machine (name ?n) (state ~?state))
  =>
  (retract ?mws)
)

(defrule production-timeout-request-cleared
  ?m <- (machine (name ?n) (state IDLE) (referee-required TRUE))
  =>
  (modify ?m (referee-required FALSE) (broken-reason ""))
)

(defrule production-timeout-while-machine-operation
  "The machine got stuck while processing the workpiece. Set it to BROKEN.
	 This may be caused by a machine failure or by a misplaced workpiece."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?m <- (machine (name ?n) (state ?state&:(member$ ?state (create$ PREPARED PROCESSING PROCESSED))) (referee-required FALSE)
        (proc-start ?start&:(timeout-sec ?gt ?start ?*PROCESSING-WAIT-TILL-RESET*)))
	=>
	(modify ?m (referee-required TRUE) (broken-reason (str-cat "Stuck in state " ?state " for > " ?*PROCESSING-WAIT-TILL-RESET* "s")))
)

(defrule prodiction-machine-broken-by-referee
  ?m <- (machine (name ?name))
  ?rm <- (reset-machine ?name)
  =>
  (retract ?rm)
  (modify ?m (state BROKEN) (task nil) (broken-reason (str-cat "MPS reset by referee")))
  (assert (send-machine-update))
  (mps-reset (str-cat ?name))
)

(defrule production-pb-recv-SetMachineState
  "We received a manual override of the machine state, process it accordingly."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetMachineState") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (bind ?state (sym-cat (pb-field-value ?p "state")))
  (assert (production-SetMachineState ?mname ?state))
)

(defrule production-proc-SetMachineState
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?cmd <- (production-SetMachineState ?mname ?state)
  =>
  (retract ?cmd)
  (printout t "Received state " ?state " for machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
    (modify ?m (proc-start ?gt) (idle-since ?gt) (state ?state))
  )
  (assert (send-machine-update))
)

(defrule production-pb-recv-MachineAddBase
  "We received a manual SLIDE-COUNTER event. Process it just like a regular SLIDE-COUNTER event."
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.MachineAddBase") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (assert (production-MachineAddBase ?mname))
)

(defrule production-send-machine-positions
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt&:(> ?gt ?*EXPLORATION-TIME*)))
  ?send-pos <- (send-mps-positions (phases $?phases&:(not (member$ PRODUCTION ?phases))))
  (not (confval (path "/llsfrb/challenges/enable") (type BOOL) (value TRUE)))
  =>
  (modify ?send-pos (phases (append$ ?phases PRODUCTION)))
)

(defrule production-proc-MachineAddBase
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?cmd <- (production-MachineAddBase ?mname)
  =>
  (retract ?cmd)
  (printout t "Add base to machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
    (assert (mps-add-base-on-slide ?m:name))
  )
)

(defrule production-ss-continuous-costs
" Each occupied shelf slot of a SS causes periodic costs.
"
	(test (> ?*PRODUCTION-POINTS-SS-PER-STORED-VOLUME* 0))
	(gamestate (state RUNNING) (phase PRODUCTION))
	(time-info (game-time ?gt))
	?s <- (machine-ss-shelf-slot (name ?n) (is-filled TRUE) (position ?shelf ?slot)
	  (num-payments ?np&:(< ?np ?*SS-MAX-NUM-PAYMENTS-PER-VOLUME*))
	  (last-payed ?lp&:(> (- ?gt ?lp) ?*SS-PAYMENT-INTERVAL*)))
	(machine (name ?n) (team ?team))
	=>
	(modify ?s (num-payments (+ ?np 1))
	           (last-payed (+ ?lp ?*SS-PAYMENT-INTERVAL*)))
	(ss-assert-points-with-threshold
	  (str-cat "Payment for keeping product at " ?n " (" ?shelf "," ?slot ")")
	  ?*PRODUCTION-POINTS-SS-PER-STORED-VOLUME*
	  ?gt ?team)
)

(defrule production-mockup-ready-at-output-timeout
" Fake a ready feedback for mockup machines.
  Moved explicitly to CLIPS to avoid interference with DOWN machines.
"
  (or
   (confval (path ?p1&:(eq ?p1 (str-cat"/llsfrb/mps/mockup-ready-at-output-trigger"))) (value "timeout"))
   (not (confval (path ?p2&:(eq ?p2 (str-cat"/llsfrb/mps/mockup-ready-at-output-trigger")))))
  )
  (time-info (game-time ?gt))
  (machine (name ?mname) (state READY-AT-OUTPUT) (mps-ready TRUE)
    (proc-start ?start&:(timeout-sec ?gt ?start ?*MOCKUP-READY-AT-OUTPUT-TIMEOUT*)))
  (confval (path ?p3&:(eq ?p3 (str-cat"/llsfrb/mps/stations/" ?mname "/connection")))
    (type STRING) (is-list FALSE) (value "mockup"))
 =>
  (assert (mps-status-feedback ?mname READY FALSE))
)

(defrule production-skip-referee-confirmation-broken
  ?m <- (machine (referee-required TRUE))
  (confval (path "/llsfrb/auto-break-machines")
           (type BOOL) (value TRUE))
  =>
  (printout warn "Automatically breaking machine due to automatic referee setting" crlf)
  (modify ?m (state BROKEN) (referee-required FALSE))
)
