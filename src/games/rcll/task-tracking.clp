
;---------------------------------------------------------------------------
;  task-tracking.clp - LLSF RefBox CLIPS task tracking rules
;
;  Created: Tue Jun 07 12:44:01 2022
;  Copyright  2022       Matteo Tschesche
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defrule agent-task-move-received
   "Warn if 'unique order prefix' assumption is violated. Assumption is
   necessary for sound order to workpiece assignment.
   Check workpiece-assig-order rule for details"
   ?a <- (agent-task (task-type MOVE) (robot-id ?robot-id) (processed FALSE)
                     (team-color ?team-color)
                     (task-parameters waypoint ?waypoint
                                      machine-point ?machine-point))
   =>
  (printout warn "Move received"  crlf)
  (modify ?a (processed TRUE))
)

(defrule agent-task-retrieve-received
   "Warn if 'unique order prefix' assumption is violated. Assumption is
   necessary for sound order to workpiece assignment.
   Check workpiece-assig-order rule for details"
   ?a <- (agent-task (task-type RETRIEVE) (robot-id ?robot-id) (processed FALSE)
                     (team-color ?team-color)
                     (task-parameters machine-id ?machine-id
                                      machine-point ?machine-point))
   =>
  (printout warn "Retrieve received"  crlf)
  (modify ?a (processed TRUE))
)
