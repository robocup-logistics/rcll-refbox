
;---------------------------------------------------------------------------
;  orders.clp - LLSF RefBox CLIPS order processing
;
;  Created: Sun Feb 24 19:40:32 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction order-q-del-index (?team)
  (if (eq ?team CYAN) then (return 1) else (return 2))
)

(defrule activate-order
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
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
  (declare (salience ?*PRIORITY_HIGH*))
  ?oa <- (order (id ?id-a))
  ?ob <- (order (id ?id-b&:(> ?id-a ?id-b)&:(< (fact-index ?oa) (fact-index ?ob))))
  =>
  (modify ?oa)
)

(defrule order-recv-SetOrderDelivered
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetOrderDelivered") (ptr ?p) (rcvd-via STREAM)
		       (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?id (pb-field-value ?p "order_id"))
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (if (not (any-factp ((?o order)) (= ?o:id ?id)))
   then
    (printout error "Received SetOrderDelivered for non-existing order " ?id crlf)
   else
    (do-for-fact ((?o order)) (= ?o:id ?id)
      (assert (product-delivered (game-time ?gt) (order ?id) (team ?team)))
    )
  )
)


(defrule order-recv-SetOrderDeliveredByColor
  (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (protobuf-msg (type "llsf_msgs.SetOrderDeliveredByColor") (ptr ?p) (rcvd-via STREAM)
											 (rcvd-from ?from-host ?from-port) (client-id ?cid))
  =>
  (bind ?team (sym-cat (pb-field-value ?p "team_color")))
  (bind ?base-color  (sym-cat (pb-field-value ?p "base_color")))
  (bind ?ring-colors (pb-field-list ?p "ring_colors"))
  (bind ?cap-color   (sym-cat (pb-field-value ?p "cap_color")))

  (assert (order-delivered-by-color (team-color ?team) (game-time ?gt)
																		(base-color ?base-color)
																		(ring-colors ?ring-colors) (cap-color ?cap-color)))
)

(defrule order-match-by-color-valid
	?of <- (order-delivered-by-color (team-color ?team) (game-time ?gt)
																	 (base-color ?base-color)
																	 (ring-colors $?ring-colors) (cap-color ?cap-color))
	(order (id ?id) (active TRUE)
				 (delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp)))
				 (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
	(not (order (id ?id2&~?id) (active TRUE)
							(delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2))&:(< (nth$ 1 ?dp2) (nth$ 1 ?dp)))))
	=>
	(retract ?of)
	(printout t "Received valid order " ?id crlf)
	(assert (product-delivered (game-time ?gt) (order ?id) (team ?team)))
)

(defrule order-match-by-color-valid-late
	?of <- (order-delivered-by-color (team-color ?team) (game-time ?gt)
																	 (base-color ?base-color)
																	 (ring-colors $?ring-colors) (cap-color ?cap-color))
	(order (id ?id) (active TRUE)
				 (delivery-period $?dp&:(> ?gt (nth$ 2 ?dp)))
				 (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
	(not (order (id ?id2&~?id) (active TRUE)
							(delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))))
	=>
	(retract ?of)
	(printout t "Received valid late order " ?id crlf)
	(assert (product-delivered (game-time ?gt) (order ?id) (team ?team)))
)

(defrule order-match-by-color-invalid
	?of <- (order-delivered-by-color (team-color ?team) (game-time ?gt)
																	 (base-color ?base-color)
																	 (ring-colors $?ring-colors) (cap-color ?cap-color))
	(not (order (id ?id) (active TRUE)
							(delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp)))
							(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)))
	=>
	(retract ?of)
	(printout warn "Product delivered which was not requested" crlf)
)



