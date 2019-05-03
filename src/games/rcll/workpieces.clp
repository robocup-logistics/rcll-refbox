
;---------------------------------------------------------------------------
;  workpieces.clp - LLSF RefBox CLIPS workpiece rules
;
;  Created: Tue Jun 28 12:14:37 2016
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------


(deffunction workpiece-base-color-by-id (?id)
	(if (and (>= ?id (nth$ 1 ?*WORKPIECE-RANGE-RED*)) (<= ?id (nth$ 2 ?*WORKPIECE-RANGE-RED*)))
		then (return BASE_RED))
	(if (and (>= ?id (nth$ 1 ?*WORKPIECE-RANGE-BLACK*)) (<= ?id (nth$ 2 ?*WORKPIECE-RANGE-BLACK*)))
		then (return BASE_BLACK))
	(if (and (>= ?id (nth$ 1 ?*WORKPIECE-RANGE-SILVER*)) (<= ?id (nth$ 2 ?*WORKPIECE-RANGE-SILVER*)))
		then (return BASE_SILVER))
	(if (and (>= ?id (nth$ 1 ?*WORKPIECE-RANGE-CLEAR*)) (<= ?id (nth$ 2 ?*WORKPIECE-RANGE-CLEAR*)))
			then (return BASE_CLEAR))
	(printout error "Invalid workpiece ID " ?id ". Cannot determine base color." crlf)
	(return BASE_INVALID)
)

(deffunction workpiece-gen-id-for-base-color (?base-color)
	(if (eq ?base-color BASE_RED)
		then (return (random (nth$ 1 ?*WORKPIECE-RANGE-RED*) (nth$ 2 ?*WORKPIECE-RANGE-RED*))))
	(if (eq ?base-color BASE_SILVER)
		then (return (random (nth$ 1 ?*WORKPIECE-RANGE-SILVER*) (nth$ 2  ?*WORKPIECE-RANGE-SILVER*))))
	(if (eq ?base-color BASE_BLACK)
		then (return (random (nth$ 1 ?*WORKPIECE-RANGE-BLACK*) (nth$ 2 ?*WORKPIECE-RANGE-BLACK*))))
	(if (eq ?base-color BASE_CLEAR)
		then (return (random (nth$ 1 ?*WORKPIECE-RANGE-CLEAR*) (nth$ 2  ?*WORKPIECE-RANGE-CLEAR*))))
  (printout error "Invalid workpiece base-color" ?base-color  ". assign random id." crlf)
	(return (random (nth$ 1 ?*WORKPIECE-RANGE-RED*) (nth$ 2 ?*WORKPIECE-RANGE-CLEAR*)))
)

(deffunction workpiece-simulate-tracking (?order-id ?team ?delivery-time)
  (do-for-fact ((?order order)) (eq ?order:id ?order-id)
      ;Create a workpiece
      (do-for-fact ((?m machine)) (and (eq ?m:mtype DS) (eq ?m:team ?team))
         (bind ?workpiece-id (workpiece-gen-id-for-base-color ?order:base-color))
         (assert (workpiece (team ?team)
                            (rtype RECORD)
                            (state AVAILABLE)
                            (id ?workpiece-id)
                            (order ?order-id)
                            (at-machine ?m:name)
                            (base-color ?order:base-color)
                            (ring-colors ?order:ring-colors)
                            (cap-color ?order:cap-color))))

      ;Simulate Base retrieval process
      (do-for-fact ((?m machine)) (and (eq ?m:mtype BS) (eq ?m:team ?team))
         (assert (product-processed (mtype BS)
                                    (team ?team)
                                    (confirmed TRUE)
                                    (at-machine ?m:name)
                                    (workpiece ?workpiece-id)
                                    (game-time ?delivery-time)
                                    (base-color ?order:base-color))))
      ;Simulate Ring mouting processes
      (foreach ?r ?order:ring-colors
        (do-for-fact ((?m machine)) (and (eq ?m:mtype RS)(eq ?m:team ?team)
                                         (member$ ?r ?m:rs-ring-colors))
           (assert (product-processed (mtype RS)
                                      (team ?team)
                                      (confirmed TRUE)
                                      (at-machine ?m:name)
                                      (workpiece ?workpiece-id)
                                      (game-time ?delivery-time)
                                      (ring-color ?r)))))
      ;Simulate Cap mounting process on a random cap station
      (do-for-fact ((?m machine)) (and (eq ?m:mtype CS) (eq ?m:team ?team))
         (assert (product-processed (mtype CS)
                                    (team ?team)
                                    (confirmed TRUE)
                                    (at-machine ?m:name)
                                    (workpiece ?workpiece-id)
                                    (game-time ?delivery-time)
                                    (cap-color ?order:cap-color))))


   )
)

