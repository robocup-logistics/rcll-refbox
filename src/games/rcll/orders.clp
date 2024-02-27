
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2019  Mostafa Gomaa  [mostafa.gomaa@rwth-aachen.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction order-q-del-index (?team)
  (if (eq ?team CYAN) then (return 1) else (return 2))
)

(deffunction order-q-del-other-index (?team)
	(if (eq ?team CYAN)
	 then
		(return (order-q-del-index MAGENTA))
	 else
		(return (order-q-del-index CYAN))
	)
)

(deffunction order-q-del-team (?q-del ?team)
	(return (nth$ (order-q-del-index ?team) ?q-del))
)

(defrule activate-order
  (gamestate (state RUNNING) (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?of <- (order (id ?id) (active FALSE) (activate-at ?at&:(>= ?gt ?at))
		(complexity ?c) (quantity-requested ?q) (delivery-period $?period))
  ?sf <- (signal (type order-info))
  =>
  (modify ?of (active TRUE))
  (modify ?sf (count 1) (time 0 0))
  (assert (attention-message (text (str-cat "Order " ?id ": " ?q " x " ?c " from "
					    (time-sec-format (nth$ 1 ?period)) " to "
					    (time-sec-format (nth$ 2 ?period))))
			     (time 10)))
)

; Sort orders by ID, such that do-for-all-facts on the orders deftemplate
; iterates in a nice order, e.g. for net-send-OrderInstruction
(defrule sort-orders
  (declare (salience ?*PRIORITY-HIGH*))
  ?oa <- (order (id ?id-a))
  ?ob <- (order (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?oa)
)

(defrule order-recv-SetOrderDelivered
  (gamestate (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetOrderDelivered") (ptr ?p) (rcvd-via STREAM))
  =>
  (bind ?team (pb-field-value ?p "team_color"))
  (bind ?order (pb-field-value ?p "order_id"))
  (assert (order-SetOrderDelivered ?team ?order))
)

(defrule order-proc-SetOrderDelivered
  (gamestate (phase PRODUCTION))
  (time-info (game-time ?gt))
  ?cmd <- (order-SetOrderDelivered ?team-color ?order)
  =>
  (bind ?wp-id (workpiece-simulate-tracking ?order ?team-color ?gt))
  (assert (product-processed (game-time ?gt) (team ?team-color) (order ?order) (confirmed TRUE)
                             (workpiece ?wp-id) (mtype DS) (scored FALSE)))
  (printout t "Delivery by team " ?team-color " for order " ?order " reported!" crlf)
  (retract ?cmd)
)

(defrule order-monitor-SetOrderDelivered
  ?cmd <- (order-SetOrderDelivered ?team-color ?order)
  =>
  (printout t "Delivery by team " ?team-color " for order " ?order " reported!" crlf)
)

(defrule order-recv-ConfirmDelivery
  (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (protobuf-msg (type "llsf_msgs.ConfirmDelivery") (ptr ?p) (rcvd-via STREAM))
  =>
  (bind ?delivery-id (pb-field-value ?p "delivery_id"))
  (bind ?correctness (pb-field-value ?p "correct"))
  (bind ?order-id (pb-field-value ?p "order_id"))
  (bind ?team-color (pb-field-value ?p "team_color"))

  (assert (order-ConfirmDelivery ?delivery-id ?correctness ?order-id ?team-color))
  (retract ?pf)
)

(defrule order-proc-ConfirmDelivery
  (gamestate (phase PRODUCTION|POST_GAME))
  ?cmd <- (order-ConfirmDelivery ?delivery-id ?correctness ?order-id ?team-color)
  =>
  (if (not (do-for-fact ((?rc referee-confirmation))
            (and (eq ?rc:process-id ?delivery-id)
                 (eq ?rc:state REQUIRED))
            (if (eq ?correctness TRUE) then
              (modify ?rc (state CONFIRMED))
            else
              (modify ?rc (state DENIED)))
              (assert (ws-update-order-cmd ?delivery-id))
            ; make sure do-for-fact evaluates to TRUE
            TRUE))
   then
    (printout error "Received invalid SetOrderDelivered"
                    " (order " ?order-id
                    ", team " ?team-color ")" crlf)
  )
  (retract ?cmd)
)


(defrule order-referee-confirmation-automatic
  "Automatically grant referee confirmation after DS idles,
  if auto-confirm enabled"
  (gamestate (phase PRODUCTION|POST_GAME))
  (confval (path "/llsfrb/auto-confirm-delivery")
           (type BOOL) (value TRUE))
  ?rf <- (referee-confirmation (process-id ?p-id) (state REQUIRED))
  (product-processed (id ?p-id) (at-machine ?mname) (mtype DS) (order ?o-id))
  (machine (name ?mname) (state IDLE))
  (order (id ?o-id) (active TRUE))
  =>
  (printout warn "Automatic confirmation of delivery" crlf)
  (modify ?rf (state CONFIRMED))
)

(defrule order-delivery-confirmation-invalid-operation
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state ~REQUIRED))
  (not (product-processed (id ?id)))
  =>
  (retract ?rf)
  (printout error "Confirmation could not be linked to an operation" crlf)
)

(defrule order-delivery-confirmation-invalid-order
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order))
  (not (order (id ?order) (active TRUE)))
  =>
  (retract ?pf)
  (retract ?rf)
  (assert (attention-message (team ?team)
                             (text (str-cat "Invalid order delivered by " ?team ": "
                             "no active order with ID " ?order crlf))))
)

(defrule order-delivery-confirmation-referee-denied
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state DENIED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE))
   =>
  (retract ?pf)
  (retract ?rf)
  (printout t "Denied delivery for order " ?order  " by team " ?team crlf)
)

