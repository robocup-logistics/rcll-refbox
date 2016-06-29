
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
	(workpiece (rtype INCOMING) (id ?id) (team ?team) (at-machine ?at-machine) (visible ?visible))
	?wf <- (workpiece (rtype RECORD) (id ?id) (team ?r-team) (at-machine ?r-at-machine) (visible ?r-visible))
	(test (or (neq ?team ?r-team) (neq ?at-machine ?r-at-machine) (neq ?visible ?r-visible)))
	=>
	(modify ?wf (team ?team) (at-machine ?at-machine) (visible ?visible))
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


(defrule workpiece-incoming-cleanup
	"Remove transient incoming facts"
	(declare (salience ?*PRIORITY_CLEANUP*))
	?wf <- (workpiece (rtype INCOMING))
	=>
	(retract ?wf)
)
