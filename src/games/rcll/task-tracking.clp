
;---------------------------------------------------------------------------
;  task-tracking.clp - LLSF RefBox CLIPS task tracking rules
;
;  Created: Tue Jun 07 12:44:01 2022
;  Copyright  2022       Matteo Tschesche
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

;------------------------ receiving agent-tasks ----------------------------

(defrule agent-task-move-explore-received
  "Processing MOVE action send by the robot. Updating robot next position in
  worldmodel."
  ?a <- (agent-task (task-type ?task-type&MOVE|EXPLORE_MACHINE)
                    (robot-id ?robot-id)
                    (unknown-action FALSE)
                    (processed FALSE)
                    (team-color ?team-color)
                    (task-parameters waypoint ?waypoint
                                     machine-point ?machine-point)
                    (base-color ?base-color)
                    (ring-color $?ring-color)
                    (cap-color ?cap-color))
  ?r <- (robot (number ?robot-id) (team-color ?team-color))
  (gamestate (game-time ?gt))
  =>
  (printout warn ?task-type " received" crlf)
  (bind ?wp-name nil)
  (modify ?r (next-at ?waypoint) (next-side ?machine-point))

  ; if robot is holding a workpiece, it needs to match the given colors
  (if (neq ?base-color nil) then
    (if (any-factp ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                          (eq ?wp:robot-holding ?robot-id)
                                          (eq ?wp:latest-data TRUE))) then
      ; check if colors match
      (do-for-fact ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                          (eq ?wp:robot-holding ?robot-id)
                                          (eq ?wp:latest-data TRUE)
                                          (or (neq ?wp:base-color ?base-color)
                                              (neq ?wp:ring-colors $?ring-color)
                                              (neq ?wp:cap-color ?cap-color)))
        (bind ?wp-name ?wp:name)
        ; create unknown action for changing colors
        (duplicate ?wp (latest-data FALSE)
                       (start-time ?gt)
                       (end-time ?gt)
                       (unknown-action TRUE)
                       (base-color ?base-color)
                       (ring-colors $?ring-color)
                       (cap-color ?cap-color))
        (modify ?wp (base-color ?base-color) (ring-colors $?ring-color) (cap-color ?cap-color))
      )
    else
      ; create unknown workpiece fact
      (bind ?wp-name (gensym*))
      (assert (workpiece (latest-data TRUE)
                         (unknown-action TRUE)
                         (name ?wp-name)
                         (start-time ?gt)
                         (holding TRUE)
                         (robot-holding ?robot-id)
                         (at-machine ?waypoint)
                         (at-side ?machine-point)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color)))
    )
  )
  (modify ?a (processed TRUE) (workpiece-name ?wp-name))
)