(defrule order-delivery-confirmation-referee-confirmed-print
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE))
  (order (id ?order))
   =>
  (printout t "Confirmed delivery for order " ?order  " by team " ?team crlf)
)

(defrule order-delivery-confirmation-referee-confirmed-simulate-tracking
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order) (active TRUE))
  (or (workpiece-tracking (enabled FALSE))
      (workpiece-tracking (fail-safe FALSE)))
  =>
  (bind ?wp-id (workpiece-simulate-tracking ?order ?team ?delivery-time))
  (modify ?pf (workpiece ?wp-id) (confirmed TRUE))
  (retract ?rf)
)

(defrule order-delivery-confirmation-referee-confirmed-DS-read-fail-recovery
 "Recover from reading failure at DS, iff there is a unique, fitting, caped unconfirmed, workpiece"
  (declare (salience ?*PRIORITY-HIGH*))
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order)
                (active TRUE)
                (base-color ?base-color)
                (ring-colors $?ring-colors))
  (workpiece (id ?wp-id)
             (latest-data TRUE)
             (team ?team)
             (base-color ?base-color)
             (ring-colors $?ring-colors))
  (product-processed (workpiece ?wp-id) (mtype CS) (confirmed FALSE))
  (workpiece-tracking (enabled TRUE))
  (not (and
      (workpiece (team ?team)
                 (latest-data TRUE)
                 (base-color ?base-color)
                 (ring-colors $?ring-colors)
                 (id ?wpp-id&:(neq ?wpp-id ?wp-id)))
      (product-processed (workpiece ?wpp-id) (mtype CS) (confirmed FALSE)))
  )
  =>
  (printout t "Delivery for order " ?order  " by team " ?team " not linked to a workpiece" crlf)
  (printout t "Linking to a fitting unique workpiece " ?wp-id   " ready for delivery " crlf)
  (modify ?pf (workpiece ?wp-id))
)

(defrule order-delivery-confirmation-referee-confirmed-DS-read-fail-safe
 "Disable tracking if fai-safe is on and couldn't recover"
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece 0) (game-time ?delivery-time))
  ?of <- (order (id ?order) (active TRUE))
  ?wf <- (workpiece-tracking (enabled TRUE) (fail-safe TRUE))
  =>
  (printout t "Could not find a unique workpiece fitting the delivery " crlf)
  (modify ?wf (enabled FALSE) (reason (str-cat "Fail-safe reader at delivery" )))
)


