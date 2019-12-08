
;---------------------------------------------------------------------------
;  production.clp - LLSF RefBox CLIPS production phase rules
;
;  Created: Thu Feb 07 19:31:12 2013
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2017       Tobias Neumann
;             2019       Till Hofmann
;             2019       Mostafa Gomaa
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule production-start
  ?gs <- (gamestate (phase PRODUCTION) (prev-phase ~PRODUCTION))
  =>
  (modify ?gs (prev-phase PRODUCTION) (game-time 0.0))

  ; trigger machine info burst period
  (do-for-fact ((?sf signal)) (eq ?sf:type machine-info-bc)
    (modify ?sf (count 1) (time 0 0))
  )
  ;(assert (attention-message (text "Entering Production Phase")))
)

(defrule production-machine-down
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gt))
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
)

(defrule production-machine-up
  (gamestate (phase PRODUCTION) (state RUNNING) (game-time ?gt))
  ?mf <- (machine (name ?name) (state DOWN) (prev-state ?prev-state&~DOWN)
		  (down-period $?dp&:(<= (nth$ 2 ?dp) ?gt)))
  =>
	(printout t "Machine " ?name " is up again" crlf)
	(modify ?mf (state ?prev-state))
)

(defrule production-machine-prepare
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?pf <- (protobuf-msg (type "llsf_msgs.PrepareMachine") (ptr ?p)
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
		        (text (str-cat "Invalid prepare for team " ?team " of team " ?group))))
	 else
		(if (not (any-factp ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))))
		 then
			(assert (attention-message (team ?team)
			        (text (str-cat "Prepare received for invalid machine " ?mname))))
		 else
		 	(printout t "Received prepare for " ?mname crlf)
		 	(do-for-fact ((?m machine)) (and (eq ?m:name ?mname) (eq ?m:team ?team))
				(if (eq ?m:state IDLE) then
					(printout t ?mname " is IDLE, processing prepare" crlf)
					(switch ?m:mtype
					 (case BS then
						(if (pb-has-field ?p "instruction_bs")
						 then
							(bind ?prepmsg (pb-field-value ?p "instruction_bs"))
							(bind ?side (sym-cat (pb-field-value ?prepmsg "side")))
							(bind ?color (sym-cat (pb-field-value ?prepmsg "color")))
							(printout t "Prepared " ?mname " (side: " ?side ", color: " ?color ")" crlf)
							(modify ?m (state PREPARED) (bs-side  ?side) (bs-color ?color) (wait-for-product-since ?gt))
						 else
							(modify ?m (state BROKEN)
							(broken-reason (str-cat "Prepare received for " ?mname " without data")))
						))
					 (case DS then
						(if (pb-has-field ?p "instruction_ds")
						 then
							(bind ?prepmsg (pb-field-value ?p "instruction_ds"))
							(bind ?order-id (pb-field-value ?prepmsg "order_id"))
							(if (any-factp ((?order order)) (eq ?order:id ?order-id))
							 then
								(printout t "Prepared " ?mname " (order: " ?order-id ")" crlf)
								(modify ?m (state PREPARED) (ds-order ?order-id)
							                       (wait-for-product-since ?gt))
							 else
								(modify ?m (state BROKEN)
								  (broken-reason (str-cat "Prepare received for " ?mname " with invalid order ID")))
							)
						 else
							(modify ?m (state BROKEN)
							           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
					 ))
					 (case SS then
						(if (pb-has-field ?p "instruction_ss")
						 then
							(bind ?prepmsg (pb-field-value ?p "instruction_ss"))
							(bind ?operation (sym-cat (pb-field-value ?prepmsg "operation")))
							(if (eq ?operation RETRIEVE)
							 then
								(if ?m:ss-holding
								 then
									(printout t "Prepared " ?mname crlf)
									(modify ?m (state PREPARED) (ss-operation ?operation) (wait-for-product-since ?gt))
								 else
									(modify ?m (state BROKEN)
									           (broken-reason (str-cat "Prepare received for " ?mname ", but station is empty")))
								)
							 else
								(if (eq ?operation STORE)
								 then
									(modify ?m (state BROKEN) (broken-reason (str-cat "Prepare received for " ?mname " with STORE operation")))
								 else
								 	(modify ?m (state BROKEN) (broken-reason (str-cat "Prepare received for " ?mname " with unknown operation " ?operation)))
								)
							)
						 else
							(modify ?m (state BROKEN)
							           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
					 ))
					 (case RS then
						(if (pb-has-field ?p "instruction_rs")
						 then
							(bind ?prepmsg (pb-field-value ?p "instruction_rs"))
							(bind ?ring-color (sym-cat (pb-field-value ?prepmsg "ring_color")))
							(if (member$ ?ring-color ?m:rs-ring-colors)
							 then
								(printout t "Prepared " ?mname " (ring color: " ?ring-color ")" crlf)
								(modify ?m (state PREPARED) (rs-ring-color ?ring-color)
								           (wait-for-product-since ?gt))
							 else
								(modify ?m (state BROKEN)
								           (broken-reason (str-cat "Prepare received for " ?mname " for invalid ring color (" ?ring-color ")")))
							)
						 else
							(modify ?m (state BROKEN)
							(broken-reason (str-cat "Prepare received for " ?mname " without data")))
					 ))
					 (case CS then
						(if (pb-has-field ?p "instruction_cs")
						 then
							(bind ?prepmsg (pb-field-value ?p "instruction_cs"))
							(bind ?cs-op (sym-cat (pb-field-value ?prepmsg "operation")))
							(switch ?cs-op
					
							 (case RETRIEVE_CAP then
								(if (not ?m:cs-retrieved)
								 then
									(printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
									(modify ?m (state PREPARED) (cs-operation ?cs-op)
									           (wait-for-product-since ?gt))
								 else
									(modify ?m (state BROKEN)
									           (broken-reason (str-cat "Prepare received for " ?mname ": "
									                                   "cannot retrieve while already holding")))
							 ))
							 (case MOUNT_CAP then
								(if ?m:cs-retrieved
								 then
									(printout t "Prepared " ?mname " (" ?cs-op ")" crlf)
									(modify ?m (state PREPARED) (cs-operation ?cs-op)
									           (wait-for-product-since ?gt))
								 else
									(modify ?m (state BROKEN)
									           (broken-reason (str-cat "Prepare received for " ?mname
									                                   ": cannot mount without cap")))
							)))
						 else
							(modify ?m (state BROKEN)
							           (broken-reason (str-cat "Prepare received for " ?mname " without data")))
					)))
				 else
					(if (eq ?m:state READY-AT-OUTPUT) then
						(modify ?m (state BROKEN)))
				)
			)
		)
	)
)