(defrule order-delivered-in-time
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (order ?id) (team ?team))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity) (ring-colors $?ring-colors)
		(points ?order-points) (points-supernumerous ?order-points-supernumerous)
		(quantity-requested ?q-req) (quantity-delivered $?q-del) (delivery-gate ?dg)
		(delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp))))
  =>
  (retract ?pf)
	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))

  (modify ?of (quantity-delivered ?q-del-new))
  (if (< (nth$ ?q-del-idx ?q-del) ?q-req)
   then
    (assert (points (game-time ?game-time) (team ?team) (phase PRODUCTION)
		    (points ?order-points) 
		    (reason (str-cat "Delivered item for order " ?id))))

    (foreach ?r ?ring-colors
      (do-for-fact ((?rs ring-spec)) (eq ?rs:color ?r)
        (assert (points (game-time ?game-time)
			(points (* ?rs:req-bases ?*POINTS-PER-ADDITIONAL-BASE*))
			(team ?team) (phase PRODUCTION)
			(reason (str-cat "Mounted ring of CC" ?rs:req-bases
					 " for order " ?id))))
      )
    )
    (bind ?complexity-num (length$ ?ring-colors))
    (if (> ?complexity-num 0) then
      (assert (points (game-time ?game-time) (points (* ?complexity-num ?*POINTS-PER-RING*))
		      (team ?team) (phase PRODUCTION)
		      (reason (str-cat "Mounted last ring for complexity "
				       ?complexity " order " ?id))))
    )
    
    (assert (points (game-time ?game-time) (points ?*POINTS-MOUNT-CAP*)
		    (team ?team) (phase PRODUCTION)
		    (reason (str-cat "Mounted cap for order " ?id))))

   else
    (assert (points (game-time ?game-time) (team ?team) (phase PRODUCTION)
		    (points ?order-points-supernumerous) 
		    (reason (str-cat "Delivered item for order " ?id))))
  )
)

(defrule order-delivered-out-of-time
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (order ?id) (team ?team))
  ; the actual order we are delivering
  ?of <- (order (active TRUE) (id ?id)
		(quantity-delivered $?q-del) (quantity-requested ?q-req)
		(delivery-period $?dp2&:(or (< ?gt (nth$ 1 ?dp2)) (> ?gt (nth$ 2 ?dp2)))))
  =>
  (retract ?pf)

	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))
  (modify ?of (quantity-delivered ?q-del-new))

  (if (> (nth$ ?q-del-idx ?q-del-new)  ?q-req)
   then
    (assert (points (game-time ?game-time) (points ?*POINTS-DELIVER-OUT-OF-TIME*)
		    (team ?team) (phase PRODUCTION)
		    (reason (str-cat "Delivered item for order " ?id
				     " (order already fulfilled)"))))
   else
    (if (< ?gt (nth$ 1 ?dp2))
     then
      (assert (points (game-time ?game-time) (points ?*POINTS-DELIVER-OUT-OF-TIME*)
		      (team ?team) (phase PRODUCTION)
		      (reason (str-cat "Delivered item for order " ?id
				       " (ahead of time window)"))))
     else
      (if (< (- ?gt (nth$ 2 ?dp2)) ?*DELIVER-MAX-LATENESS-TIME*)
       then
        ; 20 - floor(T_d - T_e) * 2
        (bind ?points-per-sec (/ ?*POINTS-DELIVER* ?*DELIVER-MAX-LATENESS-TIME*))
        (bind ?lateness (- ?*POINTS-DELIVER* (* (floor (- ?gt ?game-time)) ?points-per-sec)))
      )
    ) 
  )
)

(defrule order-print-points
  (points (game-time ?gt) (points ?points) (team ?team) (phase ?phase) (reason ?reason))
  =>
  (printout t "Awarding " ?points " points to team " ?team ": " ?reason
	    " (" ?phase " @ " ?gt ")" crlf)
)