(defrule order-delivery-confirmation-referee-confirmed-workpiece-rectify
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  (time-info (game-time ?gt))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece ?workpiece-id) (game-time ?delivery-time))
  ?wf <- (workpiece (id ?workpiece-id&~0)
                    (latest-data TRUE)
                    (order ?workpiece-order)
                    (cap-color ?workpiece-cap)
                    (base-color ?workpiece-base)
                    (ring-colors $?workpiece-rings))
  ?of <- (order (id ?order)
                (active TRUE)
                (cap-color ?order-cap)
                (base-color ?order-base)
                (ring-colors $?order-rings))
  (not (rectified ?workpiece-id))
  =>
  (printout t "Verifying unconfirmed operations performed on workpiece " ?workpiece-id  crlf)
  ; rectify base color
  ; information on workpiece is known:
  (bind ?base-needs-rectify TRUE)
  (do-for-fact ((?pd product-processed))
               (and (eq ?pd:mtype BS)
                    (eq ?pd:workpiece ?workpiece-id)
               )
               (if (neq ?pd:base-color ?order-base) then
                 (printout warn "Product processed with " ?pd:base-color " but the workpiece needed " ?order-base  crlf)
               )
               (modify ?pd (base-color ?order-base) (order ?order) (confirmed TRUE))
               (bind ?base-needs-rectify FALSE)
  )
  (if (neq ?workpiece-base ?order-base) then
    (printout t "Rectifying workpiece " ?workpiece-id ": operation at BS [" ?workpiece-base
                 "->" ?order-base "]"  crlf)
    (if ?base-needs-rectify
      then ; Some unconfirmed and isolated operation dispensed a wp with the correct color
      (if (not (do-for-fact ((?pd product-processed) (?o-wp workpiece))
                 (and (eq ?pd:mtype BS)
                      (eq ?o-wp:id ?pd:workpiece)
                      (neq ?pd:workpiece ?workpiece-id)
                      (eq ?pd:base-color ?order-base)
                      (eq ?pd:confirmed FALSE)
                      ?o-wp:latest-data
                      (eq ?o-wp:base-color ?order-base)
                      ; no other info is set
                      ; ideally partial wp information could be used, but this
                      ; gets complicated when the correct partial wp
                      ; information should be chosen, so we only cover the
                      ; simplest case
                      (eq ?o-wp:cap-color nil)
                      (eq ?o-wp:ring-colors (create$))
                 )
                 (printout t "Mapping unconfirmed operation: operation at BS [" ?o-wp:id "(" ?o-wp:name ")"
                             "->" ?workpiece-id "]"  crlf)
                 (delayed-do-for-all-facts ((?all-p product-processed)) (eq ?all-p:workpiece ?o-wp:id)
                   (modify ?all-p (workpiece ?workpiece-id) (confirmed TRUE) (order ?order))
                 )
                 (delayed-do-for-all-facts ((?all-wp workpiece)) (eq ?all-wp:id ?o-wp:id)
                   (modify ?all-wp (id ?workpiece-id) (order ?order) (latest-data FALSE))
                 )
        TRUE))
        then ; No known operation produced the wp, add it anyways as it was confirmed
        (printout warn "No prior unconfirmed processing step at BS, creating new one" crlf)
        (assert (product-processed (mtype BS)
                                   (team ?team)
                                   (order ?order)
                                   (confirmed TRUE)
                                   (workpiece ?workpiece-id)
                                   (game-time ?delivery-time)
                                   (base-color ?order-base)))
      )
    )
  )
  ; rectify cap color
  ; information on workpiece is known:
  (bind ?cap-needs-rectify TRUE)
  (do-for-fact ((?pd product-processed))
               (and
                    (eq ?pd:mtype CS)
                    (eq ?pd:workpiece ?workpiece-id)
               )
               (if (neq ?pd:cap-color ?order-cap) then
                 (printout warn "Product processed with " ?pd:cap-color " but the workpiece needed " ?order-cap  crlf)
               )
               (modify ?pd (cap-color ?order-cap) (confirmed TRUE) (scored FALSE))
               (bind ?cap-needs-rectify FALSE)
  )
  (if (neq ?workpiece-cap ?order-cap) then
    (printout t "Rectifying workpiece " ?workpiece-id ": operation at CS [" ?workpiece-cap
                 "->" ?order-cap "]"  crlf)
    (if ?cap-needs-rectify then
      ; Some unconfirmed operation mounted a cap with matching ord unknown color
      (if (not (do-for-fact ((?pd product-processed) (?o-wp workpiece))
                 (and (eq ?pd:mtype CS)
                      (eq ?o-wp:id ?pd:workpiece)
                      (neq ?pd:workpiece ?workpiece-id)
                      (or (eq ?pd:cap-color ?order-cap)
                          (eq ?pd:cap-color CAP_UNKNOWN)
                      )
                      (eq ?pd:confirmed FALSE)
                      ?o-wp:latest-data
                      (eq ?o-wp:cap-color ?pd:cap-color)
                      (eq ?o-wp:base-color nil)
                      (eq (create$ ?o-wp:ring-colors) (create$))
                 )
                 (printout t "Mapping unconfirmed operation: operation at CS [" ?o-wp:id "(" ?o-wp:name ")"
                             "->" ?workpiece-id "]"  crlf)
                 (modify ?pd (cap-color ?order-cap))
                 (delayed-do-for-all-facts ((?all-p product-processed)) (eq ?all-p:workpiece ?o-wp:id)
                   (modify ?all-p (workpiece ?workpiece-id) (confirmed TRUE) (order ?order))
                 )
                 (delayed-do-for-all-facts ((?all-wp workpiece)) (eq ?all-wp:id ?o-wp:id)
                   (modify ?all-wp (id ?workpiece-id) (order ?order) (latest-data FALSE))
                 )
        TRUE))
        then ; No known operation produced the wp, add it anyways as it was confirmed
          (printout warn "No prior unconfirmed processing step at CS, creating new one" crlf)
          (assert (product-processed (mtype CS)
                                     (team ?team)
                                     (scored FALSE)
                                     (confirmed TRUE)
                                     (order ?order)
                                     (at-machine (sym-cat (sub-string 1 1 ?team) -CS1))
                                     (workpiece ?workpiece-id)
                                     (game-time ?delivery-time)
                                     (base-color ?order-base)
                                     (cap-color ?order-cap)))
      )
    )
  )
  ; information on workpiece is known:
  (progn$ (?order-ring ?order-rings)
    (bind ?workpiece-ring (nth$ ?order-ring-index ?workpiece-rings))
    (bind ?ring-needs-rectify TRUE)
    (do-for-fact ((?pd product-processed))
            (and (eq ?pd:mtype RS)
                 (eq ?pd:workpiece ?workpiece-id)
                 (not ?pd:confirmed))
            (if (neq ?pd:ring-color ?order-ring) then
              (printout warn "Product processed with " ?pd:ring-color " but the workpiece needed " ?order-ring " (ring " ?order-ring-index")"  crlf)
            )
            (modify ?pd (ring-color ?order-ring) (confirmed TRUE))
            (bind ?ring-needs-rectify FALSE)
    )
    (if (neq ?workpiece-ring ?order-ring) then
      (printout t "Rectifying workpiece " ?workpiece-id ": operation at RS [" ?workpiece-ring
                  "->" ?order-ring "]"  crlf)
      (if ?ring-needs-rectify
        then ; Some unconfirmed operation mounted a cap with the correct color
        (if (not (do-for-fact ((?pd product-processed) (?o-wp workpiece))
                   (and (eq ?pd:mtype RS)
                        (eq ?o-wp:id ?pd:workpiece)
                        (neq ?pd:workpiece ?workpiece-id)
                        (eq ?pd:ring-color ?order-ring)
                        (eq ?pd:confirmed FALSE)
                        ?o-wp:latest-data
                        (eq (create$ ?o-wp:ring-colors) (create$ ?order-ring))
                        (eq ?o-wp:base-color nil)
                        (eq ?o-wp:cap-color nil)
                        ; ring color matches
                        (eq ?order-ring-index (nth$ 1 (create$ (member$ ?o-wp:ring-colors ?order-rings))))
                   )
                   (printout t "Mapping unconfirmed operation: operation at RS [" ?o-wp:id "(" ?o-wp:name ")"
                               "->" ?workpiece-id "]"  crlf)
                   (delayed-do-for-all-facts ((?all-p product-processed)) (eq ?all-p:workpiece ?o-wp:id)
                     (modify ?all-p (workpiece ?workpiece-id) (confirmed TRUE) (order ?order))
                   )
                   (delayed-do-for-all-facts ((?all-wp workpiece)) (eq ?all-wp:id ?o-wp:id)
                     (modify ?all-wp (id ?workpiece-id) (order ?order) (latest-data FALSE))
                   )
                   TRUE))
          then ; No known operation produced the wp, add it anyways as it was confirmed
          (printout warn "No prior unconfirmed processing step at RS, creating new one" crlf)
          (assert (product-processed (mtype RS)
                                     (team ?team)
                                     (scored FALSE)
                                     (confirmed TRUE)
                                     (order ?order)
                                     (workpiece ?workpiece-id)
                                     (game-time ?delivery-time)
                                     (base-color ?order-base)
                                     (ring-color ?order-ring)))
        )
      )
      ;Retrigger all rings score calculation if a single one is worng
      (do-for-all-facts ((?pd product-processed)) (and (eq ?pd:workpiece ?workpiece-id)
                                                       (eq ?pd:scored TRUE)
                                                       (eq ?pd:mtype RS))
                      (modify ?pd (scored FALSE))
      )
    )
  )
  (if (neq ?workpiece-order ?order) then
     (printout t "Rectifying workpiece " ?workpiece-id ": order ID corrected [" ?workpiece-order
                 "->" ?order "]"  crlf)
  )
  (do-for-fact ((?pd product-processed)) (and (eq ?pd:workpiece ?workpiece-id)
                                              (eq ?pd:scored TRUE)
                                              (eq ?pd:mtype DS)
                                              (neq ?pd:id ?id))
     (printout t "Rectifying workpiece " ?workpiece-id ": removing old delivery operation for order " ?pd:order  crlf)
     (retract ?pd)
  )
  (if (duplicate ?wf (start-time ?gt)
                 (order ?order)
                 (base-color ?order-base)
                 (cap-color ?order-cap)
                 (team ?team)
                 (ring-colors ?order-rings))
   then
     (modify ?wf (latest-data FALSE) (end-time ?gt))
   else
     (modify ?wf (end-time ?gt))
  )
  (assert (rectified ?workpiece-id))
)

