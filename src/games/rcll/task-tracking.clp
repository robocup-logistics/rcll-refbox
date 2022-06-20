
;---------------------------------------------------------------------------
;  task-tracking.clp - LLSF RefBox CLIPS task tracking rules
;
;  Created: Tue Jun 07 12:44:01 2022
;  Copyright  2022       Matteo Tschesche
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

;------------------------ receiving agent-tasks ----------------------------

(defrule agent-task-move-received
  "Processing MOVE action send by the robot. Updating robot next position in
  worldmodel."
  ?a <- (agent-task (task-type MOVE)
                    (robot-id ?robot-id)
                    ;(unknown-action FALSE)
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
  (printout warn "Move received" crlf)
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
  else
    ; check if robot is supposedly holding a workpiece and change fact
    (do-for-fact ((?wp workpiece)) (and (eq ?wp:holding TRUE)
                                        (eq ?wp:robot-holding ?robot-id)
                                        (eq ?wp:latest-data TRUE))
      (duplicate ?wp (start-time ?gt)
                     (unknown-action TRUE)
                     (holding FALSE))
      (modify ?wp (latest-data FALSE) (end-time ?gt))
    )
  )
  (modify ?a (processed TRUE) (workpiece-name ?wp-name))
)

(defrule agent-task-retrieve-received
  "Processing RETRIEVE action send by the robot. Updating robots next position in
  worldmodel."
  ?a <- (agent-task (task-type RETRIEVE)
                    (robot-id ?robot-id)
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
                                               (eq ?wp:at-side ?machine-point)
                                               (eq ?wp:holding FALSE)
                                               (eq ?wp:latest-data TRUE))
        (bind ?wp-name ?wp:name)
        ; check if colors match
        (if (or (neq ?wp:base-color ?base-color)
                (neq ?wp:ring-colors $?ring-color)
                (neq ?wp:cap-color ?cap-color)) then
          ; create unknown action for changing colors
          (duplicate ?wp (latest-data FALSE)
                         (start-time ?gt)
                         (end-time ?gt)
                         (unknown-action TRUE)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color))

          ; create holding fact
          (duplicate ?wp (latest-data TRUE)
                         (start-time ?gt)
                         (holding TRUE)
                         (robot-holding ?robot-id)
                         (unknown-action FALSE)
                         (order ?order-id)
                         (base-color ?base-color)
                         (ring-colors $?ring-color)
                         (cap-color ?cap-color))
          (modify ?wp (latest-data FALSE) (end-time ?gt))
        else
          (duplicate ?wp (start-time ?gt) (holding TRUE) (robot-holding ?robot-id) (order ?order-id))
          (modify ?wp (latest-data FALSE) (end-time ?gt))
        )
      )
    )
  then
    ; create workpiece with random id
    (bind ?wp-name (gensym*))
    (if (eq (length$ $?ring-color) 0) then
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
    else
      ; if workpiece has a ring already, create unknown-action instead
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
    )
  )
  (modify ?a (workpiece-name ?wp-name) (processed TRUE))
)


;---------------------- receiving product-processed --------------------------