(defrule agent-task-retrieve-received
  "Processing RETRIEVE action send by the robot. Updating workpiece fact in
  worldmodel."
  ?a <- (agent-task (task-type RETRIEVE)
                    (robot-id ?robot-id)
                    (unknown-action FALSE)
                    (processed FALSE)
                    (team-color ?team-color)
                    (task-parameters machine-id ?machine-id
                                     machine-point ?machine-point)
                    (order-id ?order-id)
                    (base-color ?base-color)
                    (ring-color $?ring-color)
                    (cap-color ?cap-color))
  ?r <- (robot (number ?robot-id) (team-color ?team-color) (next-at ?next-at) (next-side ?next-side))
  (gamestate (game-time ?gt))
  =>
  (printout warn "Retrieve received" crlf)
  (bind ?wp-name nil)

  ; if robot is not at the right place, ensure it with an unknown-action
  (if (or (neq ?next-at ?machine-id)
          (neq ?next-side ?machine-point)) then
    (assert (agent-task (task-type MOVE)
                        (unknown-action TRUE)
                        (task-parameters waypoint ?machine-id machine-point ?machine-point)
                        (task-id 0)
                        (robot-id ?robot-id)
                        (team-color ?team-color)
                        (start-time ?gt)
                        (end-time ?gt)
                        (order-id ?order-id)
                        (successful TRUE)
                        (base-color ?base-color)
                        (ring-color $?ring-color)
                        (cap-color ?cap-color)))
  )

  ; check if robot is supposedly holding a workpiece and change fact
  (do-for-fact ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                      (eq ?wp:robot-holding ?robot-id)
                                      (eq ?wp:latest-data TRUE))
    (duplicate ?wp (start-time ?gt)
                   (unknown-action TRUE)
                   (holding FALSE))
    (modify ?wp (latest-data FALSE) (end-time ?gt))
  )

  ; check if a workpiece is at the target
  (if (not (do-for-fact ((?wp workpiece)) (and (eq ?wp:at-machine ?machine-id)
                                               (or (eq ?wp:at-side ?machine-point)
                                                   (and (or (eq ?machine-id C-BS)
                                                            (eq ?machine-id M-BS))
                                                        (eq ?wp:at-side nil)))
                                               (eq ?wp:holding FALSE)
                                               (eq ?wp:latest-data TRUE))
        (bind ?wp-name ?wp:name)
        ; check if colors match
        (if (and (neq ?base-color nil)
                 (or (neq ?wp:base-color ?base-color)
                     (neq ?wp:ring-colors $?ring-color)
                     (neq ?wp:cap-color ?cap-color))) then
          ; create unknown action for changing colors
          (duplicate ?wp (latest-data FALSE)
                         (start-time ?gt)
                         (end-time ?gt)
                         (unknown-action TRUE)
                         (at-side ?machine-point)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color))

          ; create holding fact
          (duplicate ?wp (latest-data TRUE)
                         (start-time ?gt)
                         (holding TRUE)
                         (robot-holding ?robot-id)
                         (unknown-action FALSE)
                         (at-side ?machine-point)
                         (order ?order-id)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color))
          (modify ?wp (latest-data FALSE) (at-side ?machine-point) (end-time ?gt))
        else
          (duplicate ?wp (start-time ?gt) (holding TRUE) (robot-holding ?robot-id) (at-side ?machine-point) (order ?order-id))
          (modify ?wp (latest-data FALSE) (at-side ?machine-point) (end-time ?gt))
        )
      )
    )
  then
    ; create workpiece with random id
    (bind ?wp-name (gensym*))
    (if (eq (length$ $?ring-color) 0) then
      (assert (workpiece (latest-data TRUE)
                         (unknown-action FALSE)
                         (name ?wp-name)
                         (start-time ?gt)
                         (holding TRUE)
                         (robot-holding ?robot-id)
                         (order ?order-id)
                         (at-machine ?machine-id)
                         (at-side ?machine-point)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color)))
    else
      ; if workpiece has a ring already, create unknown-action instead
      (assert (workpiece (latest-data TRUE)
                         (unknown-action TRUE)
                         (name ?wp-name)
                         (start-time ?gt)
                         (holding TRUE)
                         (robot-holding ?robot-id)
                         (order ?order-id)
                         (at-machine ?machine-id)
                         (at-side ?machine-point)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color)))
    )
  )
  (modify ?a (workpiece-name ?wp-name) (processed TRUE))
)