(defrule order-delivery-confirmation-referee-confirmed-workpiece-verified
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?rf <- (referee-confirmation (process-id ?id) (state CONFIRMED))
  ?pf <- (product-processed (id ?id) (team ?team) (order ?order) (confirmed FALSE)
                            (workpiece ?wp-id) (game-time ?delivery-time))
  ?wf <- (workpiece (id ?wp-id)
                    (latest-data TRUE)
                    (order ?order)
                    (cap-color ?cap-color)
                    (base-color ?base-color)
                    (ring-colors $?ring-colors))
  ?of <- (order (id ?order)
                (active TRUE)
                (cap-color ?cap-color)
                (base-color ?base-color)
                (ring-colors $?ring-colors))
   =>
  (printout t "Workpiece " ?wp-id  " verified for order " ?order crlf)
  (modify ?pf (confirmed TRUE))
  (retract ?rf)
)

(defrule order-delivered-correct
	?gf <- (gamestate (phase PRODUCTION|POST_GAME))
	?pf <- (product-processed (game-time ?delivery-time) (team ?team)
	                          (order ?id) (delivery-gate ?gate)
	                          (workpiece ?wp-id) (id ?p-id)
	                          (scored FALSE) (mtype DS)
	                          (confirmed TRUE))
	(not (product-processed (game-time ?other-delivery&:(< ?other-delivery ?delivery-time))
	                        (scored FALSE) (mtype DS) (order ?o-id&:(> ?o-id 0))))
  (workpiece (id ?wp-id) (order ?id) (latest-data TRUE))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity) (competitive ?competitive)
	        (delivery-gate ?dgate&:(or (eq ?gate 0) (eq ?gate ?dgate)))
	        (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
	        (quantity-requested ?q-req) (quantity-delivered $?q-del)
	        (delivery-period $?dp &:(>= ?delivery-time (nth$ 1 ?dp))))
	=>
  (modify ?pf (scored TRUE))
	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))

  (modify ?of (quantity-delivered ?q-del-new))
	(bind ?points 0)
	(if (< (nth$ ?q-del-idx ?q-del) ?q-req) then
		; Delivery points
		(switch ?complexity
		  (case C0 then (bind ?points ?*PRODUCTION-POINTS-DELIVER-C0*))
		  (case C1 then (bind ?points ?*PRODUCTION-POINTS-DELIVER-C1*))
		  (case C2 then (bind ?points ?*PRODUCTION-POINTS-DELIVER-C2*))
		  (case C3 then (bind ?points ?*PRODUCTION-POINTS-DELIVER-C3*))
		)
		(assert (points (game-time ?delivery-time) (team ?team) (phase PRODUCTION)
			                (points ?points) (order ?id) (product-step ?p-id) (reason "Delivered item for order")))
		(bind ?delivery-overtime (- ?delivery-time (nth$ 2 ?dp)))
		(bind ?overtime-percentile 0.0)
		(bind ?delivery-length (- (nth$ 2 ?dp) (nth$ 1 ?dp)))
		(while  (and (< ?overtime-percentile 1.0)
		             (> ?delivery-overtime (* ?overtime-percentile ?delivery-length))
		             (> ?*PRODUCTION-POINTS-DELIVER-LATE-POINTS-STEPS* 0)
		        )
			(bind ?overtime-percentile (+ ?overtime-percentile
			      (/ 1 ?*PRODUCTION-POINTS-DELIVER-LATE-POINTS-STEPS*)))
		)
		(if (> ?overtime-percentile 0.0)
		 then
			(assert (points (game-time ?delivery-time) (order ?id) (team ?team) (phase PRODUCTION)
			  (points (- 0 (integer (* ?overtime-percentile
			                  (* ?*PRODUCTION-POINTS-DELIVER-LATE-POINTS-DEDUCTION-REL* ?points)))))
			  (product-step ?p-id) (reason (str-cat "Late delivery of order (>" ?overtime-percentile "% )"))))
		)
		(if ?competitive
		 then
			(if (> (nth$ (order-q-del-other-index ?team) ?q-del) (nth$ ?q-del-idx ?q-del))
			 then
				; the other team delivered first
				(bind ?deduction (min ?points ?*PRODUCTION-POINTS-COMPETITIVE-SECOND-DEDUCTION*))
				(assert (points (game-time ?delivery-time) (order ?id) (team ?team) (phase PRODUCTION)
				                (points (* -1 ?deduction)) (product-step ?p-id)
				                (reason (str-cat "Second delivery for competitive order " ?id))))
			 else
				; this team delivered first
				(assert (points (game-time ?delivery-time) (order ?id) (team ?team) (phase PRODUCTION)
				                (points ?*PRODUCTION-POINTS-COMPETITIVE-FIRST-BONUS*)
				                (product-step ?p-id)
				                (reason (str-cat "First delivery for competitive order " ?id))))
			)
		)
  )
)

