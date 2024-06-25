
; Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

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
	(if (and (>= ?id (nth$ 1 ?*WORKPIECE-RANGE-UNKNOWN*)) (<= ?id (nth$ 2 ?*WORKPIECE-RANGE-UNKNOWN*)))
			then (return BASE_UNKNOWN))
	(printout t "Invalid workpiece ID " ?id ". Cannot determine base color." crlf)
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
	(if (or (eq ?base-color BASE_UNKNOWN) (eq ?base-color nil))
		then (return (random (nth$ 1 ?*WORKPIECE-RANGE-UNKNOWN*) (nth$ 2  ?*WORKPIECE-RANGE-UNKNOWN*))))
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
                            (latest-data TRUE)
                            (holding FALSE)
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
                                    (game-time 0.1)
                                    (base-color ?order:base-color))))
      ;Simulate Ring mouting processes
      (foreach ?r ?order:ring-colors
        (do-for-fact ((?m machine)) (and (eq ?m:mtype RS)(eq ?m:team ?team)
                                         (member$ ?r ?m:available-colors))
           (assert (product-processed (mtype RS)
                                      (team ?team)
                                      (confirmed TRUE)
                                      (at-machine ?m:name)
                                      (workpiece ?workpiece-id)
                                      (game-time 0.1)
                                      (ring-color ?r)))))
      ;Simulate Cap mounting process on a random cap station
      (do-for-fact ((?m machine)) (and (eq ?m:mtype CS) (eq ?m:team ?team))
         (assert (product-processed (mtype CS)
                                    (team ?team)
                                    (confirmed TRUE)
                                    (at-machine ?m:name)
                                    (workpiece ?workpiece-id)
                                    (game-time 0.1)
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
  (confval (path "/llsfrb/workpiece-tracking/broadcast") (type BOOL) (value ?send-info))
  (not (workpiece-tracking))
  =>
  (if (eq ?tracking TRUE) then (bind ?enabled TRUE) else (bind ?enabled FALSE))
  (if (eq ?disable-on-failure TRUE) then (bind ?fail-safe TRUE) else (bind ?fail-safe FALSE))
  (if (eq ?send-info TRUE) then (bind ?broadcast TRUE) else (bind ?broadcast FALSE))
  (assert (workpiece-tracking (enabled ?enabled) (fail-safe ?fail-safe) (reason "by config") (broadcast ?broadcast)))
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
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    ?mf <- (mps-read-barcode ?m-name ?id)
    (machine (name ?m-name) (state ~BROKEN) (team ?team))
    =>
    (retract ?mf)
    (printout t "Workpiece " ?id ": at " ?m-name ", available!"crlf)
    (if (not (do-for-fact ((?workpiece workpiece)) (and (eq ?workpiece:id ?id)
                                                        (eq ?workpiece:latest-data TRUE))
          ;Update existing
          (duplicate ?workpiece (unknown-action FALSE) (start-time ?gt) (at-machine ?m-name)
                                (state AVAILABLE) (visible ?gt) (holding FALSE) (team ?team))
          (modify ?workpiece (latest-data FALSE) (end-time ?gt))
        )
      )
    then
      ;Find unidentified workpiece at input
      (if (not (do-for-fact ((?workpiece workpiece)) (and (eq ?workpiece:at-machine ?m-name)
                                                          (eq ?workpiece:at-side INPUT)
                                                          (eq ?workpiece:latest-data TRUE))
            (duplicate ?workpiece (unknown-action FALSE) (id ?id) (start-time ?gt) (at-machine ?m-name)
                                  (state AVAILABLE) (visible ?gt) (holding FALSE) (team ?team))
            (modify ?workpiece (latest-data FALSE) (id ?id) (end-time ?gt))
            ;Update all workpiece facts with same workpiece-name without id
            (do-for-all-facts ((?wp workpiece)) (and (eq ?wp:name ?workpiece:name)
                                                     (eq ?wp:id 0))
              (modify ?workpiece (id ?id))
            )
          )
        )
      then
        ;Else create a new workpiece fact
        (assert (workpiece (at-machine ?m-name) (at-side INPUT) (holding FALSE)
                           (state AVAILABLE) (visible ?gt) (id ?id) (name (sym-cat WP- (gensym*)))
                           (latest-data TRUE) (start-time ?gt) (unknown-action FALSE)
                           (team ?team) (base-color (workpiece-base-color-by-id ?id))))
      )
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
    (workpiece (id ?id) (at-machine ?m-name) (holding FALSE) (state AVAILABLE) (latest-data TRUE))
    (time-info (game-time ?gt))
    =>
    (do-for-all-facts ((?workpiece workpiece)) (and (eq ?workpiece:at-machine ?m-name)
                                               (eq ?workpiece:latest-data TRUE)
                                               (eq ?workpiece:holding FALSE)
                                               (eq ?workpiece:state AVAILABLE))
      (printout t "Workpiece " ?workpiece:id ": at " ?m-name ", retrieved!" crlf)
      (duplicate ?workpiece (start-time ?gt) (state RETRIEVED))
      (modify ?workpiece (latest-data FALSE) (end-time ?gt))
    )
)

(defrule workpiece-mps-waiting-in-processed
  "Print a message if the MPS is in state PROCESSED but has not seen a workpiece yet."
  (machine (name ?n) (state PROCESSED))
  (workpiece-tracking (enabled TRUE))
  (not (workpiece (at-machine ?n) (latest-data TRUE) (state AVAILABLE)))
  =>
  (printout warn "Machine " ?n " is waiting to detect a workpiece" crlf)
)

;-----------------------------Order assignment---------------------------------
(defrule workpiece-assign-order
   "Assign order to workpiece.
   This has a very specific assumption about the ring colors. We assume that
   the color of the first ring fully determines the complexity class. Furthermore
   we assume that there is only a single order of complexity >0 each. Hence, knowing
   the first ring will immediately determine the order of the workpiece."
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id) (order 0) (team ?r-team)
                      (latest-data TRUE)
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
    (duplicate ?wf (start-time ?gt) (order ?order-id))
    (modify ?wf (latest-data FALSE) (end-time ?gt))
    (printout t "Workpiece " ?id ": order assigned " ?order-id crlf)
)

(defrule workpiece-resign-order
    "Resign order from workpiece, if they became inconsistent"
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (latest-data TRUE)
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
    (duplicate ?wf (start-time ?gt) (order 0))
    (modify ?wf (latest-data FALSE) (end-time ?gt))
    (printout t "Workpiece " ?id ": order resigned " ?order-id  crlf)
)

;--------------------------Workpiece Processing-------------------------------
(defrule workpiece-at-bs
    "When workpiece available at BS, confirm generated base-color against
    prepared base-color."
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (latest-data TRUE)
                      (state AVAILABLE)
                      (at-machine ?m-name)
                      (holding FALSE)
                      (base-color ?base-color))
    ?pf <- (product-processed (mtype BS)
                              (confirmed FALSE)
                              (workpiece ?wp-id)
                              (at-machine ?m-name)
                              (base-color ?current-base-color))
    =>
    (printout t "Workpiece " ?id ": at " ?m-name ", processed"crlf)
    (if (neq ?current-base-color ?base-color)
      then (printout t "Workpiece correction ["
                       ?base-color  "->" ?current-base-color "]" crlf)
    )
    (duplicate ?wf (start-time ?gt) (base-color ?current-base-color))
    (modify ?wf (latest-data FALSE) (end-time ?gt))
    (modify ?pf (workpiece ?id) (confirmed TRUE))
)

(defrule workpiece-processed-at-rs
    "Update workpiece available at an RS with the recent production operation.
    Link the production operation to the workpiece"
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (latest-data TRUE)
                      (state AVAILABLE)
                      (holding FALSE)
                      (at-machine ?m-name)
                      (ring-colors $?ring-colors))
    ?pf <- (product-processed (mtype RS)
                              (confirmed FALSE)
                              (workpiece ?wp-id)
                              (at-machine ?m-name)
                              (ring-color ?r-color))
    =>
    (printout t "Workpiece " ?id ": at " ?m-name ", processed" crlf)
    (duplicate ?wf (start-time ?gt) (ring-colors (append$ ?ring-colors ?r-color)) (at-side OUTPUT))
    (modify ?wf (latest-data FALSE) (end-time ?gt))
    (modify ?pf (workpiece ?id) (confirmed TRUE))
)