(defrule agent-task-deliver-received
  "Processing DELIVER action send by the robot. Updating workpiece fact in
  worldmodel."
  ?a <- (agent-task (task-type DELIVER)
                    (robot-id ?robot-id)
                    (unknown-action FALSE)
                    (processed FALSE)
                    (team-color ?team-color)
                    (task-parameters machine-id ?machine-id
                                     machine-point ?machine-point)
                    (order-id ?order-id)
                    (base-color ?base-color)
                    (ring-color $?ring-color)
                    (cap-color ?cap-color))
  ?r <- (robot (number ?robot-id) (team-color ?team-color) (next-at ?next-at) (next-side ?next-side))
  (gamestate (game-time ?gt))
  =>
  (printout warn "Deliver received" crlf)
  (bind ?wp-name nil)

  ; check if robot is supposedly not holding a workpiece and create fact
  (if (not (any-factp ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                             (eq ?wp:robot-holding ?robot-id)
                                             (eq ?wp:latest-data TRUE)))) then
    ; create workpiece with random id
    (bind ?wp-name (gensym*))
    (assert (workpiece (latest-data TRUE)
                       (unknown-action FALSE)
                       (name ?wp-name)
                       (start-time ?gt)
                       (holding TRUE)
                       (robot-holding ?robot-id)
                       (order ?order-id)
                       (at-machine nil)
                       (at-side nil)
                       (base-color ?base-color)
                       (ring-colors $?ring-color)
                       (cap-color ?cap-color)))
  )

  ; if robot is not at the right place, ensure it with an unknown-action
  (if (or (neq ?next-at ?machine-id)
          (neq ?next-side ?machine-point)) then
    (assert (agent-task (task-type MOVE)
                        (unknown-action TRUE)
                        (task-parameters waypoint ?machine-id machine-point ?machine-point)
                        (task-id 0)
                        (robot-id ?robot-id)
                        (team-color ?team-color)
                        (start-time ?gt)
                        (end-time ?gt)
                        (workpiece-name ?wp-name)
                        (order-id ?order-id)
                        (successful TRUE)
                        (base-color ?base-color)
                        (ring-color $?ring-color)
                        (cap-color ?cap-color)))
    (modify ?r (next-at ?machine-id) (next-side ?machine-point))
  )

  ; if a workpiece is supposedly at the target, modify its position
  (do-for-all-facts ((?wp workpiece)) (and (eq ?wp:holding FALSE)
                                           (eq ?wp:at-machine ?machine-id)
                                           (eq ?wp:at-side ?machine-point)
                                           (eq ?wp:latest-data TRUE))
    (duplicate ?wp (unknown-action TRUE) (start-time ?gt) (at-machine nil) (at-side nil))
    (modify ?wp (end-time ?gt) (latest-data FALSE))
  )

  ; place workpiece at target
  (do-for-fact ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                      (eq ?wp:robot-holding ?robot-id)
                                      (eq ?wp:latest-data TRUE))
    (bind ?wp-name ?wp:name)
    ; check if colors match
    (if (and (neq ?base-color nil)
             (or (neq ?wp:base-color ?base-color)
                 (neq ?wp:ring-colors $?ring-color)
                 (neq ?wp:cap-color ?cap-color))) then
      ; create unknown action for changing colors
      (duplicate ?wp (latest-data FALSE)
                     (start-time ?gt)
                     (end-time ?gt)
                     (unknown-action TRUE)
                     (base-color ?base-color)
                     (ring-colors $?ring-color)
                     (cap-color ?cap-color))
      (modify ?wp (latest-data TRUE)
                  (start-time ?gt)
                  (end-time ?gt)
                  (unknown-action FALSE)
                  (base-color ?base-color)
                  (ring-colors $?ring-color)
                  (cap-color ?cap-color)
                  (at-machine ?machine-id)
                  (at-side ?machine-point)
                  (holding FALSE))
    else
      (duplicate ?wp (start-time ?gt)
                     (unknown-action FALSE)
                     (at-machine ?machine-id)
                     (at-side ?machine-point)
                     (holding FALSE))
      (modify ?wp (end-time ?gt) (latest-date FALSE))
    )
  )

  (modify ?a (workpiece-name ?wp-name) (processed TRUE))
)

(defrule agent-task-buffer-station-received
  "Processing DELIVER action send by the robot. Updating workpiece fact in
  worldmodel."
  ?a <- (agent-task (task-type BUFFER)
                    (robot-id ?robot-id)
                    (unknown-action FALSE)
                    (processed FALSE)
                    (team-color ?team-color)
                    (task-parameters machine-id ?machine-id
                                     shelf_number ?shelf-number)
                    (order-id ?order-id)
                    (base-color ?base-color)
                    (ring-color $?ring-color)
                    (cap-color ?cap-color))
  ?r <- (robot (number ?robot-id) (team-color ?team-color) (next-at ?next-at) (next-side ?next-side))
  (gamestate (game-time ?gt))
  =>
  (printout warn "BufferStation received" crlf)
  (bind ?wp-name nil)

  ; if robot is not at the right place, ensure it with an unknown-action
  (if (or (neq ?next-at ?machine-id)
          (neq ?next-side SHELF)) then
    (assert (agent-task (task-type MOVE)
                        (unknown-action TRUE)
                        (task-parameters waypoint ?machine-id machine-point SHELF)
                        (task-id 0)
                        (robot-id ?robot-id)
                        (team-color ?team-color)
                        (start-time ?gt)
                        (end-time ?gt)
                        (workpiece-name ?wp-name)
                        (order-id ?order-id)
                        (successful TRUE)
                        (base-color ?base-color)
                        (ring-color $?ring-color)
                        (cap-color ?cap-color)))
  )

  ; if a workpiece is supposedly at the target's input, modify its position
  (do-for-all-facts ((?wp workpiece)) (and (eq ?wp:holding FALSE)
                                           (eq ?wp:at-machine ?machine-id)
                                           (eq ?wp:at-side INPUT)
                                           (eq ?wp:latest-data TRUE))
    (duplicate ?wp (unknown-action TRUE) (start-time ?gt) (at-machine nil) (at-side nil))
    (modify ?wp (end-time ?gt) (latest-data FALSE))
  )

  ; check if robot is supposedly holding a workpiece and change fact
  (do-for-all-facts ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                      (eq ?wp:robot-holding ?robot-id)
                                      (eq ?wp:latest-data TRUE))
    (duplicate ?wp (start-time ?gt)
                   (unknown-action TRUE)
                   (holding FALSE))
    (modify ?wp (latest-data FALSE) (end-time ?gt))
  )

  ; create new workpiece and place it at target
  (bind ?wp-name (gensym*))
  (assert (workpiece (latest-data TRUE)
                     (unknown-action FALSE)
                     (name ?wp-name)
                     (start-time ?gt)
                     (holding FALSE)
                     (robot-holding ?robot-id)
                     (order ?order-id)
                     (at-machine ?machine-id)
                     (at-side INPUT)
                     (base-color ?base-color)
                     (ring-colors $?ring-color)
                     (cap-color ?cap-color)))

  (modify ?r (next-at ?machine-id) (next-side INPUT))
  (modify ?a (workpiece-name ?wp-name) (processed TRUE))
)