(defrule production-machine-reset-by-team
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
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
	?m <- (machine (name ?n) (mps-base-counter ?mps-counter))
	?fb <- (mps-status-feedback ?n SLIDE-COUNTER ?new-counter&:(> ?new-counter ?mps-counter))
	=>
	(retract ?fb)
	(modify ?m (mps-base-counter ?new-counter))
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
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype BS) (state PREPARED) (bs-color ?color) (task nil))
  =>
  (printout t "Machine " ?n " dispensing " ?color " base" crlf)
	(modify ?m (state PROCESSING) (proc-start ?gt) (task DISPENSE) (mps-busy WAIT))
  (mps-bs-dispense (str-cat ?n) (str-cat ?color))
)

(defrule production-bs-move-conveyor
  "The BS has dispensed a base. We now need to move the conveyor"
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype BS) (state PROCESSING) (task DISPENSE) (mps-busy FALSE)
                   (bs-side ?side) (bs-color ?bs-color) (team ?team))
	(workpiece-tracking (enabled ?tracking-enabled))
	=>
	(printout t "Machine " ?n " moving base to " ?side crlf)
	(if ?tracking-enabled then
		(assert (product-processed (at-machine ?n) (mtype BS) (team ?team)
		                           (game-time ?gt) (base-color ?bs-color))))
	(modify ?m (task MOVE-OUT) (state PROCESSED) (mps-busy WAIT))
	(if (eq ?side INPUT)
	 then
		(mps-move-conveyor (str-cat ?n) "INPUT" "BACKWARD")
	 else
		(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
	)
)

