
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
      (bind ?workpiece-id (workpiece-gen-id-for-base-color ?order:base-color))
      (do-for-fact ((?m machine)) (and (eq ?m:mtype DS) (eq ?m:team ?team))
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
     ; (do-for-fact ((?m machine)) (and (eq ?m:mtype DS) (eq ?m:team ?team))
     ;    (assert (product-processed (mtype DS)
     ;                               (team ?team)
     ;                               (confirmed TRUE)
     ;                               (order ?order:id)
     ;                               (at-machine ?m:name)
     ;                               (workpiece ?workpiece-id)
     ;                               (game-time ?delivery-time)
     ;                               (delivery-gate ?order:delivery-gate))))

    return ?workpiece-id
   )
  return 0
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
                    (cap-color ?cap-color)
                    (base-color ?base-color)
                    (ring-colors $?ring-colors))
    (order (id ?order-id)
           (base-color ?base-color)
           (ring-colors $?order-ring-colors)
           (cap-color ?order-cap-color))
    (test (or (eq ?cap-color nil)
              (eq ?cap-color ?order-cap-color)))
    (test (or (and (eq (length$ ?ring-colors) 0)
                   (eq (length$ ?order-ring-colors) 0))
              (and (> (length$ ?ring-colors) 0)
                   (eq ?ring-colors (subseq$ ?order-ring-colors 1 (length$ ?ring-colors))))))
    =>
    (modify ?wf (order ?order-id))
    (printout t "Workpiece " ?id " received a ring, determined order to be " ?order-id crlf)
)

(defrule workpiece-resign-order
    "Resign order from workpiece, if they became incosistent"
    (gamestate (phase PRODUCTION))
    ?wf <- (workpiece (id ?id)
                      (rtype RECORD)
                      (order ?order-id)
                      (team ?r-team)
                      (base-color ?base-color)
                      (ring-colors $?ring-colors)
                      (cap-color ?cap-color))
    (order (id ?order-id)
           (base-color ?order-base-color)
           (ring-colors $?order-ring-colors)
           (cap-color ?order-cap-color))
    (test (or (neq ?base-color ?order-base-color)
              (and (> (length$ ?ring-colors) 0)
                   (neq ?ring-colors (subseq$ ?order-ring-colors 1 (length$ ?ring-colors))))
              (and (neq ?cap-color nil)
                   (neq ?cap-color ?order-cap-color))))
    =>
    (modify ?wf (order 0))
    (printout t "Workpiece [" ?id "] became incosistent with tracked order  ID-" ?order-id  crlf)
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
    ?wf <- (workpiece (id ?id) 
                      (rtype RECORD)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (base-color ?base-color))
  ?pf <- (product-processed (mtype BS)
                            (confirmed FALSE)
                            (workpiece ?wp-id)
                            (at-machine ?m-name)
                            (base-color ?bs-color))
   =>
  (printout t "Workpiece " ?id ": Confirming process " ?m-name crlf)
  (if (neq ?bs-color ?base-color)
    then (printout t "Workpiece " ?id ": Base color corrected ["
                     ?base-color  "->" ?bs-color "]" crlf)
  )
  (modify ?pf (workpiece ?id) (confirmed TRUE))
  (modify ?wf (base-color ?bs-color))
)

(defrule workpiece-processed-at-rs
    "Update the availbe workpiece with the recent prodcut processing  "
    (gamestate (phase PRODUCTION))
    ?wf <- (workpiece (id ?id)
                      (rtype RECORD)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (ring-colors $?ring-colors))
    ?pf <- (product-processed (mtype RS)
                              (confirmed FALSE)
                              (workpiece ?wp-id)
                              (at-machine ?m-name)
                              (ring-color ?r-color))
    =>
    (printout t "Workpiece " ?id ": Confirming process " ?m-name crlf)
    ;TODO: Find out what points needs to be given
    (modify ?pf (workpiece ?id) (confirmed TRUE))
    (modify ?wf (ring-colors (append$ ?ring-colors ?r-color)))
)