(defrule workpiece-processed-at-cs
    "Update the available workpiece at CS with the recent production operation.
    Like the production operation to the workpiece."
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (latest-data TRUE)
                      (state AVAILABLE)
                      (holding FALSE)
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
    (duplicate ?wf (start-time ?gt) (cap-color ?c-color) (at-side OUTPUT))
    (modify ?wf (latest-data FALSE) (end-time ?gt))
    (modify ?pf (workpiece ?id))
    ;Cap-color info needs to be confirmed by the referee (latest on delivery)
)

(defrule workpiece-processed-at-ds
    "Update the available workpiece with the recent production operation.
    Link the delivery to the available workpiece"
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf <- (workpiece (id ?id)
                      (latest-data TRUE)
                      (state AVAILABLE)
                      (holding FALSE)
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
   (duplicate ?wf (start-time ?gt) (at-side nil))
   (modify ?wf (latest-data FALSE) (end-time ?gt))
   (modify ?pf (workpiece ?id))
)

;------------------------------Sanity Checks
(defrule workpiece-available-twice
    "Error different workpieces available at same  machines"
    (gamestate (phase PRODUCTION))
    (time-info (game-time ?gt))
    (workpiece-tracking (enabled TRUE))
    ?wf1 <- (workpiece (id ?first-id)
                       (latest-data TRUE)
                       (visible ?first-last-seen)
                       (holding FALSE)
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    ?wf2 <- (workpiece (id ?second-id&:(neq ?first-id ?second-id))
                       (latest-data TRUE)
                       (visible ?second-last-seen&:(>= ?second-last-seen ?first-last-seen))
                       (holding FALSE)
                       (at-machine ?at-machine)
                       (state AVAILABLE))
    =>
    (duplicate ?wf1 (start-time ?gt) (state RETRIEVED))
    (modify ?wf1 (latest-data FALSE) (end-time ?gt))
    (printout warn "Workpiece " ?second-id " detected at " ?at-machine
                   " while workpiece " ?first-id " is still available..retrieving the oldest!"crlf)
)

(defrule workpiece-failed-read-at-BS-RS
    "Processed operation finished at a machine where workpiece was not identified"
    ?wf <- (workpiece-tracking (enabled TRUE) (fail-safe ?fail-safe))
    ?pf <- (product-processed (at-machine ?at-machine) (mtype ?mtype&~DS) (workpiece 0))
    (machine (name ?at-machine) (mtype ?mtype) (state ?state&WAIT-IDLE|BROKEN))
    =>
    (retract ?pf)
    (printout warn "Operation at " ?at-machine " not linked to a workpiece!" crlf)
    (printout debug "Barcode scanner at " ?at-machine " failed to read workpiece" crlf)
    (if (and ?fail-safe
             (neq ?state BROKEN)
             (neq ?mtype CS)
             (neq ?mtype DS))
     then
     ; In case of failure at BS|RS, disabling tracking is recommended.
     ; Recovering from CS|DS failure could be attempted upon confirmation.
     (modify ?wf (enabled FALSE) (reason (str-cat " Fail-safe reader at " ?at-machine)))
    )
)

(defrule workpiece-unique-order-prefixs
   "Warn if 'unique order prefix' assumption is violated. Assumption is
   necessary for sound order to workpiece assignment.
   Check workpiece-assig-order rule for details"
   (order (id ?o1-id) (complexity ~C0) (ring-colors $?o1-ring-colors) (active TRUE))
   (order (id ?o2-id&:(and (neq ?o2-id ?o1-id) (< ?o1-id ?o2-id)))
          (complexity ~C0) (ring-colors $?o2-ring-colors) (active TRUE))
   (workpiece-tracking (enabled TRUE))
   (test (eq (first$ ?o1-ring-colors) (first$ ?o2-ring-colors)))
   =>
  (printout warn "Violated uniqueness assumption 'cx-order prefix', might influence tracking"  crlf)
  (printout warn " Orders " ?o1-id " & " ?o2-id " have the same first ring color "
                 (first$ ?o1-ring-colors) crlf)
)