(defrule workpiece-learn-new
	"Learn a new workpiece we had not seen before"
	(gamestate (phase PRODUCTION))
	(workpiece (rtype INCOMING) (id ?id) (at-machine ?at-machine) (visible ?visible))
	(not (workpiece (rtype RECORD) (id ?id)))
	=>
	(assert (workpiece (rtype RECORD) (id ?id) (team (machine-team ?at-machine))
										 (base-color (workpiece-base-color-by-id ?id))
										 (at-machine ?at-machine) (visible ?visible)))
)

(defrule workpiece-not-in-production
	"workpiece update received at wrong time"
	(gamestate (phase ~PRODUCTION))
	(workpiece (rtype INCOMING) (id ?id) (at-machine ?at-machine) (visible ?visible))
	=>
	(printout warn "Received workpiece update for " ?id " while not in production" crlf)
)

(defrule workpiece-update
	"Update a workpiece if information changes"
	(gamestate (phase PRODUCTION))
	(workpiece (rtype INCOMING) (id ?id) (at-machine ?at-machine) (visible ?visible))
	?wf <- (workpiece (rtype RECORD) (id ?id) (at-machine ?r-at-machine) (visible ?r-visible))
	(test (or (neq ?at-machine ?r-at-machine) (neq ?visible ?r-visible)))
	=>
	(modify ?wf (at-machine ?at-machine) (visible ?visible))
)

(defrule workpiece-assign-order
	"Assign order to workpiece.
   This has a very specific assumption about the ring colors. We assume that
   the color of the first ring fully determines the complexity class. Furthermore
   we assume that there is only a single order of complexity >0 each. Hence, knowing
   the first ring will immediately determine the order of the workpiece."
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (rtype RECORD) (id ?id) (order 0) (team ?r-team)
										(base-color ?base-color) (cap-color ?cap-color)
										; we can only determine the order if a ring has been mounted
										(ring-colors $?ring-colors&:(> (length$ ?ring-colors) 0)))
	(order (id ?order-id) (base-color ?base-color)
				 (ring-colors $?order-ring-colors&:(eq (first$ ?order-ring-colors) (first$ ?ring-colors))))
	=>
	(modify ?wf (order ?order-id))
	(printout t "Workpiece " ?id " received a ring, determined order to be " ?order-id crlf)
)

(defrule workpiece-resign-order
	"Resign order from workpiece, if they became incosistent"
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (rtype RECORD) (id ?id) (order ?o-id&:(neq ?o-id 0)) (team ?r-team)
										(base-color ?base-color) (cap-color ?cap-color)
										; we can only determine the order if a ring has been mounted
										(ring-colors $?ring-colors&:(> (length$ ?ring-colors) 0)))
	(order (id ?o-id) (base-color ?base-color)
          (ring-colors $?order-ring-colors&:(neq ?ring-colors
                                                (subseq$ ?order-ring-colors 1 (length$ ?ring-colors)))))
	=>
	(modify ?wf (order 0))
	(printout t "Workpiece " ?id " received a ring, became incosistent with " ?o-id crlf)
)

