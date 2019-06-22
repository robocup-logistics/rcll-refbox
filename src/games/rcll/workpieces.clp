
;---------------------------------------------------------------------------
;  workpieces.clp - LLSF RefBox CLIPS workpiece rules
;
;  Created: Tue Jun 28 12:14:37 2016
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;             2019       Mostafa Gomaa
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
  (bind ?workpiece-id 0)
  (do-for-fact ((?order order)) (eq ?order:id ?order-id)
      ;Create a workpiece
      (bind ?workpiece-id (workpiece-gen-id-for-base-color ?order:base-color))
      (do-for-fact ((?m machine)) (and (eq ?m:mtype DS) (eq ?m:team ?team))
         (assert (workpiece (team ?team)
                            (state IDLE)
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

   )
  return ?workpiece-id
)

(defrule workpiece-tracking-state-from-config
  (confval (path "/llsfrb/workpiece-tracking/enable") (type BOOL) (value ?tracking))
  (confval (path "/llsfrb/workpiece-tracking/fail-safe") (type BOOL) (value ?disable-on-failure))
  (not (workpiece-tracking))
  =>
  (if (eq ?tracking true) then (bind ?enabled TRUE) else (bind ?enabled FALSE))
  (if (eq ?disable-on-failure true) then (bind ?fail-safe TRUE) else (bind ?fail-safe FALSE))
  (assert (workpiece-tracking (enabled ?enabled) (fail-safe ?fail-safe) (reason "by config")))
)

(defrule workpiece-tracking-print
  (workpiece-tracking (enabled ?enabled) (reason ?reason))
  =>
  (bind ?state "disabled")
  (if ?enabled then (bind ?state "enabled"))
  (printout warn "Workpiece tracking " ?state ", " ?reason crlf)
)

;-------------------------Workpiece Availability------------------------------
(defrule workpiece-update-available
    "Workpiece available on a usable machine while tracking is enabled.
    Update workpiece by read event information (mps-status-feedback)"
    (workpiece-tracking (enabled TRUE))
    (gamestate (phase PRODUCTION) (game-time ?gt))
    ?mf <- (mps-status-feedback ?m-name BARCODE ?id)
    (machine (name ?m-name) (state ~BROKEN) (team ?team))
    =>
    (retract ?mf)
    (printout t "Workpiece " ?id ": at " ?m-name ", available!"crlf)
    (if (any-factp ((?wp workpiece)) (eq ?wp:id ?id)) then
       ;Update existing
       (do-for-fact ((?workpiece workpiece)) (eq ?workpiece:id ?id)
         (modify ?workpiece (at-machine ?m-name) (state AVAILABLE) (visible ?gt)))
      else
        ;Learn new
        (assert (workpiece (at-machine ?m-name) (state AVAILABLE) (visible ?gt) (id ?id)
                           (team ?team) (base-color (workpiece-base-color-by-id ?id))))
    )
)

(defrule workpiece-update-not-in-production
    "workpiece update received at wrong time"
    (gamestate (phase ~PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?mf <- (mps-status-feedback ?machine BARCODE ?id)
	=>
    (retract ?mf)
    (printout warn "Received workpiece update for " ?id " while not in production" crlf)
)

(defrule workpiece-update-machine-broken
    "workpiece update received at while machine is broken"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?mf <- (mps-status-feedback ?machine BARCODE ?id)
    (machine (name ?machine) (state BROKEN))
	=>
    (retract ?mf)
    (printout warn "Received workpiece update for " ?id " while " ?machine "  BROKEN" crlf)
)

(defrule workpiece-update-tracking-disabled
    "workpiece update received but tracking is disabled"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled FALSE))
    ?mf <- (mps-status-feedback ?machine BARCODE ?id)
	=>
    (retract ?mf)
    (printout warn "Received workpiece update for " ?id " but workpiece tracking is disabled" crlf)
)

(defrule workpiece-update-retrieved
    "Workpiece no longer at machine"
    (workpiece-tracking (enabled TRUE))
    (machine (name ?m-name) (state WAIT-IDLE|BROKEN))
    (workpiece (id ?id) (at-machine ?m-name) (state AVAILABLE))
    =>
    (do-for-all-facts ((?workpiece workpiece)) (and (eq ?workpiece:at-machine ?m-name)
                                               (eq ?workpiece:state AVAILABLE))
      (printout t "Workpiece " ?workpiece:id ": at " ?m-name ", retrieved!" crlf)
      (modify ?workpiece (state RETRIEVED))
    )
)

;-----------------------------Order assignment---------------------------------
(defrule workpiece-assign-order
   "Assign order to workpiece.
   This has a very specific assumption about the ring colors. We assume that
   the color of the first ring fully determines the complexity class. Furthermore
   we assume that there is only a single order of complexity >0 each. Hence, knowing
   the first ring will immediately determine the order of the workpiece."
   (gamestate (phase PRODUCTION))
   (workpiece-tracking (enabled TRUE))
   ?wf <- (workpiece (id ?id) (order 0) (team ?r-team)
                    (cap-color ?cap-color)
                    (base-color ?base-color)
                    (ring-colors $?ring-colors))
    (order (id ?order-id)
           (active TRUE)
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
    (printout t "Workpiece " ?id ": order assigned " ?order-id crlf)
)

(defrule workpiece-resign-order
    "Resign order from workpiece, if they became inconsistent"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
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
    (printout t "Workpiece " ?id ": order resigned " ?order-id  crlf)
)

;--------------------------Workpiece Processing-------------------------------
(defrule workpiece-at-bs
    "When workpiece available at BS, confirm generated base-color against
    prepared base-color."
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id) 
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (base-color ?base-color))
  ?pf <- (product-processed (mtype BS)
                            (confirmed FALSE)
                            (workpiece ?wp-id)
                            (at-machine ?m-name)
                            (base-color ?bs-color))
  =>
  (printout t "Workpiece " ?id ": at " ?m-name ", processed"crlf)
  (if (neq ?bs-color ?base-color)
    then (printout t "Workpiece correction ["
                     ?base-color  "->" ?bs-color "]" crlf)
  )
  (modify ?pf (workpiece ?id) (confirmed TRUE))
  (modify ?wf (base-color ?bs-color))
)

(defrule workpiece-processed-at-rs
    "Update workpiece available at an RS with the recent production operation.
    Link the production operation to the workpiece"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (ring-colors $?ring-colors))
    ?pf <- (product-processed (mtype RS)
                              (confirmed FALSE)
                              (workpiece ?wp-id)
                              (at-machine ?m-name)
                              (ring-color ?r-color))
    =>
    (printout t "Workpiece " ?id ": at " ?m-name ", processed" crlf)
    (modify ?pf (workpiece ?id) (confirmed TRUE))
    (modify ?wf (ring-colors (append$ ?ring-colors ?r-color)))
)

(defrule workpiece-processed-at-cs
    "Update the available workpiece at CS with the recent production operation.
    Like the production operation to the workpiece."
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
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
  (printout t "Workpiece " ?id ": at  " ?m-name ", processed" crlf)
  (modify ?pf (workpiece ?id))
  (modify ?wf (cap-color ?c-color))
  ;Cap-color info needs to be confirmed by the referee (latest on delivery)
)

(defrule workpiece-processed-at-ds
    "Update the available workpiece with the recent production operation.
    Link the delivery to the available workpiece"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (order ?tracked-order-id))
    ?pf <- (product-processed (mtype DS)
                              (workpiece 0)
                              (confirmed FALSE)
                              (at-machine ?m-name)
                              (order ?order-id))
    =>
    (printout t "Workpiece " ?id ": at " ?m-name ", processed" crlf)
    (if (neq ?order-id ?tracked-order-id)
      then
      (printout t "Workpiece " ?id ": Delivery requested for order " ?order-id
                " not the tracked order " ?tracked-order-id crlf)
      (printout t "Conflict will be resolved upon referee confirmation!" crlf)
   )
   (modify ?pf (workpiece ?id))
)

;------------------------------Sanity Checks
(defrule workpiece-available-twice
    "Error different workpieces available at same  machines"
    (gamestate (phase PRODUCTION))
    (workpiece-tracking (enabled TRUE))
    ?wf1 <- (workpiece (id ?first-id)
                       (visible ?first-last-seen)
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    ?wf2 <- (workpiece (id ?second-id&:(neq ?first-id ?second-id))
                       (visible ?second-last-seen&:(>= ?second-last-seen ?first-last-seen))
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    =>
    (modify ?wf1 (state RETRIEVED))
    (printout warn "Workpiece " ?second-id " detected at " ?at-machine
                   " while workpiece " ?first-id " is still available..retrieving the oldest!"crlf)
)

(defrule workpiece-non-available-at-machine
    "Processed operation finished at a machine where workpiece was identified"
    (workpiece-tracking (enabled TRUE))
    ?pf <- (product-processed (at-machine ?at-machine) (mtype ~DS) (workpiece 0))
    (machine (name ?at-machine) (mtype ?mtype)(state WAIT-IDLE|BROKEN))
    =>
    (retract ?pf)
    (printout warn "Operation at " ?at-machine " could not be linked to a workpiece!" crlf)
    (printout debug "Barcode scanner might be broken at " ?at-machine
                    " no workpiece was recognized" crlf)
    ;TODO: if its a BS or RS, disable tracking as implications are potentially
    ;      too severe.
    ;      Recovering from CS and DS failure could be attempted

(defrule workpiece-tracking-disabled-remove-undelivered-workpiece
    "If tracking was disabled for any reason, remove any undelivered workpieces."
    (workpiece-tracking (enabled FALSE))
    ?wf <- (workpiece (id ?w-id))
    (not (product-processed (mtype DS) (workpiece ?w-id)))
    =>
    (retract ?wf)
    (printout t "Removing obsolete workpiece " ?w-id crlf)
)