(defrule workpiece-processed-at-cs
    "Update the available workpiece with the recent product processing  "
    (gamestate (phase PRODUCTION))
    ?wf <- (workpiece (id ?id)
                      (rtype RECORD)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (cap-color nil))
 ?pf <- (product-processed (mtype CS)
                           (workpiece 0)
                           (confirmed FALSE)
                           (at-machine ?m-name)
                           (cap-color ?c-color))
  (not (product-processed (workpiece ?id) (mtype CS) (confirmed TRUE)))
  =>
  (printout t "Workpiece " ?id ": Confirming process at  " ?m-name crlf)
  (modify ?pf (workpiece ?id))
  (modify ?wf (cap-color ?c-color))
  ;The cap-color will most probably be nil at the moment of processing cause
  ;the info is not present yet. The color will be confirmed by the referee
  ;(latest on delivery confirmation)
)

(defrule workpiece-processed-at-ds
    "Update the availbe workpiece with the recent prodcut processing  "
    (gamestate (phase PRODUCTION))
    ?wf <- (workpiece (id ?id)
                      (rtype RECORD)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      ;This will be CONFIRMED by the referee later on.
                      (order ?unconfirmed-order-id))
    ?pf <- (product-processed (mtype DS)
                              (workpiece 0)
                              (confirmed FALSE)
                              (order ?order-id))
    ; The process that WILL be done on that WP anyways.
    =>
    (printout t "Workpiece [" ?id "] Confirming process at " ?m-name crlf)
    (if (neq ?order-id ?unconfirmed-order-id)
      then
      (printout t "Workpiece [" ?id "]: Tracked for order with  ID-" ?unconfirmed-order-id
                " is inconistent with the requested delivery for order with ID-" ?order-id
                " This inconsistency will be resolved upon delivery confirmation" crlf)
   )
   (modify ?pf (workpiece ?id))
)

;------------------------------Sanity Checks
(defrule workpiece-make-sure-wp-tracks-requested-order
    ""
    (gamestate (phase PRODUCTION))
    ?wf <- (workpiece (id ?wp-id)
                      (rtype RECORD)
                      (at-machine ?m-name)
                      (order ?tracked-order-id)
                      (base-color ?tracked-base-color)
                      (ring-colors ?tracked-ring-colors)
                      (cap-color ?tracked-cap-color))
    ?pf <- (product-processed (mtype DS) 
                              (confirmed TRUE)
                              (workpiece ?wp-id)
                              (at-machine ?m-name)
                              (order ?deliver-order-id&:(neq ?deliver-order-id ?tracked-order-id)))
  (order (id ?req-order-id)
         (base-color ?tracked-base-color)
         (ring-colors ?tracked-ring-colors)
         (cap-color ?tracked-cap-color))
  (order (id ?tracked-order-id&:(neq ?tracked-order-id ?req-order-id)))
  =>
  (printout t "Workpiece [" ?wp-id "] Order IDs corrected [" ?tracked-order-id
              "->" ?deliver-order-id "]"  crlf)
  (modify ?wf (order ?deliver-order-id))
)

(defrule workpiece-available-twice
    "Error differnt workpieces availble at same  machines"
    (gamestate (phase PRODUCTION))
    ?wf1 <- (workpiece (rtype RECORD) (id ?first-id)
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    ?wf2 <- (workpiece (rtype RECORD)
                       (id ?second-id&:(neq ?first-id ?second-id))
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    =>
    (printout t "Workpiece " ?first-id " and " ?second-id
                "are both AVAILABE at " ?at-machine crlf)
)


;----------------------------------------Clean Up Incoming
(defrule workpiece-incoming-cleanup
	"Remove transient incoming facts"
	(declare (salience ?*PRIORITY_CLEANUP*))
	?wf <- (workpiece (rtype INCOMING))
	=>
	(retract ?wf)
)
