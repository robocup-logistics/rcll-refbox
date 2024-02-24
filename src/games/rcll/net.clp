;---------------------------------------------------------------------------
;  net.clp - LLSF RefBox CLIPS network handling
;
;  Created: Thu Feb 14 17:26:27 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;             2019  Mostafa Gomaa
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction net-create-VersionInfo ()
  (bind ?vi (pb-create "llsf_msgs.VersionInfo"))
  (pb-set-field ?vi "version_major" ?*VERSION-MAJOR*)
  (pb-set-field ?vi "version_minor" ?*VERSION-MINOR*)
  (pb-set-field ?vi "version_micro" ?*VERSION-MICRO*)
  (pb-set-field ?vi "version_string"
		(str-cat ?*VERSION-MAJOR*
		         (if (> ?*VERSION-MINOR* 0) then (str-cat "." ?*VERSION-MINOR*) else "")
		         (if (> ?*VERSION-MICRO* 0) then (str-cat "." ?*VERSION-MICRO*) else "")
		)
	)
  (do-for-all-facts ((?pb-conf public-pb-conf)) TRUE
     (bind ?conf (pb-create "llsf_msgs.ConfigValue"))
     (pb-set-field ?conf "name" ?pb-conf:mapped-to)
     (pb-set-field ?conf "type" ?pb-conf:type)
     (if (eq ?pb-conf:type INTEGER) then
       (pb-set-field ?conf "integer_value" ?pb-conf:value)
     )
     (if (eq ?pb-conf:type FLOAT) then
       (pb-set-field ?conf "float_value" ?pb-conf:value)
     )
     (if (eq ?pb-conf:type STRING) then
       (pb-set-field ?conf "string_value" ?pb-conf:value)
     )
     (if (eq ?pb-conf:type BOOLEAN) then
       (pb-set-field ?conf "boolean_value" ?pb-conf:value)
     )
      (pb-add-list ?vi "configuration" ?conf)
  )
  (return ?vi)
)

(deffunction net-init-peer (?cfg-prefix ?group)
  (bind ?peer-id 0)

  (do-for-fact ((?csp confval) (?crp confval) (?ch confval))
	       (and (eq ?csp:type UINT) (eq ?csp:path (str-cat ?cfg-prefix "send-port"))
		    (eq ?crp:type UINT) (eq ?crp:path (str-cat ?cfg-prefix "recv-port"))
		    (eq ?ch:type STRING) (eq ?ch:path (str-cat ?cfg-prefix "host")))
    (printout t "Creating local communication peer for group " ?group
	      " (send port " ?csp:value "  recv port " ?crp:value ")" crlf)
    (bind ?peer-id (pb-peer-create-local ?ch:value ?csp:value ?crp:value))
  )
  (if (eq ?peer-id 0)
   then
    (do-for-fact ((?cp confval) (?ch confval))
	       (and (eq ?cp:type UINT) (eq ?cp:path (str-cat ?cfg-prefix "port"))
		    (eq ?ch:type STRING) (eq ?ch:path (str-cat ?cfg-prefix "host")))
      (printout t "Creating communication peer for group " ?group " (port " ?cp:value ")" crlf)
      (bind ?peer-id (pb-peer-create ?ch:value ?cp:value))
    )
  )

  (if (neq ?peer-id 0)
   then
    (assert (network-peer (group ?group) (id ?peer-id) (network-prefix "")))
   else
    (printout warn "No network configuration found for " ?group " at " ?cfg-prefix crlf)
  )
)

(deffunction net-set-crypto (?team-color ?crypto-key)
  (do-for-fact ((?peer network-peer)) (eq ?peer:group ?team-color)
    (if (debug 3) then (printout t "Setting key " ?crypto-key " for " ?team-color crlf))
    (pb-peer-setup-crypto ?peer:id ?crypto-key "aes-128-cbc")
  )
)

(defrule net-init
  (init)
  (config-loaded)
  (not (network-peer (group PUBLIC)))
  (not (network-peer (group CYAN)))
  (not (network-peer (group MAGENTA)))
  =>
  (net-init-peer "/llsfrb/comm/public-peer/" PUBLIC)
  (net-init-peer "/llsfrb/comm/cyan-peer/" CYAN)
  (net-init-peer "/llsfrb/comm/magenta-peer/" MAGENTA)
)

; (defrule net-print-msg-info
;   (protobuf-msg (type ?t))
;   =>
;   (printout t "Message of type " ?t " received" crlf)
; )

(defrule net-read-known-teams
  (declare (salience -1000))
  (init)
  (confval (path "/llsfrb/game/teams") (type STRING) (is-list TRUE) (list-value $?lv))
  =>
  (printout t "Teams: " ?lv crlf)
  (assert (known-teams ?lv))
)

(defrule net-client-connected
  ?cf <- (protobuf-server-client-connected ?client-id ?host ?port)
  =>
  (retract ?cf)
  (assert (network-client (id ?client-id) (host ?host) (port ?port)))
  (printout t "Client " ?client-id " connected from " ?host ":" ?port crlf)
  ; reset certain signals to trigger immediate re-sending
  (delayed-do-for-all-facts ((?signal signal))
    (member$ ?signal:type
	     (create$ gamestate robot-info machine-info machine-info-bc order-info))
    (modify ?signal (time 0 0))
  )

  ; Send version information right away
  (bind ?vi (net-create-VersionInfo))
  (pb-send ?client-id ?vi)
  (pb-destroy ?vi)

  ; Send game information
  (bind ?gi (pb-create "llsf_msgs.GameInfo"))
  (do-for-fact ((?teams known-teams)) TRUE
    (foreach ?t ?teams:implied
      (pb-add-list ?gi "known_teams" ?t)
    )
  )
  (pb-send ?client-id ?gi)
  (pb-destroy ?gi)
)