(defrule production-rs-insufficient-bases
  "The RS has been prepared but it does not have sufficient additional material; switch to BROKEN."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED)
		 (rs-ring-color ?ring-color) (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color)
	     (req-bases ?req-bases&:(> ?req-bases (- ?ba ?bu))))
  =>
  (modify ?m (state BROKEN)
	  (broken-reason (str-cat ?n ": insufficient bases ("
				  (- ?ba ?bu) " < " ?req-bases ")")))
)

(defrule production-rs-move-to-mid
  "The RS is PREPARED. Start moving the workpiece to the middle to mount a ring."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype RS) (state PREPARED) (task nil)
		 (rs-ring-color ?ring-color) (rs-ring-colors $?ring-colors)
                 (bases-added ?ba) (bases-used ?bu))
  (ring-spec (color ?ring-color) (req-bases ?req-bases&:(>= (- ?ba ?bu) ?req-bases)))
  =>
  (printout t "Machine " ?n " of type RS switching to PREPARED state" crlf)
  (modify ?m (task MOVE-MID) (mps-busy WAIT))
  (mps-move-conveyor (str-cat ?n) "MIDDLE" "FORWARD")
)

(defrule production-rs-mount-ring
  "Workpiece is in the middle of the conveyor belt of the RS, mount a ring."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype RS) (state PREPARED) (task MOVE-MID) (mps-busy FALSE)
	               (rs-ring-color ?ring-color) (rs-ring-colors $?ring-colors) (bases-used ?bu))
  (ring-spec (color ?ring-color) (req-bases ?req-bases))
	=>
	(printout t "Machine " ?n ": mount ring" crlf)
	(modify ?m (state PROCESSING) (proc-start ?gt) (task MOUNT-RING) (mps-busy WAIT)
	           (bases-used (+ ?bu ?req-bases)))
  (mps-rs-mount-ring (str-cat ?n) (member$ ?ring-color ?ring-colors))
)

(defrule production-rs-move-to-output
	"Ring is mounted, move to output"
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype RS) (state PROCESSING) (task MOUNT-RING) (mps-busy FALSE)
	               (rs-ring-color ?ring-color) (team ?team))
	(workpiece-tracking (enabled ?tracking-enabled))
	=>
	(printout t "Machine " ?n ": move to output" crlf)
	(if ?tracking-enabled then
		(assert (product-processed (at-machine ?n) (mtype RS) (team ?team)
		                           (game-time ?gt) (ring-color ?ring-color))))
	(modify ?m (state PROCESSED) (task MOVE-OUT) (mps-busy WAIT))
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
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (state ?state) (mtype RS) (team ?team)
	               (bases-used ?bases-used) (bases-added ?old-num-bases))
	?fb <- (mps-add-base-on-slide ?n)
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
			(modify ?m (bases-added ?num-bases))
		 else
			(modify ?m (state BROKEN)
			           (broken-reason (str-cat ?n ": too many additional bases loaded")))
		)
	)
)

(defrule production-cs-move-to-mid
  "Start moving the workpiece to the middle if the CS is PREPARED."
  (gamestate (state RUNNING) (phase PRODUCTION))
  ?m <- (machine (name ?n) (mtype CS) (state PREPARED) (task nil)
	               (cs-operation ?cs-op))
  =>
  (printout t "Machine " ?n " prepared for " ?cs-op crlf)
  (modify ?m (task MOVE-MID) (mps-busy WAIT))
	(mps-move-conveyor (str-cat ?n) "MIDDLE" "FORWARD")
)