(defrule order-delivered-wrong-delivgate
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id)  (workpiece ?wp-id) (order ?o-id)
                            (scored FALSE) (confirmed TRUE)
                            (delivery-gate ?gate))
  (workpiece (id ?wp-id) (order ?o-id) (latest-data TRUE))
  ; the actual order we are delivering
  (order (id ?o-id) (active TRUE) (delivery-gate ?dgate&~?gate&:(neq ?gate 0)))
	=>
  (modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (wrong delivery gate, got " ?gate ", expected " ?dgate ")" crlf)
)

(defrule order-delivered-wrong-too-soon
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id) (order ?o-id) (workpiece ?wp-id)
                            (scored FALSE) (confirmed TRUE))
  (workpiece (id ?wp-id) (order ?o-id) (latest-data TRUE))
  ; the actual order we are delivering
  (order (id ?o-id) (active TRUE) (delivery-period $?dp&:(< ?game-time (nth$ 1 ?dp))))
	=>
	(modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (too soon, before time window)" crlf)

	(assert (points (game-time ?game-time) (order ?o-id) (points 0)
									(team ?team) (phase PRODUCTION) (product-step ?p-id)
									(reason (str-cat "Delivered item for order " ?o-id
																	 " (too soon, before time window)"))))
)

(defrule order-delivered-wrong-too-many
  ?gf <- (gamestate (phase PRODUCTION|POST_GAME))
  ?pf <- (product-processed (game-time ?game-time) (team ?team) (mtype DS)
                            (id ?p-id) (order ?o-id) (workpiece ?wp-id)
                            (scored FALSE) (confirmed TRUE))
  (workpiece (id ?wp-id) (order ?o-id) (latest-data TRUE))
  ; the actual order we are delivering
  ?of <- (order (id ?o-id) (active TRUE) (quantity-requested ?q-req)
								(quantity-delivered $?q-del&:(>= (order-q-del-team ?q-del ?team) ?q-req)))
  =>
  (modify ?pf (scored TRUE))
	(printout warn "Delivered item for order " ?o-id " (too many)" crlf)

	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))
  (modify ?of (quantity-delivered ?q-del-new))

	(assert (points (game-time ?game-time) (order ?o-id) (points ?*PRODUCTION-POINTS-DELIVERY-WRONG*)
									(team ?team) (phase PRODUCTION) (product-step ?p-id)
									(reason (str-cat "Delivered item for order " ?o-id
																	 " (too many)"))))
)