(defrule net-client-disconnected
  ?cf <- (protobuf-server-client-disconnected ?client-id)
  ?nf <- (network-client (id ?client-id) (host ?host))
  =>
  (retract ?cf ?nf)
  (printout t "Client " ?client-id " ( " ?host ") disconnected" crlf)
)

(defrule net-send-beacon
  (time $?now)
  ?f <- (signal (type beacon) (time $?t&:(timeout ?now ?t ?*BEACON-PERIOD*)) (seq ?seq))
  (network-peer (group PUBLIC) (id ?peer-id-public))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending beacon" crlf))
  (bind ?beacon (pb-create "llsf_msgs.BeaconSignal"))
  (bind ?beacon-time (pb-field-value ?beacon "time"))
  (pb-set-field ?beacon-time "sec" (nth$ 1 ?now))
  (pb-set-field ?beacon-time "nsec" (integer (* (nth$ 2 ?now) 1000)))
  (pb-set-field ?beacon "time" ?beacon-time) ; destroys ?beacon-time!
  (pb-set-field ?beacon "seq" ?seq)
  (pb-set-field ?beacon "number" 0)
  (pb-set-field ?beacon "team_name" "LLSF")
  (pb-set-field ?beacon "peer_name" "RefBox")
  (pb-broadcast ?peer-id-public ?beacon)
  (pb-destroy ?beacon)
)