(defrule production-cs-main-op
	"Workpiece is in the middle of the CS. MOUNT or RETRIEVE the cap, depending
	 on the instructed task."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype CS) (state PREPARED) (task MOVE-MID) (mps-busy FALSE)
	               (cs-operation ?cs-op))
	=>
	(modify ?m (state PROCESSING) (proc-start ?gt) (task ?cs-op) (mps-busy WAIT))
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
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype CS) (state PROCESSING) (team ?team)
                   (task ?cs-op&RETRIEVE_CAP|MOUNT_CAP) (mps-busy FALSE))
	(workpiece-tracking (enabled ?tracking-enabled))
	=>
	(printout t "Machine " ?n ": move to output" crlf)
	(if (eq ?cs-op RETRIEVE_CAP) then
		(assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
		                (points ?*PRODUCTION-POINTS-RETRIEVE-CAP*)
		                (reason (str-cat "Retrieved cap at " ?n))))
	else (if ?tracking-enabled then
		(assert (product-processed (at-machine ?n) (mtype CS)
		                           (team ?team) (game-time ?gt))))
	)
	(modify ?m (state PROCESSED) (task MOVE-OUT) (mps-busy WAIT)
	           (cs-retrieved (eq ?cs-op RETRIEVE_CAP)))
	(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
)

(defrule production-bs-cs-rs-ss-ready-at-output
	"Workpiece is in output, switch to READY-AT-OUTPUT"
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype BS|CS|RS|SS) (state PROCESSED) (task MOVE-OUT)
	               (mps-busy FALSE) (mps-ready TRUE))
	(or (workpiece-tracking (enabled FALSE))
	    (workpiece (at-machine ?n) (state AVAILABLE)))
	=>
	(modify ?m (state READY-AT-OUTPUT) (task nil))
)

(defrule production-mps-product-retrieved
	"The workpiece has been taken away, set the machine to IDLE and reset it."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (state READY-AT-OUTPUT) (mps-ready FALSE))
	=>
	(printout t "Machine " ?n ": workpiece has been picked up" crlf)
	(modify ?m (state WAIT-IDLE) (idle-since ?gt))
)

(defrule production-ds-start-processing
  "DS is prepared, start processing the delivered workpiece."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PREPARED) (task nil) (ds-order ?order))
  (order (id ?order) (delivery-gate ?gate))
	=>
  (printout t "Machine " ?n " processing to gate " ?gate " for order " ?order crlf)
	(modify ?m (state PROCESSING) (proc-start ?gt) (task DELIVER) (mps-busy WAIT))
  (mps-ds-process (str-cat ?n) ?gate)
)

(defrule production-ds-order-delivered
	"The DS processed the workpiece, ask the referee for confirmation of the delivery."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PROCESSING) (task DELIVER) (mps-busy FALSE)
	               (ds-order ?order) (team ?team))
  =>
	(bind ?p-id (gen-int-id))
	(assert (product-processed (at-machine ?n) (mtype DS) (team ?team) (game-time ?gt)
	                           (id ?p-id) (order ?order) (confirmed FALSE)))
	(assert (referee-confirmation (process-id ?p-id) (state REQUIRED)))
	(assert (attention-message (team ?team)
	                           (text (str-cat "Please confirm delivery for order " ?order))))
	(modify ?m (state PROCESSED) (task nil))
)

(defrule production-ds-processed
  "The DS finished processing the workpiece, set the machine to IDLE and reset it."
	(declare (salience ?*PRIORITY_LAST*))
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype DS) (state PROCESSED))
	=>
  (printout t "Machine " ?n " finished processing" crlf)
  (modify ?m (state WAIT-IDLE) (idle-since ?gt))
)

(defrule production-ss-start-retrieval
	"SS is prepared, move the workpiece to the output."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (state PREPARED)
                 (ss-operation RETRIEVE) (task nil))
	=>
	(modify ?m (state PROCESSING) (proc-start ?gt))
)

(defrule production-ss-processed-retrieval
	"The conveyor started moving, go to processed."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (mtype SS) (team ?team) (state PROCESSING) (ss-operation RETRIEVE)
	               (proc-start ?t&:(timeout-sec ?gt ?t ?*PROCESS-TIME-SS*)))
	=>
	(assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
	                (points ?*PRODUCTION-POINTS-SS-RETRIEVAL*)
	                (reason (str-cat "Retrieved product from " ?n))))
	(mps-move-conveyor (str-cat ?n) "OUTPUT" "FORWARD")
	(modify ?m (state PROCESSED) (task MOVE-OUT) (mps-busy WAIT) (ss-holding FALSE))
)