(defrule order-print-points
  (points (game-time ?gt) (points ?points) (team ?team) (phase ?phase) (reason ?reason))
  =>
  (printout t "Giving " ?points " points to team " ?team ": " ?reason
	    " (" ?phase " @ " ?gt ")" crlf)
)

;-------------------------------------Score for Intermediate
(deffunction order-step-scoring-allowed (?order-id ?team ?q-req ?mtype ?base ?ring ?cap)
   (bind ?scored-for-step
           (find-all-facts ((?p product-processed)) (and (eq ?p:scored TRUE)
                                                       (eq ?p:order ?order-id)
                                                       (eq ?p:team ?team)
                                                       (eq ?p:mtype ?mtype)
                                                       (eq ?p:base-color ?base)
                                                       (eq ?p:ring-color ?ring)
                                                       (eq ?p:cap-color ?cap)
                                                       (eq ?p:confirmed TRUE))))
  (return (> ?q-req (length$ ?scored-for-step)))
)

(defrule order-step-mount-ring
 "Production points for mounting a ring on an intermediate product "
  ?pf <- (product-processed (id ?p-id)
                            (mtype RS)
                            (team ?team)
                            (scored FALSE)
                            (confirmed TRUE)
                            (workpiece ?w-id)
                            (game-time ?g-time)
                            (at-machine ?m-name)
                            (base-color ?step-b-color)
                            (cap-color ?step-c-color)
                            (ring-color ?step-r-color&~nil))
  (workpiece (id ?w-id)
             (latest-data TRUE)
             (team ?team)
             (order ?o-id)
             (base-color ?base-color)
             (ring-colors $?wp-r-colors&:(member$ ?step-r-color
                                                  ?wp-r-colors)))
  (order (id ?o-id)
         (complexity ?complexity)
         (quantity-requested ?q-req)
         (delivery-period $?dp)
         (base-color ?base-color)
         (ring-colors $?r-colors&:(eq ?wp-r-colors
                                      (subseq$ ?r-colors 1 (length$ ?wp-r-colors)))))
  (ring-spec (color ?step-r-color) (req-bases ?cc))
  (not (points (product-step ?p-id)))
  (test (order-step-scoring-allowed ?o-id ?team ?q-req RS ?step-b-color ?step-r-color ?step-c-color))
   =>
  ; Production points for ring color complexities
  (bind ?points 0)
  (bind ?reason (str-cat "Mounted CC" ?cc " ring of CC" ?cc
                                     " for order " ?o-id))
  (switch ?cc
      (case 0 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC0-STEP*))
      (case 1 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC1-STEP*))
      (case 2 then (bind ?points ?*PRODUCTION-POINTS-FINISH-CC2-STEP*)))
  (if (> ?g-time (nth$ 2 ?dp)) then
    (if (config-get-bool "/llsfrb/workpiece-tracking/enable") then (bind ?points 0))
    (bind ?reason (str-cat ?reason " Late (deadline: " (nth$ 2 ?dp) ")"))
  )
  (assert (points (phase PRODUCTION) (order ?o-id) (game-time ?g-time) (team ?team)
                  (points ?points) (product-step ?p-id)
                  (reason ?reason)))
  (modify ?pf (scored TRUE) (order ?o-id))
)