(defrule net-recv-beacon
  ?mf <- (protobuf-msg (type "llsf_msgs.BeaconSignal") (ptr ?p) (rcvd-at $?rcvd-at)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  (time-info (game-time ?gt))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  ;(printout t "Received beacon from known " ?from-host ":" ?from-port crlf)
  (bind ?time (pb-field-value ?p "time"))
  (bind ?has-pose FALSE)
  (bind ?pose (create$ 0.0 0.0 0.0))
  (bind ?pose-time (create$ 0 0))

  (if (pb-has-field ?p "pose")
   then
    (bind ?has-pose TRUE)
    (bind ?p-pose (pb-field-value ?p "pose"))
    (bind ?p-pose-time (pb-field-value ?p-pose "timestamp"))
    (bind ?p-pose-x (pb-field-value ?p-pose "x"))
    (bind ?p-pose-y (pb-field-value ?p-pose "y"))
    (bind ?p-pose-ori (pb-field-value ?p-pose "ori"))
    (bind ?p-pose-time-sec (pb-field-value ?p-pose-time "sec"))
    (bind ?p-pose-time-usec (integer (/ (pb-field-value ?p-pose-time "nsec") 1000)))
    (bind ?pose-time (create$ ?p-pose-time-sec ?p-pose-time-usec))
    (bind ?pose (create$ ?p-pose-x ?p-pose-y ?p-pose-ori))
    (pb-destroy ?p-pose-time)
    (pb-destroy ?p-pose)
  )
  (if (pb-has-field ?p "team_color") then
    (bind ?team-color (sym-cat (pb-field-value ?p "team_color")))
  )

  (bind ?time-sec (pb-field-value ?time "sec"))
  (bind ?time-usec (integer (/ (pb-field-value ?time "nsec") 1000)))
  (pb-destroy ?time)

  (assert (robot-beacon (seq (pb-field-value ?p "seq"))
                        (time ?time-sec ?time-usec)
                        (rcvd-at ?rcvd-at)
                        (number (pb-field-value ?p "number"))
                        (team-name (pb-field-value ?p "team_name"))
                        (team-color (sym-cat (pb-field-value ?p "team_color")))
                        (peer-name (pb-field-value ?p "peer_name"))
                        (host ?from-host) (port ?from-port)
                        (has-pose ?has-pose) (pose ?pose) (pose-time ?pose-time)))

  (if (pb-has-field ?p "task") then
    (printout ?*AGENT-TASK-ROUTER* "task received" crlf)
    (bind ?at (pb-field-value ?p "task"))
    (bind ?team-color (pb-field-value ?at "team_color"))
    (bind ?task-id (pb-field-value ?at "task_id"))
    (bind ?robot-id (pb-field-value ?at "robot_id"))

    ; if agent task does not exist, create one
    (if (not (any-factp ((?agent-task agent-task)) (and (eq ?agent-task:team-color ?team-color)
                                                        (eq ?agent-task:task-id ?task-id)
                                                        (eq ?agent-task:robot-id ?robot-id)))) then
      (printout ?*AGENT-TASK-ROUTER* "create agent-task" crlf)
      ; bind task infos
      (bind ?task-type nil)
      (bind ?task-parameters (create$ waypoint nil))
      (if (pb-has-field ?at "move") then
        (bind ?t (pb-field-value ?at "move"))
        (bind ?task-type MOVE)
        (bind ?machine-point nil)
        (if (pb-has-field ?t "machine_point") then
          (bind ?machine-point (sym-cat (pb-field-value ?t "machine_point"))))
        (bind ?task-parameters (create$ waypoint (sym-cat (pb-field-value ?t "waypoint"))
                                        machine-point ?machine-point))
      )
      (if (pb-has-field ?at "retrieve") then
        (bind ?t (pb-field-value ?at "retrieve"))
        (bind ?task-type RETRIEVE)
        (bind ?task-parameters (create$ machine-id (sym-cat (pb-field-value ?t "machine_id"))
                                        machine-point (sym-cat (pb-field-value ?t "machine_point"))))
      )
      (if (pb-has-field ?at "deliver") then
        (bind ?t (pb-field-value ?at "deliver"))
        (bind ?task-type DELIVER)
        (bind ?machine-point nil)
        (if (pb-has-field ?t "machine_point") then
          (bind ?machine-point (sym-cat (pb-field-value ?t "machine_point"))))
        (bind ?task-parameters (create$ machine-id (sym-cat (pb-field-value ?t "machine_id"))
                                        machine-point ?machine-point))
      )
      (if (pb-has-field ?at "buffer") then
        (bind ?t (pb-field-value ?at "buffer"))
        (bind ?task-type BUFFER)
        (bind ?shelf-nr 0)
        (if (pb-has-field ?t "shelf_number") then
          (bind ?shelf-nr (pb-field-value ?t "shelf_number")))
        (bind ?task-parameters (create$ machine-id (sym-cat (pb-field-value ?t "machine_id"))
                                        shelf-number ?shelf-nr))
      )
      (if (pb-has-field ?at "explore_machine") then
        (bind ?t (pb-field-value ?at "explore_machine"))
        (bind ?task-type EXPLORE_MACHINE)
        (bind ?machine-id nil)
        (bind ?machine-point nil)
        (if (pb-has-field ?t "machine_id") then
          (bind ?machine-id (pb-field-value ?t "machine_id")))
        (if (pb-has-field ?t "machine_point") then
          (bind ?machine-point (pb-field-value ?t "machine_point")))
        (bind ?task-parameters (create$ waypoint (sym-cat (pb-field-value ?t "waypoint"))
                                        machine-id ?machine-id
                                        machine-point ?machine-point))
      )

      ; bind wp description
      (bind ?base-color nil)
      (bind $?ring-color (create$))
      (bind ?cap-color nil)
      (if (pb-has-field ?at "workpiece_description") then
        (bind ?wp-desc (pb-field-value ?at "workpiece_description"))
        (bind ?base-color (pb-field-value ?wp-desc "base_color"))
        (bind $?ring-color (pb-field-list ?wp-desc "ring_colors"))
        (if (pb-has-field ?wp-desc "cap_color") then
          (bind ?cap-color (pb-field-value ?wp-desc "cap_color")))
      )

      (bind ?order-id nil)
      (if (pb-has-field ?at "order_id") then
        (bind ?order-id (pb-field-value ?at "order_id"))
      )

      (bind ?successful TRUE)
      (if (pb-has-field ?at "successful") then
        (bind ?order-id (pb-field-value ?at "successful"))
      )

      (assert (agent-task (task-type ?task-type)
                          (task-parameters ?task-parameters)
                          (task-id ?task-id)
                          (robot-id ?robot-id)
                          (team-color ?team-color)
                          (start-time ?gt)
                          (end-time 0.0)
                          (order-id ?order-id)
                          (successful ?successful)
                          (processed FALSE)
                          (base-color ?base-color)
                          (ring-colors $?ring-color)
                          (cap-color ?cap-color)))
      (printout ?*AGENT-TASK-ROUTER* "agent-task: " ?task-type ?task-parameters crlf)
    )

    ; check if end time and succesful flag are set
    (progn$ (?ft (pb-field-list ?p "finished_tasks"))
      (bind ?f-task-id (pb-field-value ?ft "TaskId"))
      (bind ?success (pb-field-value ?ft "successful"))

      (do-for-fact ((?agent-task agent-task))
                    (and (eq ?agent-task:team-color ?team-color)
                         (eq ?agent-task:task-id ?f-task-id)
                         (eq ?agent-task:robot-id ?robot-id)
                         (= ?agent-task:end-time 0.0)) then
        (modify ?agent-task (end-time ?gt))
      )
      (do-for-fact ((?agent-task agent-task))
                (and (eq ?agent-task:team-color ?team-color)
                     (eq ?agent-task:task-id ?f-task-id)
                     (eq ?agent-task:robot-id ?robot-id)
                     (neq ?agent-task:successful ?success)) then
        (modify ?agent-task (successful ?success))
      )
    )
    ; check if previous tasks were not properly marked as finished and set end time
    (do-for-fact ((?agent-task agent-task))
                  (and (eq ?agent-task:team-color ?team-color)
                       (neq ?agent-task:task-id ?task-id)
                       (eq ?agent-task:robot-id ?robot-id)
                       (= ?agent-task:end-time 0.0)) then
      (modify ?agent-task (end-time ?gt))
    )
  )
)

(defrule send-attmsg
  ?af <- (attention-message (text ?text) (team ?team) (time ?time-to-show))
  =>
  (retract ?af)
  (bind ?attmsg (pb-create "llsf_msgs.AttentionMessage"))
  ;(printout warn "AM " ?team ": " (str-cat ?text) crlf)
  (pb-set-field ?attmsg "message" (str-cat ?text))
  (if (neq ?team nil) then (pb-set-field ?attmsg "team_color" ?team))
  (if (> ?time-to-show 0) then
    (pb-set-field ?attmsg "time_to_show" ?time-to-show))

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
		    (pb-send ?client:id ?attmsg))
  (pb-destroy ?attmsg)
  (assert (ws-attention-message ?text ?team ?time-to-show))
)

