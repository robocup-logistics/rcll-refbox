
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

(deffunction order-q-del-team (?q-del ?team)
	(return (nth$ (order-q-del-index ?team) ?q-del))
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

	(do-for-fact ((?m machine)) (and (eq ?m:mtype DS) (eq ?m:team ?team))
		;(printout t "Product delivered: " ?gt " " ?team " " ?m:ds-last-gate " "
		;					?base-color " " ?ring-colors " " ?cap-color crlf)
    (assert (product-delivered (game-time ?gt) (team ?team) (delivery-gate ?m:ds-last-gate)
															 (base-color ?base-color) (ring-colors ?ring-colors) (cap-color ?cap-color)))
	)
)

(defrule order-delivered-correct-by-id
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (delivery-gate ?gate)
														(order ?id&~0))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity)
								(delivery-gate ?dgate&:(or (eq ?gate 0) (eq ?gate ?dgate)))
								(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
								(points ?order-points) (points-supernumerous ?order-points-supernumerous)
								(quantity-requested ?q-req) (quantity-delivered $?q-del)
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

(defrule order-delivered-by-color-invalid
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (order 0)
														(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
	(not (order (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)))
	=>
	(retract ?pf)
	(assert (attention-message (team ?team)
														 (text (str-cat "Invalid Order delivered: " ?base-color " "
																						?ring-colors " " ?cap-color))))
)

(defrule order-delivered-correct-by-color
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (delivery-gate ?gate) (order 0)
														(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity)
								(delivery-gate ?dgate&:(or (eq ?gate 0) (eq ?gate ?dgate)))
								(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
								(points ?order-points) (points-supernumerous ?order-points-supernumerous)
								(quantity-requested ?q-req)
								(quantity-delivered $?q-del&:(< (order-q-del-team ?q-del ?team) ?q-req))
								(delivery-period $?dp&:(>= ?gt (nth$ 1 ?dp))&:(<= ?gt (nth$ 2 ?dp))))


  ; There is no other order, which would score more points
  (not (and
    ; in-time, proper gate, open (q-del < q-req)
		(order (id ?oid2&:(<> ?oid2 ?id)) (active TRUE)
					 (delivery-gate ?dgate2&:(or (eq ?gate 0) (eq ?gate ?dgate2)))
					 (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
					 (delivery-period $?dp2&:(>= ?gt (nth$ 1 ?dp2))&:(<= ?gt (nth$ 2 ?dp2)))
					 (quantity-requested ?q-req-2)
					 (quantity-delivered $?q-del-2&:(< (order-q-del-team ?q-del-2 ?team) ?q-req-2)))

		(or
		 (test (> (- ?q-req-2 (order-q-del-team ?q-del-2 ?team)) (- ?q-req (order-q-del-team ?q-del ?team))))
		 (test (and
						(= (- ?q-req-2 (order-q-del-team ?q-del-2 ?team)) (- ?q-req (order-q-del-team ?q-del ?team)))
					  (< (nth$ 2 ?dp2) (nth$ 2 ?dp))))
		)
	))

	?sf <- (signal (type order-info))

	=>
  (retract ?pf)

	; Cause immediate sending of order info
	(modify ?sf (time (create$ 0 0)) (count 0))

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

(defrule order-delivered-by-color-late
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (delivery-gate ?gate) (order 0)
														(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
  ; the actual order we are delivering
  ?of <- (order (id ?id) (active TRUE) (complexity ?complexity) (delivery-gate ?gate)
								(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
								(quantity-requested ?q-req)
								(quantity-delivered $?q-del&:(< (order-q-del-team ?q-del ?team) ?q-req))
								(delivery-period $?dp&:(> ?gt (nth$ 2 ?dp))))

	?sf <- (signal (type order-info))
	=>
  (retract ?pf)

	; Cause immediate sending of order info
	(modify ?sf (time (create$ 0 0)) (count 0))

	(bind ?q-del-idx (order-q-del-index ?team))
  (bind ?q-del-new (replace$ ?q-del ?q-del-idx ?q-del-idx (+ (nth$ ?q-del-idx ?q-del) 1)))
  (modify ?of (quantity-delivered ?q-del-new))

  (if (< (- ?gt (nth$ 2 ?dp)) ?*DELIVER-MAX-LATENESS-TIME*)
   then
    ; 15 - floor(T_d - T_e) * 1.5 + 5
	  (bind ?points (+ (- 15 (* (floor (- ?gt ?game-time)) 1.5)) 5))
		(assert (points (game-time ?game-time) (points ?points)
										(team ?team) (phase PRODUCTION)
										(reason (str-cat "Delivered item for order " ?id
																		" (late delivery grace time)"))))
   else
	  (assert (points (game-time ?game-time) (points ?*POINTS-DELIVER-TOO-LATE*)
										(team ?team) (phase PRODUCTION)
										(reason (str-cat "Delivered item for order " ?id
																		 " (too late delivery)")))) 
  )
)

(defrule order-delivered-wrong
  ?gf <- (gamestate (state RUNNING) (phase PRODUCTION) (game-time ?gt))
  ?pf <- (product-delivered (game-time ?game-time) (team ?team) (delivery-gate ?gate) (order 0)
														(base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color))
  ; the actual order we are delivering
  (order (id ?id) (active TRUE) (complexity ?complexity)
				 (base-color ?base-color) (ring-colors $?ring-colors) (cap-color ?cap-color)
				 (quantity-requested ?q-req))
	(or (order (id ?id) (delivery-gate ?dgate&~?gate))
			(order (id ?id) (quantity-delivered $?q-del&:(>= (order-q-del-team ?q-del ?team) ?q-req)))
			(order (id ?id) (delivery-period $?dp&:(> ?gt (nth$ 2 ?dp)))))
  =>
  (retract ?pf)
	(printout warn "Delivered item for order " ?id " (wrong delivery)" crlf)

	(assert (points (game-time ?game-time) (points ?*POINTS-DELIVER-WRONG*)
									(team ?team) (phase PRODUCTION)
									(reason (str-cat "Delivered item for order " ?id
																	 " (wrong delivery)"))))
)

(defrule order-print-points
  (points (game-time ?gt) (points ?points) (team ?team) (phase ?phase) (reason ?reason))
  =>
  (printout t "Awarding " ?points " points to team " ?team ": " ?reason
	    " (" ?phase " @ " ?gt ")" crlf)
)