(defrule order-step-mount-cap
  "Production points for mounting a cap on an intermediate product "
  ?pf <- (product-processed (id ?p-id)
                            (mtype CS)
                            (team ?team)
                            (scored FALSE)
                            (confirmed TRUE)
                            (workpiece ?w-id)
                            (game-time ?g-time)
                            (at-machine ?m-name)
                            (base-color ?step-b-color)
                            (ring-color ?step-r-color)
                            (cap-color ?step-c-color&~nil))
  (workpiece (id ?w-id)
             (latest-data TRUE)
             (team ?team)
             (order ?o-id)
             (base-color ?base-color)
             (ring-colors $?ring-colors)
             (cap-color ?cap-color&:(eq ?cap-color ?step-c-color)))
  (order (id ?o-id)
         (complexity ?complexity)
         (quantity-requested ?q-req)
         (delivery-period $?dp)
         (base-color ?base-color)
         (ring-colors $?ring-colors)
         (cap-color ?cap-color))
  (not (points (product-step ?p-id)))
  (test (order-step-scoring-allowed ?o-id ?team ?q-req CS ?step-b-color ?step-r-color ?step-c-color))
  =>
  ; Production points for mounting the cap
  (bind ?reason (str-cat "Mounted cap for order " ?o-id))
  (bind ?points 0)
  (bind ?points ?*PRODUCTION-POINTS-MOUNT-CAP*)
  (if (> ?g-time (nth$ 2 ?dp)) then
    (bind ?reason (str-cat ?reason " Late (deadline: " (nth$ 2 ?dp) ")"))
    (if (config-get-bool "/llsfrb/workpiece-tracking/enable") then (bind ?points 0))
  )
  (assert (points (game-time ?g-time) (order ?o-id) (team ?team)
                  (points ?points)
                  (phase PRODUCTION) (product-step ?p-id)
                  (reason ?reason)))
  (modify ?pf (scored TRUE) (order ?o-id))
)