(defrule net-recv-SetGameState
  ?sf <- (gamestate (state ?state))
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGameState") (ptr ?p) (rcvd-via ?via)
           (rcvd-from ?host ?port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (if (or (eq ?via STREAM) (config-get-bool "/llsfrb/comm/control-via-broadcast"))
   then
    (assert (net-SetGameState (sym-cat (pb-field-value ?p "state"))))
   else
    (printout warn "Illegal SetGameState message received from host " ?host crlf)
  )
)

(defrule net-proc-SetGameState
  ?sf <- (gamestate (state ?old-state))
  ?cmd <- (net-SetGameState ?new-state)
  =>
  (retract ?cmd)
  (printout warn "new state set to the following " ?new-state crlf)
  (modify ?sf (state ?new-state) (prev-state ?old-state))
)

(defrule net-recv-SetGamePhase
  ?sf <- (gamestate (phase ?phase))
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGamePhase") (ptr ?p) (rcvd-via ?via)
           (rcvd-from ?host ?port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (if (or (eq ?via STREAM) (config-get-bool "/llsfrb/comm/control-via-broadcast"))
   then
    (assert (net-SetGamePhase (sym-cat (pb-field-value ?p "phase"))))
   else
    (printout warn "Illegal SetGamePhase message received from host " ?host crlf)
  )
)

(defrule net-proc-SetGamePhase
  ?sf <- (gamestate (phase ?old-phase))
  ?cmd <- (net-SetGamePhase ?new-phase)
  =>
  (retract ?cmd)
  (modify ?sf (phase ?new-phase) (prev-phase ?old-phase))
)


(defrule net-recv-SetTeamName
  ?sf <- (gamestate (phase ?phase) (teams $?old-teams))
  ?mf <- (protobuf-msg (type "llsf_msgs.SetTeamName") (ptr ?p) (rcvd-via ?via)
           (rcvd-from ?host ?port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (if (or (eq ?via STREAM) (config-get-bool "/llsfrb/comm/control-via-broadcast"))
   then
    (bind ?team-color (sym-cat (pb-field-value ?p "team_color")))
    (bind ?new-team (pb-field-value ?p "team_name"))
    (assert (net-SetTeamName ?team-color ?new-team))
   else
    (printout warn "Illegal SetTeamName message received from host " ?host crlf)
  )

)

(defrule net-proc-SetTeamName
  ?sf <- (gamestate (phase ?phase) (teams $?old-teams))
  ?cmd <- (net-SetTeamName ?team-color ?new-team)
  =>
  (retract ?cmd) ; message will be destroyed after rule completes
  (bind ?new-teams ?old-teams)
  (printout t "Setting team " ?team-color " to " ?new-team crlf)
  (if (eq ?team-color CYAN)
   then (bind ?new-teams (replace$ ?new-teams 1 1 ?new-team))
   else (bind ?new-teams (replace$ ?new-teams 2 2 ?new-team))
  )
  (modify ?sf (teams ?new-teams))

  (bind ?confpfx (str-cat "/llsfrb/game/crypto-keys/" ?new-team))
  (bind ?crypto-done FALSE)
  (do-for-fact ((?ckey confval))
	       (and (eq ?ckey:path (str-cat "/llsfrb/game/crypto-keys/" ?new-team)) (eq ?ckey:type STRING))
    (net-set-crypto ?team-color ?ckey:value)
    (bind ?crypto-done TRUE)
  )
  (if (not ?crypto-done) then
    (printout warn "No encryption configured for team " ?new-team ", disabling" crlf)
    (net-set-crypto ?team-color "")
  )

  ; Remove all known robots if the team is changed
  (if (and (eq ?phase PRE_GAME) (neq ?old-teams ?new-teams))
    then (delayed-do-for-all-facts ((?r robot)) TRUE (retract ?r)))
)


(defrule net-recv-RandomizeField
	?sf <- (gamestate (phase PRE_GAME|SETUP))
	?mf <- (protobuf-msg (type "llsf_msgs.RandomizeField") (ptr ?p) (rcvd-via STREAM))
	=>
	(retract ?mf)
	(assert (net-RandomizeField))
)

(defrule net-proc-RandomizeField
	?sf <- (gamestate (phase PRE_GAME|SETUP))
  ?cmd <- (net-RandomizeField)
  =>
  (retract ?cmd)
	(assert (game-reset))
	(delayed-do-for-all-facts ((?m machine)) TRUE
	  (modify ?m (zone TBD))
	)
)

(defrule net-recv-Workpiece
  ?mf <- (protobuf-msg (type "llsf_msgs.Workpiece") (ptr ?p) (rcvd-at $?rcvd-at)
											 (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf) ; message will be destroyed after rule completes

  (bind ?id (pb-field-value ?p "id"))
  (bind ?at-machine-str (pb-field-value ?p "at_machine"))
  (printout t "Read barcode " ?id " at " ?at-machine-str crlf)
  (assert (mps-read-barcode (sym-cat ?at-machine-str) ?id))
)

(defrule net-recv-WorkpieceAddRing
  ?mf <- (protobuf-msg (type "llsf_msgs.WorkpieceAddRing") (ptr ?p) (rcvd-at $?rcvd-at)
											 (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  (time-info (game-time ?gt))
  =>
  (retract ?mf) ; message will be destroyed after rule completes

  (bind ?id (pb-field-value ?p "id"))
	(bind ?ring-color (sym-cat (pb-field-value ?p "ring_color")))

	(printout t "Add ring " ?ring-color " to workpiece " ?id crlf)

	(do-for-fact ((?wp workpiece)) (and (eq ?wp:id ?id) (eq ?wp:latest-data TRUE))
		(printout t "Add ring " ?ring-color " to workpiece " ?id " *** " crlf)
	  (duplicate ?wp (start-time ?gt) (ring-colors (append$ ?wp:ring-colors ?ring-color)))
	  (modify ?wp (latest-data FALSE) (end-time ?gt))
	)
)

(deffunction net-create-WorkpieceInfo ()
  (bind ?wi (pb-create "llsf_msgs.WorkpieceInfo"))

  (do-for-all-facts
    ((?wp workpiece)) (eq ?wp:latest-data TRUE)

    (bind ?w (pb-create "llsf_msgs.Workpiece"))
		(pb-set-field ?w "id" ?wp:id)
		(pb-set-field ?w "at_machine" (str-cat ?wp:at-machine))
		(pb-set-field ?w "team" (str-cat ?wp:team))
		(pb-set-field ?w "visible" ?wp:visible)

		(if (neq ?wp:base-color nil) then
			(pb-set-field ?w "base_color" (str-cat ?wp:base-color)))
		(foreach ?rc ?wp:ring-colors (pb-add-list ?w "ring_colors" (str-cat ?rc)))
		(if (neq ?wp:cap-color nil) then
			(pb-set-field ?w "cap_color" (str-cat ?wp:cap-color)))

		(pb-add-list ?wi "workpieces" ?w) ; destroys ?w
  )

  (return ?wi)
)

(defrule net-send-WorkpieceInfo
  (time $?now)
  ?f <- (signal (type workpiece-info) (time $?t&:(timeout ?now ?t ?*WORKPIECEINFO-PERIOD*)) (seq ?seq))
  (workpiece-tracking (enabled TRUE) (broadcast TRUE))
  (time-info (cont-time ?ctime))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?wi (net-create-WorkpieceInfo))

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
    (pb-send ?client:id ?wi))
  (pb-destroy ?wi)
)

(deffunction net-create-GameState (?gs ?ti)
  (bind ?gamestate (pb-create "llsf_msgs.GameState"))
  (bind ?gamestate-time (pb-field-value ?gamestate "game_time"))
  (if (eq (type ?gamestate-time) EXTERNAL-ADDRESS) then
    (bind ?gt (time-from-sec (fact-slot-value ?ti game-time)))
    (pb-set-field ?gamestate-time "sec" (nth$ 1 ?gt))
    (pb-set-field ?gamestate-time "nsec" (integer (* (nth$ 2 ?gt) 1000)))
    (pb-set-field ?gamestate "game_time" ?gamestate-time) ; destroys ?gamestate-time!
  )
  (pb-set-field ?gamestate "state" (fact-slot-value ?gs state))
  (pb-set-field ?gamestate "phase" (fact-slot-value ?gs phase))
  (pb-set-field ?gamestate "points_cyan"    (nth$ 1 (fact-slot-value ?gs points)))
  (pb-set-field ?gamestate "points_magenta" (nth$ 2 (fact-slot-value ?gs points)))
  (bind ?team_cyan    (nth$ 1 (fact-slot-value ?gs teams)))
  (bind ?team_magenta (nth$ 2 (fact-slot-value ?gs teams)))
  (if (neq ?team_cyan "") then
    (pb-set-field ?gamestate "team_cyan"  ?team_cyan))
  (if (neq ?team_magenta "") then
    (pb-set-field ?gamestate "team_magenta"  ?team_magenta))

  (pb-set-field ?gamestate "field_width"  ?*FIELD-WIDTH*)
  (pb-set-field ?gamestate "field_height" ?*FIELD-HEIGHT*)
  (pb-set-field ?gamestate "field_mirrored" ?*FIELD-MIRRORED*)
  (return ?gamestate)
)

(defrule net-send-GameState
  (time $?now)
  ?gs <- (gamestate (refbox-mode ?refbox-mode) (state ?state) (phase ?phase) (teams $?teams))
  ?ti <- (time-info (game-time ?game-time))
  ?f <- (signal (type gamestate) (time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  (network-peer (group PUBLIC) (id ?peer-id-public))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending GameState" crlf))
  (bind ?gamestate (net-create-GameState ?gs ?ti))

  (pb-broadcast ?peer-id-public ?gamestate)

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
    (pb-send ?client:id ?gamestate)
  )
  (pb-destroy ?gamestate)
)

(deffunction net-create-RobotInfo (?ctime ?pub-pose)
  (bind ?ri (pb-create "llsf_msgs.RobotInfo"))

  (do-for-all-facts
    ((?robot robot)) (neq ?robot:team-color nil)

    (bind ?r (pb-create "llsf_msgs.Robot"))
    (bind ?r-time (pb-field-value ?r "last_seen"))
    (if (eq (type ?r-time) EXTERNAL-ADDRESS) then
      (pb-set-field ?r-time "sec" (nth$ 1 ?robot:last-seen))
      (pb-set-field ?r-time "nsec" (integer (* (nth$ 2 ?robot:last-seen) 1000)))
      (pb-set-field ?r "last_seen" ?r-time) ; destroys ?r-time!
    )

    ; If we have a pose publish it
    (if (and ?pub-pose (non-zero-pose ?robot:pose)) then
      (bind ?p (pb-field-value ?r "pose"))
      (bind ?p-time (pb-field-value ?p "timestamp"))
      (pb-set-field ?p-time "sec" (nth$ 1 ?robot:pose-time))
      (pb-set-field ?p-time "nsec" (integer (* (nth$ 2 ?robot:pose-time) 1000)))
      (pb-set-field ?p "timestamp" ?p-time)
      (pb-set-field ?p "x" (nth$ 1 ?robot:pose))
      (pb-set-field ?p "y" (nth$ 2 ?robot:pose))
      (pb-set-field ?p "ori" (nth$ 3 ?robot:pose))
      (pb-set-field ?r "pose" ?p)
    )

    (pb-set-field ?r "name" ?robot:name)
    (pb-set-field ?r "team" ?robot:team)
    (pb-set-field ?r "team_color" ?robot:team-color)
    (pb-set-field ?r "number" ?robot:number)
    (pb-set-field ?r "state" ?robot:state)
    (pb-set-field ?r "host" ?robot:host)

    (if (eq ?robot:state MAINTENANCE) then
      (bind ?maintenance-time-remaining
	    (- ?*MAINTENANCE-ALLOWED-TIME* (- ?ctime ?robot:maintenance-start-time)))
      (pb-set-field ?r "maintenance_time_remaining" ?maintenance-time-remaining)
    )
    (pb-set-field ?r "maintenance_cycles" ?robot:maintenance-cycles)

    (pb-add-list ?ri "robots" ?r) ; destroys ?r
  )

  (return ?ri)
)

(defrule net-send-RobotInfo
  (time $?now)
  ?f <- (signal (type robot-info) (time $?t&:(timeout ?now ?t ?*ROBOTINFO-PERIOD*)) (seq ?seq))
  (time-info (cont-time ?ctime))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?ri (net-create-RobotInfo ?ctime TRUE))

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
    (pb-send ?client:id ?ri))
  (pb-destroy ?ri)
)

(defrule net-broadcast-RobotInfo
  (time $?now)
  ?f <- (signal (type bc-robot-info)
		(time $?t&:(timeout ?now ?t ?*BC-ROBOTINFO-PERIOD*)) (seq ?seq))
  (time-info (game-time ?gtime))
  (network-peer (group PUBLIC) (id ?peer-id-public))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?ri (net-create-RobotInfo ?gtime FALSE))
  (pb-broadcast ?peer-id-public ?ri)
  (pb-destroy ?ri)
)

(deffunction net-create-ShelfSlotInfo (?message ?mps-ss)
	(do-for-all-facts ((?ssf machine-ss-shelf-slot))
	                       (eq ?ssf:name ?mps-ss)
	                  (bind ?ssf-pb (pb-create "llsf_msgs.ShelfSlotInfo"))
	                  (pb-set-field ?ssf-pb "shelf" (nth$ 1 ?ssf:position))
	                  (pb-set-field ?ssf-pb "slot" (nth$ 2 ?ssf:position))
	                  (pb-set-field ?ssf-pb "is_filled" ?ssf:is-filled)
	                  (pb-set-field ?ssf-pb "description" ?ssf:description)
	                  (pb-add-list ?message "status_ss" ?ssf-pb)
	)
)

(deffunction net-create-Machine (?mf ?meta-f ?mlf ?add-restricted-info)
    (bind ?m (pb-create "llsf_msgs.Machine"))

    (bind ?mtype (fact-slot-value ?mf mtype))
    (bind ?zone (fact-slot-value ?mf zone))
    (bind ?rotation (fact-slot-value ?mf rotation))

    (pb-set-field ?m "name" (fact-slot-value ?mf name))
    (pb-set-field ?m "type" ?mtype)
    (pb-set-field ?m "team_color" (fact-slot-value ?mf team))
    (if (and (any-factp ((?gs gamestate)) (or (eq ?gs:phase SETUP) (eq ?gs:phase PRODUCTION)))
             (eq ?mtype RS) (> (length$ (fact-slot-value ?meta-f available-colors)) 0))
     then
     (foreach ?rc (fact-slot-value ?meta-f available-colors)
       (pb-add-list ?m "ring_colors" ?rc)
     )
    )
    (if (any-factp ((?gs gamestate) (?send send-mps-positions)) (member$ ?gs:phase ?send:phases))
      then
        (if (neq ?zone TBD) then (pb-set-field ?m "zone" (fact-slot-value ?mf zone)))
        (if (neq ?rotation -1) then (pb-set-field ?m "rotation" (fact-slot-value ?mf rotation)))
    )
    (pb-set-field ?m "state" (fact-slot-value ?mf state))

    (if (eq ?mtype SS) then
      (net-create-ShelfSlotInfo ?m (fact-slot-value ?mf name))
    )

    (if ?add-restricted-info
     then
      (if (neq ?zone TBD) then
        (pb-set-field ?m "zone" (fact-slot-value ?mf zone))
      )
      (if (neq ?rotation -1) then (pb-set-field ?m "rotation" (fact-slot-value ?mf rotation)))
      (if (eq ?mtype RS) then
        (pb-set-field ?m "loaded_with"
          (- (fact-slot-value ?meta-f bases-added) (fact-slot-value ?meta-f bases-used)))
      )
      (if (eq ?mtype CS) then
        (pb-set-field ?m "loaded_with"
          (if (fact-slot-value ?meta-f has-retrieved) then 1 else 0))
      )

      (foreach ?l (fact-slot-value ?mlf actual-lights)
        (bind ?ls (pb-create "llsf_msgs.LightSpec"))
	(bind ?dashidx (str-index "-" ?l))
	(bind ?color (sub-string 1 (- ?dashidx 1) ?l))
	(bind ?state (sub-string (+ ?dashidx 1) (str-length ?l) ?l))
	(pb-set-field ?ls "color" ?color)
	(pb-set-field ?ls "state" ?state)
	(pb-add-list ?m "lights" ?ls)
      )

      (if (not (member$ (fact-slot-value ?mf state) (create$ IDLE BROKEN DOWN))) then
	(switch (fact-slot-value ?mf mtype)
	  (case BS then
	    (bind ?pm (pb-create "llsf_msgs.PrepareInstructionBS"))
	    (pb-set-field ?pm "side" (fact-slot-value ?meta-f current-side))
	    (pb-set-field ?pm "color" (fact-slot-value ?meta-f current-base-color))
	          (pb-set-field ?m "instruction_bs" ?pm)
	    )
	  (case DS then
	    (bind ?pm (pb-create "llsf_msgs.PrepareInstructionDS"))
	    (pb-set-field ?pm "gate" (fact-slot-value ?meta-f gate))
	    (pb-set-field ?pm "order_id" (fact-slot-value ?meta-f order-id))
	    (pb-set-field ?m "instruction_ds" ?pm)
	  )
	  (case SS then
	    (bind ?pm (pb-create "llsf_msgs.PrepareInstructionSS"))
	    (pb-set-field ?pm "operation" (fact-slot-value ?meta-f current-operation))
	    (bind ?shelf-slot (fact-slot-value ?meta-f current-shelf-slot))
	    (pb-set-field ?pm "shelf" (nth$ 1 ?shelf-slot))
	    (pb-set-field ?pm "slot" (nth$ 2 ?shelf-slot))
	    (pb-set-field ?m "instruction_ss" ?pm)
	  )
	  (case RS then
	    (bind ?pm (pb-create "llsf_msgs.PrepareInstructionRS"))
	    (pb-set-field ?pm "ring_color" (fact-slot-value ?meta-f current-ring-color))
	    (pb-set-field ?m "instruction_rs" ?pm)
	  )
	  (case CS then
	    (bind ?pm (pb-create "llsf_msgs.PrepareInstructionCS"))
	    (pb-set-field ?pm "operation" (fact-slot-value ?meta-f operation-mode))
	    (pb-set-field ?m "instruction_cs" ?pm)
	  )
        )
      )
    )

    ; If we have a pose publish it
    (if (non-zero-pose (fact-slot-value ?mf pose)) then
      (bind ?p (pb-field-value ?m "pose"))
      (bind ?p-time (pb-field-value ?p "timestamp"))
      (pb-set-field ?p-time "sec" (nth$ 1 (fact-slot-value ?mf pose-time)))
      (pb-set-field ?p-time "nsec" (integer (* (nth$ 2 (fact-slot-value ?mf pose-time)) 1000)))
      (pb-set-field ?p "timestamp" ?p-time)
      (pb-set-field ?p "x" (nth$ 1 (fact-slot-value ?mf pose)))
      (pb-set-field ?p "y" (nth$ 2 (fact-slot-value ?mf pose)))
      (pb-set-field ?p "ori" (nth$ 3 (fact-slot-value ?mf pose)))
      (pb-set-field ?m "pose" ?p)
    )

    ; In exploration phase, indicate whether this was correctly reported
    (do-for-fact ((?gs gamestate)) (eq ?gs:phase EXPLORATION)
      (do-for-fact ((?report exploration-report))
			 	(and (eq ?report:rtype RECORD) (eq ?report:name (fact-slot-value ?mf name)))

				(pb-set-field ?m "correctly_reported" (if (eq ?report:correctly-reported TRUE) then TRUE else FALSE))
				(pb-set-field ?m "exploration_rotation_state" ?report:rotation-state)
				(pb-set-field ?m "exploration_zone_state" ?report:zone-state)
      )
    )

    (return ?m)
)

(defrule net-send-MachineInfo
  (time $?now)
  (gamestate (phase ?phase))
  ?sf <- (signal (type machine-info)
		 (time $?t&:(timeout ?now ?t ?*MACHINE-INFO-PERIOD*)) (seq ?seq))
  (machine-generation (state FINISHED))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))
  (bind ?s (pb-create "llsf_msgs.MachineInfo"))

  (do-for-all-facts ((?machine machine) (?machine-lights machine-lights))
    (eq ?machine:name ?machine-lights:name)
    (bind ?m (net-create-Machine ?machine (get-machine-meta-fact ?machine) ?machine-lights TRUE))
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
    (pb-send ?client:id ?s)
  )
  (pb-destroy ?s)
)

(deffunction net-create-broadcast-MachineInfo (?team-color)
  (bind ?s (pb-create "llsf_msgs.MachineInfo"))
  (pb-set-field ?s "team_color" ?team-color)
  (do-for-all-facts ((?machine machine) (?machine-lights machine-lights))
    (and (eq ?machine:name ?machine-lights:name)
         (eq ?machine:team ?team-color))
    (bind ?m (net-create-Machine ?machine (get-machine-meta-fact ?machine) ?machine-lights FALSE))
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  (return ?s)
)

(defrule net-broadcast-MachineInfo-on-state-change
  (declare (salience ?*PRIORITY_HIGH*))
  (time $?now)
  (gamestate (phase PRODUCTION))
  ?sf <- (signal (type machine-info-bc) (seq ?seq) (count ?count))
  (network-peer (group CYAN) (id ?peer-id-cyan))
  (network-peer (group MAGENTA) (id ?peer-id-magenta))
  ?smu <- (send-machine-update)
  (machine-generation (state FINISHED))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

  (bind ?s (net-create-broadcast-MachineInfo CYAN))
  (pb-broadcast ?peer-id-cyan ?s)
  (pb-destroy ?s)

  (bind ?s (net-create-broadcast-MachineInfo MAGENTA))
  (pb-broadcast ?peer-id-magenta ?s)
  (pb-destroy ?s)
  (retract ?smu)
)


(defrule net-broadcast-MachineInfo
  (time $?now)
  (gamestate (phase PRODUCTION))
  ?sf <- (signal (type machine-info-bc) (seq ?seq) (count ?count)
		 (time $?t&:(timeout ?now ?t (if (> ?count ?*BC-MACHINE-INFO-BURST-COUNT*)
					       then ?*BC-MACHINE-INFO-PERIOD*
					       else ?*BC-MACHINE-INFO-BURST-PERIOD*))))
  (network-peer (group CYAN) (id ?peer-id-cyan))
  (network-peer (group MAGENTA) (id ?peer-id-magenta))
  (machine-generation (state FINISHED))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

  (bind ?s (net-create-broadcast-MachineInfo CYAN))
  (pb-broadcast ?peer-id-cyan ?s)
  (pb-destroy ?s)

  (bind ?s (net-create-broadcast-MachineInfo MAGENTA))
  (pb-broadcast ?peer-id-magenta ?s)
  (pb-destroy ?s)
)

(deffunction net-create-RingInfo ()
  (bind ?s (pb-create "llsf_msgs.RingInfo"))

  (do-for-all-facts ((?ring-spec ring-spec)) TRUE
    (bind ?rs (pb-create "llsf_msgs.Ring"))
    (pb-set-field ?rs "ring_color" ?ring-spec:color)
    (pb-set-field ?rs "raw_material" ?ring-spec:req-bases)
		(pb-add-list ?s "rings" ?rs)
  )

  (return ?s)
)

(defrule net-broadcast-RingInfo
  (time $?now)
  (gamestate (phase PRODUCTION))
  ?sf <- (signal (type ring-info-bc) (seq ?seq) (count ?count)
								 (time $?t&:(timeout ?now ?t ?*BC-MACHINE-INFO-PERIOD*)))
  (network-peer (group CYAN) (id ?peer-id-cyan))
  (network-peer (group MAGENTA) (id ?peer-id-magenta))
  (machine-generation (state FINISHED))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

  (bind ?s (net-create-RingInfo))
  (pb-broadcast ?peer-id-cyan ?s)
  (pb-broadcast ?peer-id-magenta ?s)
  (pb-destroy ?s)
)

(deffunction net-create-UnconfirmedDelivery (?id ?team ?time)
  (bind ?msg (pb-create "llsf_msgs.UnconfirmedDelivery"))
  (pb-set-field ?msg "id" ?id)
  (pb-set-field ?msg "team" ?team)
  (bind ?delivery-time (pb-field-value ?msg "delivery_time"))
  (if (eq (type ?delivery-time) EXTERNAL-ADDRESS) then
    (bind ?gt (time-from-sec ?time))
    (pb-set-field ?delivery-time "sec" (nth$ 1 ?gt))
    (pb-set-field ?delivery-time "nsec" (integer (* (nth$ 2 ?gt) 1000)))
    (pb-set-field ?msg "delivery_time" ?delivery-time) ; destroys ?delivery-time!
  )
  (return ?msg)
)

(deffunction net-create-Order (?order-fact)
  (bind ?o (pb-create "llsf_msgs.Order"))

  (pb-set-field ?o "id" (fact-slot-value ?order-fact id))
  (pb-set-field ?o "complexity" (fact-slot-value ?order-fact complexity))
  (pb-set-field ?o "competitive" (fact-slot-value ?order-fact competitive))
  (pb-set-field ?o "base_color" (fact-slot-value ?order-fact base-color))
  (foreach ?rc (fact-slot-value ?order-fact ring-colors)
    (pb-add-list ?o "ring_colors" ?rc)
  )
  (pb-set-field ?o "cap_color" (fact-slot-value ?order-fact cap-color))

  (pb-set-field ?o "quantity_requested" (fact-slot-value ?order-fact quantity-requested))
  (pb-set-field ?o "quantity_delivered_cyan"
		(nth$ 1 (fact-slot-value ?order-fact quantity-delivered)))
  (pb-set-field ?o "quantity_delivered_magenta"
		(nth$ 2 (fact-slot-value ?order-fact quantity-delivered)))
  (pb-set-field ?o "delivery_gate" (fact-slot-value ?order-fact delivery-gate))
  (pb-set-field ?o "delivery_period_begin"
		(nth$ 1 (fact-slot-value ?order-fact delivery-period)))
  (pb-set-field ?o "delivery_period_end"
		(nth$ 2 (fact-slot-value ?order-fact delivery-period)))

  (do-for-all-facts
    ((?delivery product-processed) (?rf referee-confirmation))
    (and (eq ?delivery:confirmed FALSE) (eq ?delivery:order (fact-slot-value ?order-fact id))
         (eq ?delivery:mtype DS) (eq ?delivery:id ?rf:process-id)
         (eq ?rf:state REQUIRED))

    (bind ?d (net-create-UnconfirmedDelivery ?delivery:id ?delivery:team ?delivery:game-time))
    (pb-add-list ?o "unconfirmed_deliveries" ?d)
  )
  (return ?o)
)

(deffunction net-create-OrderInfo ()
  (bind ?oi (pb-create "llsf_msgs.OrderInfo"))

  (do-for-all-facts
    ((?order order)) (eq ?order:active TRUE)
    (bind ?o (net-create-Order ?order))
    (pb-add-list ?oi "orders" ?o) ; destroys ?o
  )
  (return ?oi)
)

(defrule net-send-OrderInfo
  (time $?now)
  (gamestate (phase PRODUCTION))
  ?sf <- (signal (type order-info) (seq ?seq) (count ?count)
		 (time $?t&:(timeout ?now ?t (if (> ?count ?*BC-ORDERINFO-BURST-COUNT*)
					       then ?*BC-ORDERINFO-PERIOD*
					       else ?*BC-ORDERINFO-BURST-PERIOD*))))
  (network-peer (group PUBLIC) (id ?peer-id))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

  (bind ?oi (net-create-OrderInfo))

  (do-for-all-facts ((?client network-client)) (not ?client:is-slave)
    (pb-send ?client:id ?oi))
  (pb-broadcast ?peer-id ?oi)
  (pb-destroy ?oi)
)


(defrule net-send-VersionInfo
  (time $?now)
  ?sf <- (signal (type version-info) (seq ?seq)
		 (count ?count&:(< ?count ?*BC-VERSIONINFO-COUNT*))
		 (time $?t&:(timeout ?now ?t ?*BC-VERSIONINFO-PERIOD*)))
  (network-peer (group PUBLIC) (id ?peer-id-public))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))
  (bind ?vi (net-create-VersionInfo))
  (pb-broadcast ?peer-id-public ?vi)
  (pb-destroy ?vi)
)