;-------------------------Workpiece Availability------------------------------
(defrule workpiece-available
	"Workpiece seen at machine"
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (rtype RECORD) (id ?id) (at-machine ?m-name)
                    (state ?state&~AVAILABLE) (visible TRUE))
	(machine (name ?m-name) (state ~BROKEN))
	=>
  (printout t "Workpiece " ?id ": Visible at " ?m-name crlf)
  (modify ?wf (state AVAILABLE))
)

(defrule workpiece-retrieved
	"Workpiece no longer at machine"
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (rtype RECORD) (id ?id) (at-machine ?m-name)
                    (state AVAILABLE) (visible FALSE))
	(machine (name ?m-name) (state IDLE|WAIT-IDLE|BROKEN))
	=>
  (printout t "Workpiece " ?id ": Retrieved from " ?m-name crlf)
  (modify ?wf (state RETRIEVED))
)


;--------------------------Workpiece Processing-------------------------------
(defrule workpiece-at-bs
	"Confirm workpiece base_color when visible at BS
   ps. The workpiece should have been created already"
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (id ?id) (rtype RECORD) (state AVAILABLE)
                    (at-machine ?m-name) (base-color ?base-color))
  ?pf <- (product-processed (at-machine ?m-name) (mtype BS)
                            (workpiece ?wp-id) (confirmed FALSE)
                            (base-color ?bs-color))
	=>
  (printout t "Workpiece " ?id ": Confirming process " ?m-name crlf)
  (if (neq ?bs-color ?base-color)
   then
	  (printout t "Workpiece " ?id ": Base color corrected [" ?base-color
                "->" ?bs-color "]" crlf)
  )
  (modify ?pf (workpiece ?id) (confirmed TRUE))
  (modify ?wf (base-color ?bs-color))
)

(defrule workpiece-processed-at-rs
	"Update the availbe workpiece with the recent prodcut processing  "
	(gamestate (phase PRODUCTION))
	?wf <- (workpiece (id ?id) (rtype RECORD) (state AVAILABLE)
                    (at-machine ?m-name) (ring-colors $?ring-colors))
	?pf <- (product-processed (at-machine ?m-name) (mtype RS)
                            (workpiece ?wp-id) (confirmed FALSE)
                            (ring-color ?r-color))
	=>
  (printout t "Workpiece " ?id ": Confirming process at  " ?m-name crlf)
  (modify ?pf (workpiece ?id) (confirmed TRUE))
  (modify ?wf (ring-colors (append$ ?ring-colors ?r-color)))
)

;(defrule workpiece-processed-at-cs
;	""
;	(gamestate (phase PRODUCTION))
;	?wf <- (workpiece (rtype RECORD) (id ?wf-id) (at-machine ?m-name) (state AVAILABLE))
;	?p <- (product-processed (id ?p-id) (workpiece-id ?workpiece-id)
;                           (at-machine ?m-name) (mtype CS) )
;  (not (eq ?workpiece-id ?wf-id))
;	=>
;  (printout t "Workpiece " ?id ": Processed at " ?m-name crlf)
;   ;Add the proper operation
;   (modify ?wf (state PROCESSED))
;)


;------------------------------Sanity Checks


(defrule workpiece-available-twice
 "Error differnt workpieces availble at same  machines"
	(gamestate (phase PRODUCTION))
	?wf1 <- (workpiece (rtype RECORD) (id ?first-id)
                     (at-machine ?at-machine)
                     (state AVAILABLE)
  ?wf2 <- (workpiece (rtype RECORD)
                     (id ?second-id&:(neq ?first-id ?second-id))
                     (at-machine ?at-machine)
                     (state AVAILABLE)
 =>
	(printout t "Workpiece " ?first-id " and " ?second-id
               "are both AVAILABE at " ?at-machine crlf)

)

;TODO: if mounting ring with no base


;----------------------------------------Clean Up Incoming
(defrule workpiece-incoming-cleanup
	"Remove transient incoming facts"
	(declare (salience ?*PRIORITY_CLEANUP*))
	?wf <- (workpiece (rtype INCOMING))
	=>
	(retract ?wf)
)