(defrule production-mps-idle
	"The machine has been in WAIT-IDLE for the specified time, switch to IDLE."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (state WAIT-IDLE)
	               (idle-since ?it&:(timeout-sec ?gt ?it ?*WAIT-IDLE-TIME*)))
	=>
	(modify ?m (state IDLE))
	(mps-reset (str-cat ?n))
)

(defrule production-mps-broken
	"The MPS is BROKEN. Inform the referee and reset the machine to stop the conveyor belt."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state BROKEN) (team ?team) (broken-reason ?reason) (broken-since 0.0)
                 (bases-added ?ba) (bases-used ?bu) (cs-retrieved ?cap-loaded))
  =>
  (printout t "Machine " ?n " broken: " ?reason crlf)
  (assert (attention-message (team ?team) (text ?reason)))
  (modify ?m (broken-since ?gt))
  ; Revoke points awarded for unused bases at RS slide
  (bind ?unused-bases (- ?ba ?bu))
  (if (> ?unused-bases 0) then
   (bind ?deduct (* -1 ?unused-bases))
   (assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
                   (points (* ?deduct ?*PRODUCTION-POINTS-ADDITIONAL-BASE*))
                   (reason (str-cat "Deducting unused additional bases at " ?n))))
  )
  ; Revoke points awarded for an unused cap at CS
  (if ?cap-loaded then
    (assert (points (game-time ?gt) (team ?team) (phase PRODUCTION)
                    (points (* -1  ?*PRODUCTION-POINTS-RETRIEVE-CAP*))
                    (reason (str-cat "Deducting retrieved cap at " ?n))))
  )
  (mps-reset (str-cat ?n))
)

(defrule production-mps-broken-recover
	"Reset a machine that was BROKEN after the down time has passed."
	(gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
	?m <- (machine (name ?n) (state BROKEN) (bases-added ?ba)
	               (broken-since ?bs&~0.0&:(timeout-sec ?gt ?bs ?*BROKEN-DOWN-TIME*)))
	=>
	(printout t "Machine " ?n " recovered" crlf)
	(modify ?m (state IDLE) (bases-used ?ba) (cs-retrieved FALSE) (broken-since 0.0))
)

(defrule production-prepared-but-no-input
  "The machine has been prepared but has never received a workpiece. Set it to BROKEN."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (mtype ?type&~BS) (state ?state
	                 &:(or (and (eq ?type DS) (eq ?state PROCESSING)) (eq ?state PREPARED)))
	               (wait-for-product-since ?ws&:(timeout-sec ?gt ?ws ?*PREPARE-WAIT-TILL-RESET*)))
  =>
  (modify ?m (state BROKEN)
             (broken-reason (str-cat "MPS " ?n " prepared, but no product fed in time")))
)

(defrule production-timeout-while-processing
  "The machine got stuck while processing the workpiece. Set it to BROKEN.
	 This may be caused by a machine failure or by a misplaced workpiece."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?m <- (machine (name ?n) (state PROCESSING|PROCESSED)
        (proc-start ?start&:(timeout-sec ?gt ?start ?*PROCESSING-WAIT-TILL-RESET*)))
	=>
	(printout error "Machine " ?n " timed out while processing" crlf)
	(modify ?m (state BROKEN) (task nil)
	           (broken-reason (str-cat "MPS " ?n " timed out while processing")))
	(mps-reset (str-cat ?n))
)

(defrule production-pb-recv-SetMachineState
  "We received a manual override of the machine state, process it accordingly."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetMachineState") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (bind ?state (sym-cat (pb-field-value ?p "state")))
  (printout t "Received state " ?state " for machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
		(modify ?m (state ?state))
	)
)

(defrule production-pb-recv-MachineAddBase
  "We received a manual SLIDE-COUNTER event. Process it just like a regular SLIDE-COUNTER event."
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.MachineAddBase") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?mname (sym-cat (pb-field-value ?p "machine_name")))
  (printout t "Add base to machine " ?mname crlf)
  (do-for-fact ((?m machine)) (eq ?m:name ?mname)
		(assert (mps-add-base-on-slide ?m:name))
  )
)