; (defrule order-delivered-in-time
;   ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
;   ?pf <- (product-delivered (game-time ?game-time) (production-time ?prod-time)
; 			    (team ?t) (product ?p) (delivery-gate ?dg))
;   ; the actual order we are delivering
;   ?of <- (order (id ?oid) (active TRUE) (product ?p) (quantity-requested ?q-req) (team ?t)
; 		(points ?order-points) (points-supernumerous ?order-points-supernumerous)
; 		(quantity-delivered ?q-del) (delivery-gate ?odg&?dg|ANY)
; 		(delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp))))

;     ; There is no other order, which would score more points
;   (not (or
;     ; give more points, is active, is in-time, and still goods requested
;     (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p) (team ?t)
; 	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
; 	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
; 	   (points ?op2&:(> ?op2 ?order-points))
; 	   (quantity-requested ?q-req-2&:(< ?q-del-2 ?q-req-2)))
;     ; open, in-time, goods remaining and starts sooner
;     (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p) (team ?t)
; 	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2))&:(< (nth$ 1 ?dp2) (nth$ 1 ?dp)))
; 	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
; 	   (quantity-requested ?q-req-2&:(< ?q-del-2 ?q-req-2)))
;     ; give more points than supernumerous points, is active, is in-time, and that has
;     ; goods requested while the chosen order has not
;     (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p) (team ?t)
; 	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
; 	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
; 	   (points-supernumerous ?op2&:(> ?order-points ?op2))
; 	   (quantity-requested ?q-req-2&:(< ?q-del-2 ?q-req-2 )&:(>= ?q-del ?q-req)))
;     ; give more supernumerous points, is active, is in-time, and that has
;     ; all products delivered while the chosen order has as well
;     (order (id ?oid2&:(<> ?oid ?oid2)) (active TRUE) (product ?p) (team ?t)
; 	   (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
; 	   (delivery-gate ?odg2&?dg|ANY) (quantity-delivered ?q-del-2)
; 	   (points-supernumerous ?op2&:(> ?op2 ?order-points-supernumerous))
; 	   (quantity-requested ?q-req-2&:(>= ?q-del-2 ?q-req-2 )&:(>= ?q-del ?q-req)))
;   ))
;   =>
;   (retract ?pf)
;   (modify ?of (quantity-delivered (+ ?q-del 1)))
;   (if (< ?q-del ?q-req)
;    then
;     (bind ?addp ?order-points)
;     (if (time-in-range ?prod-time ?dp)
;      then
;      (printout t "Product " ?p " produced in delivery time slot. Awarding "
; 	       ?*PRODUCED-IN-DELIVER-TIME-POINTS* " extra points" crlf)
;      (assert (points (game-time ?game-time) (points ?*PRODUCED-IN-DELIVER-TIME-POINTS*)
; 		     (team ?t) (phase PRODUCTION)
; 		     (reason (str-cat "Produced " ?p " in delivery time (order "
; 				      ?oid " , time " ?prod-time ")"))))
;     )
;    else
;     (bind ?addp ?order-points-supernumerous)
;   )
;   (printout t "Product " ?p " delivered at " ?dg ". Awarding " ?addp " points" crlf)
;   (assert (points (game-time ?game-time) (points ?addp) (team ?t) (phase PRODUCTION)
;                   (reason (str-cat "Delivered " ?p " to " ?dg))))
; )

; (defrule order-delivered-out-of-time
;   ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
;   ?pf <- (product-delivered (game-time ?game-time) (team ?t) (product ?p) (delivery-gate ?dg))
;   ; the actual order we are delivering
;   (not (order (active TRUE) (product ?p)
; 	      (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))))
;   =>
;   (retract ?pf)
;   (bind ?addp ?*DELIVER-WITH-NO-ACTIVE-ORDER*)
;   (printout t "Product " ?p " delivered at " ?dg ". Awarding " ?addp
; 	    " points (no active order)" crlf)
;   (assert (points (game-time ?game-time) (points ?addp) (team ?t) (phase PRODUCTION)
;                   (reason (str-cat "Delivered " ?p " to " ?dg " (no active order)"))))
; )