;---------------------- receiving product-processed --------------------------

(defrule workpiece-at-bs-received
  "When workpiece available at BS, create workpiece fact."
  (gamestate (phase PRODUCTION) (game-time ?gt))
  (workpiece-tracking (enabled FALSE))
  ?pf <- (product-processed (mtype BS)
                            (confirmed FALSE)
                            (at-machine ?m-name)
                            (base-color ?base-color))
  =>
  (bind ?wp-name (gensym*))
  (assert (workpiece (latest-data TRUE)
                     (unknown-action FALSE)
                     (name ?wp-name)
                     (start-time ?gt)
                     (holding FALSE)
                     (robot-holding 0)
                     (at-machine ?m-name)
                     (at-side nil)
                     (base-color ?base-color)
                     (cap-color nil)))

  (modify ?pf (confirmed TRUE))
  (printout t "Workpiece " ?wp-name " created at " ?m-name crlf)
)

(defrule workpiece-at-rs-received
  "Update workpiece available at an RS with the recent production operation."
  (gamestate (phase PRODUCTION) (game-time ?gt))
  (workpiece-tracking (enabled FALSE))
  ?pf <- (product-processed (mtype RS)
                            (confirmed FALSE)
                            (at-machine ?m-name)
                            (ring-color ?r-color))
  =>
  ; if a workpiece is supposedly at the MPS's output, modify its position
  (do-for-all-facts ((?wp workpiece)) (and (eq ?wp:holding FALSE)
                                           (eq ?wp:at-machine ?m-name)
                                           (eq ?wp:at-side OUTPUT)
                                           (eq ?wp:latest-data TRUE))
    (duplicate ?wp (unknown-action TRUE) (start-time ?gt) (at-machine nil) (at-side nil))
    (modify ?wp (end-time ?gt) (latest-data FALSE))
  )

  ; place workpiece at output
  (bind ?wp-name nil)
  (if (not (do-for-fact ((?wp workpiece)) (and (eq ?wp:at-machine ?m-name)
                                               (eq ?wp:at-side INPUT)
                                               (eq ?wp:holding FALSE)
                                               (eq ?wp:latest-data TRUE))
        (bind ?wp-name ?wp:name)
        (duplicate ?wp (start-time ?gt)
                       (unknown-action FALSE)
                       (ring-colors (append$ ?wp:ring-colors ?r-color))
                       (at-side OUTPUT))
        (modify ?wp (latest-data FALSE) (end-time ?gt))
        )
      ) then
    ; create one if none is at the input
    (bind ?wp-name (gensym*))
    (assert (workpiece (latest-data FALSE)
                       (unknown-action TRUE)
                       (name ?wp-name)
                       (start-time ?gt)
                       (end-time ?gt)
                       (holding FALSE)
                       (robot-holding 0)
                       (at-machine ?m-name)
                       (at-side INPUT)
                       (base-color nil)
                       (cap-color nil)))
    (assert (workpiece (latest-data TRUE)
                       (unknown-action FALSE)
                       (name ?wp-name)
                       (start-time ?gt)
                       (holding FALSE)
                       (robot-holding 0)
                       (at-machine ?m-name)
                       (at-side OUTPUT)
                       (base-color nil)
                       (ring-colors ?r-color)
                       (cap-color nil)))
  )

  (modify ?pf (confirmed TRUE))
  (printout t "Workpiece " ?wp-name ": at " ?m-name ", processed" crlf)
)
