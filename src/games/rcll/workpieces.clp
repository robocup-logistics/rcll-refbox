
;---------------------------------------------------------------------------
;  workpieces.clp - LLSF RefBox CLIPS workpiece rules
;
;  Created: Tue Jun 28 12:14:37 2016
;  Copyright  2013-2016  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------


(defrule workpiece-learn-new
	"Learn a new workpiece we had not seen before"
	(gamestate (phase PRODUCTION))
	(workpiece (rtype INCOMING) (id ?id) (at-machine ?at-machine) (visible ?visible))
	(not (workpiece (rtype RECORD) (id ?id)))
	=>
	(assert (workpiece (rtype RECORD) (id ?id) (team (machine-team ?at-machine))
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

(defrule workpiece-incoming-cleanup
	"Remove transient incoming facts"
	(declare (salience ?*PRIORITY_CLEANUP*))
	?wf <- (workpiece (rtype INCOMING))
	=>
	(retract ?wf)
)