;-------------------------------fault handling
(defrule order-remove-operations-scored-with-no-workpiece
    "When a workpiece is removed, remove related operations"
    ?pf <- (product-processed (id ?id) (workpiece ?w-id&~0)
                              (confirmed TRUE) (scored TRUE)
                              (mtype ~DS) (team ?team))
    (not (workpiece (id ?w-id) (latest-data TRUE)))
    =>
    (retract ?pf)
)

(defrule order-remove-points-of-invalid-operation
   "When operations are removed or scored reset, remove points given for that operation"
   ?pf <- (points (product-step ?id&~0) (points ?points) (reason ?reason))
   (not (product-processed (id ?id) (scored TRUE)))
   =>
   (printout t " Removing " ?points  " points of: " ?reason crlf)
   (retract ?pf)
)

(defrule order-inconsistent-operation-with-workpiece
"Intermediate scored operation became inconsistent its workpiece. This should
 never happen. Operations should always be inline with their workpiece."
    ?pf <- (product-processed (id ?p-id)
                              (team ?team)
                              (mtype ?mtype)
                              (scored TRUE)
                              (confirmed TRUE)
                              (workpiece ?w-id)
                              (game-time ?operation-time)
                              (at-machine ?operation-machine)
                              (base-color ?operation-base)
                              (ring-color ?operation-ring)
                              (cap-color ?operation-cap))
    (workpiece (id ?w-id)
               (latest-data TRUE)
               (team ?team)
               (order ?order)
               (base-color ?base-color)
               (ring-colors $?ring-colors)
               (cap-color ?cap-color))
    (or (and (eq ?mtype BS) (neq ?operation-base ?base-color))
        (and (eq ?mtype CS) (neq ?operation-cap ?cap-color))
        (and (eq ?mtype RS) (not (member$ ?operation-ring $?ring-colors)))
    )
    =>
    (printout warn  "Inconsistent operation at " ?operation-machine
                    " scored for workpiece " ?w-id " " crlf)
)

(defrule order-inconsistent-operation-order
"Retrigger score calculation for operations scored for a different order than
 the one the workpiece is currently tracking."
    ?pf <- (product-processed (id ?p-id)
                              (team ?team)
                              (mtype ?mtype)
                              (scored TRUE)
                              (confirmed TRUE)
                              (workpiece ?w-id)
                              (at-machine ?operation-machine)
                              (base-color ?operation-base)
                              (ring-color ?operation-ring)
                              (cap-color ?operation-cap)
                              (order ?operation-order))
    (workpiece (id ?w-id)
               (latest-data TRUE)
               (team ?team)
               (order ?order)
               (cap-color ?cap-color)
               (base-color ?base-color)
               (ring-colors $?ring-colors))
    (order (id ?order) (active TRUE))
    (test (or (and (eq ?mtype CS) (eq ?operation-cap ?cap-color))
              (and (eq ?mtype RS) (member$ ?operation-ring $?ring-colors))))
    (test (neq ?operation-order ?order))
    =>
    (modify ?pf (order ?order) (scored FALSE))
    (printout warn "Workpiece " ?w-id  " currently tracks order " ?order
                    " instead of " ?operation-order
                     ", Canceling operation at " ?operation-machine  crlf)
)

