
;---------------------------------------------------------------------------
;  net.clp - LLSF RefBox CLIPS network handling
;
;  Created: Thu Feb 14 17:26:27 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction net-create-VersionInfo ()
  (bind ?vi (pb-create "llsf_msgs.VersionInfo"))
  (pb-set-field ?vi "version_major" ?*VERSION-MAJOR*)
  (pb-set-field ?vi "version_minor" ?*VERSION-MINOR*)
  (pb-set-field ?vi "version_micro" ?*VERSION-MICRO*)
  (pb-set-field ?vi "version_string"
		(str-cat ?*VERSION-MAJOR* "." ?*VERSION-MINOR* "." ?*VERSION-MICRO*))
  (return ?vi)
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
	     (create$ gamestate robot-info machine-info machine-info-bc puck-info order-info))
    (modify ?signal (time 0 0))
  )

  ; Send version information right away
  (bind ?vi (net-create-VersionInfo))
  (pb-send ?client-id ?vi)
  (pb-destroy ?vi)
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
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending beacon" crlf))
  (bind ?beacon (pb-create "llsf_msgs.BeaconSignal"))
  (bind ?beacon-time (pb-field-value ?beacon "time"))
  (pb-set-field ?beacon-time "sec" (nth$ 1 ?now))
  (pb-set-field ?beacon-time "nsec" (integer (* (nth$ 2 ?now) 1000)))
  (pb-set-field ?beacon "time" ?beacon-time) ; destroys ?beacon-time!
  (pb-set-field ?beacon "seq" ?seq)
  (pb-set-field ?beacon "team_name" "LLSF")
  (pb-set-field ?beacon "peer_name" "RefBox")
  (pb-broadcast ?beacon)
  (pb-destroy ?beacon)
)

(defrule net-recv-beacon-known
  (time $?now)
  ?mf <- (protobuf-msg (type "llsf_msgs.BeaconSignal") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  ?rf <- (robot (host ?from-host) (port ?from-port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  ;(printout t "Received beacon from known " ?from-host ":" ?from-port crlf)
  (bind ?time (pb-field-value ?p "time"))
  (bind ?pose-time (create$ 0 0))
  (bind ?pose (create$ 0.0 0.0 0.0))
  (if (pb-has-field ?p "pose")
   then
    (bind ?p-pose (pb-field-value ?p "pose"))
    (bind ?p-pose-time (pb-field-value ?p-pose "timestamp"))
    (bind ?p-pose-time-sec (pb-field-value ?p-pose-time "sec"))
    (bind ?p-pose-time-usec (integer (/ (pb-field-value ?p-pose-time "nsec") 1000)))
    (bind ?p-pose-x (pb-field-value ?p-pose "x"))
    (bind ?p-pose-y (pb-field-value ?p-pose "y"))
    (bind ?p-pose-ori (pb-field-value ?p-pose "ori"))
    (bind ?pose-time (create$ ?p-pose-time-sec ?p-pose-time-usec))
    (bind ?pose (create$ ?p-pose-x ?p-pose-y ?p-pose-ori))
    (pb-destroy ?p-pose-time)
    (pb-destroy ?p-pose)
  )
  (modify ?rf (last-seen ?now) (warning-sent FALSE)
              (pose ?pose) (pose-time ?pose-time))
)

(defrule net-recv-beacon-unknown
  (declare (salience -10))
  (time $?now)
  ?mf <- (protobuf-msg (type "llsf_msgs.BeaconSignal") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  (not (robot (host ?from-host) (port ?from-port)))
  ?sf <- (signal (type version-info))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (modify ?sf (count 0) (time 0 0))
  (printout debug "Received initial beacon from " ?from-host ":" ?from-port crlf)
  (bind ?team (pb-field-value ?p "team_name"))
  (bind ?name (pb-field-value ?p "peer_name"))
  (bind ?timef (pb-field-value ?p "time"))
  (bind ?time (create$ (pb-field-value ?timef "sec") (integer (/ (pb-field-value ?timef "nsec") 1000))))
  (bind ?peer-time-diff (abs (time-diff-sec ?now ?time)))
  (if (> ?peer-time-diff ?*PEER-TIME-DIFFERENCE-WARNING*)
   then
    (printout warn "Robot " ?name " of " ?team
	      " has a large time offset (" ?peer-time-diff " sec)" crlf)
    (assert (attention-message (str-cat "Robot " ?name " of " ?team
					" has a large time offset (" ?peer-time-diff " sec)")
			       5))
  )
  (do-for-fact ((?other robot)) (eq ?other:host ?from-host)
    (printout warn "Received two BeaconSignals from host " ?from-host
	      " (" ?other:team "/" ?other:name "@" ?other:port " vs "
	      ?team "/" ?from-host "@" ?from-port ")" crlf)
    (assert (attention-message (str-cat "Received two BeaconSignals form host " ?from-host
					" (" ?other:team "/" ?other:name "@" ?other:port
					" vs " ?team "/" ?from-host "@" ?from-port ")") 5))
  )
  (assert (robot (team (pb-field-value ?p "team_name"))
		 (name (pb-field-value ?p "peer_name"))
		 (host ?from-host) (port ?from-port)
		 (last-seen ?now)))
)

(defrule net-robot-lost
  (time $?now)
  ?rf <- (robot (team ?team) (name ?name) (host ?host) (port ?port) (warning-sent FALSE)
		(last-seen $?ls&:(timeout ?now ?ls ?*PEER-LOST-TIMEOUT*)))
  =>
  (modify ?rf (warning-sent TRUE))
  (printout warn "Robot " ?name " of " ?team " at " ?host " lost" crlf)
  (assert (attention-message (str-cat "Robot " ?name " of " ?team " at " ?host " lost") 10))
)

(defrule net-robot-remove
  (time $?now)
  ?rf <- (robot (team ?team) (name ?name) (host ?host) (port ?port)
		(last-seen $?ls&:(timeout ?now ?ls ?*PEER-REMOVE-TIMEOUT*)))
  =>
  (retract ?rf)
  (printout warn "Robot " ?name " of " ?team " at " ?host " definitely lost" crlf)
  (assert
   (attention-message (str-cat "Robot " ?name " of " ?team " at " ?host " definitely lost") 4))
)

(defrule send-attmsg
  ?af <- (attention-message ?message $?time-to-show)
  (network-client)
  =>
  (retract ?af)
  (bind ?attmsg (pb-create "llsf_msgs.AttentionMessage"))
  (pb-set-field ?attmsg "message" (str-cat ?message))
  (if (> (length$ ?time-to-show) 0) then
    (pb-set-field ?attmsg "time_to_show" (first$ ?time-to-show)))

  (do-for-all-facts ((?client network-client)) TRUE
		    (pb-send ?client:id ?attmsg))
  (pb-destroy ?attmsg)
)

(defrule discard-attmsg
  ?af <- (attention-message ?message $?time-to-show)
  (not (network-client))
  =>
  (retract ?af)
)

(defrule net-recv-SetGameState
  ?sf <- (gamestate (state ?state))
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGameState") (ptr ?p) (rcvd-via STREAM))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (modify ?sf (state (sym-cat (pb-field-value ?p "state"))) (prev-state ?state))
)

(defrule net-recv-SetGameState-illegal
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGameState") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (printout warn "Illegal SetGameState message received from host " ?host crlf)
)


(defrule net-recv-SetGamePhase
  ?sf <- (gamestate (phase ?phase))
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGamePhase") (ptr ?p))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (modify ?sf (phase (sym-cat (pb-field-value ?p "phase"))) (prev-phase ?phase))
)

(defrule net-recv-SetGamePhase-illegal
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGamePhase") (ptr ?p)
		       (rcvd-via BROADCAST) (rcvd-from ?host ?port))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (printout warn "Illegal SetGamePhase message received from host " ?host crlf)
)

(defrule net-recv-SetPucks
  ?mf <- (protobuf-msg (type "llsf_msgs.SetPucks") (ptr ?p))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  ; retract all puck facts, we store new ones
  (do-for-all-facts ((?puck puck)) TRUE (retract ?puck))
  ; store new pucks
  (foreach ?m (pb-field-list ?p "ids")
    (assert (puck (index ?m-index) (id ?m)))
  )
)

(defrule net-send-GameState
  (time $?now)
  (gamestate (state ?state) (phase ?phase) (game-time ?game-time) (points ?points))
  ?f <- (signal (type gamestate) (time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending GameState" crlf))
  (bind ?gamestate (pb-create "llsf_msgs.GameState"))
  (bind ?gamestate-time (pb-field-value ?gamestate "game_time"))
  (if (eq (type ?gamestate-time) EXTERNAL-ADDRESS) then 
    (bind ?gt (time-from-sec ?game-time))
    (pb-set-field ?gamestate-time "sec" (nth$ 1 ?gt))
    (pb-set-field ?gamestate-time "nsec" (integer (* (nth$ 2 ?gt) 1000)))
    (pb-set-field ?gamestate "game_time" ?gamestate-time) ; destroys ?gamestate-time!
  )
  (pb-set-field ?gamestate "state" (str-cat ?state))
  (pb-set-field ?gamestate "phase" (str-cat ?phase))
  (pb-set-field ?gamestate "points" ?points)
  (do-for-all-facts ((?client network-client)) TRUE
    (pb-send ?client:id ?gamestate)
  )
  (pb-broadcast ?gamestate)
  (pb-destroy ?gamestate)
)

(defrule net-send-RobotInfo
  (time $?now)
  ?f <- (signal (type robot-info) (time $?t&:(timeout ?now ?t ?*ROBOTINFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?ri (pb-create "llsf_msgs.RobotInfo"))

  (do-for-all-facts
    ((?robot robot)) TRUE

    (bind ?r (pb-create "llsf_msgs.Robot"))
    (bind ?r-time (pb-field-value ?r "last_seen"))
    (if (eq (type ?r-time) EXTERNAL-ADDRESS) then
      (pb-set-field ?r-time "sec" (nth$ 1 ?robot:last-seen))
      (pb-set-field ?r-time "nsec" (integer (* (nth$ 2 ?robot:last-seen) 1000)))
      (pb-set-field ?r "last_seen" ?r-time) ; destroys ?r-time!
    )
    ; If we have a pose publish it
    (if (non-zero-pose ?robot:pose) then
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
    (pb-set-field ?r "host" ?robot:host)
    (pb-add-list ?ri "robots" ?r) ; destroys ?r
  )

  (do-for-all-facts ((?client network-client)) TRUE
    (pb-send ?client:id ?ri))
  (pb-destroy ?ri)
)

(defrule net-send-MachineInfo
  (time $?now)
  ?sf <- (signal (type machine-info)
		 (time $?t&:(timeout ?now ?t ?*MACHINE-INFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)))
  (bind ?s (pb-create "llsf_msgs.MachineInfo"))

  (do-for-all-facts ((?machine machine)) TRUE
    (bind ?m (pb-create "llsf_msgs.Machine"))

    (pb-set-field ?m "name" ?machine:name)
    (pb-set-field ?m "type" ?machine:mtype)
    (do-for-fact ((?mspec machine-spec)) (eq ?mspec:mtype ?machine:mtype)
      (foreach ?puck ?mspec:inputs (pb-add-list ?m "inputs" (str-cat ?puck)))
      (pb-set-field ?m "output" (str-cat ?mspec:output))
    )
    (foreach ?puck-id ?machine:loaded-with
      (bind ?p (pb-create "llsf_msgs.Puck"))
      (do-for-fact ((?puck puck)) (eq ?puck:id ?puck-id)
        (pb-set-field ?p "id" ?puck:id)
	(pb-set-field ?p "state" ?puck:state)
      )
      (pb-add-list ?m "loaded_with" ?p)
    )
    (foreach ?l ?machine:actual-lights
      (bind ?ls (pb-create "llsf_msgs.LightSpec"))
      (bind ?dashidx (str-index "-" ?l))
      (bind ?color (sub-string 1 (- ?dashidx 1) ?l))
      (bind ?state (sub-string (+ ?dashidx 1) (str-length ?l) ?l))
      (pb-set-field ?ls "color" ?color)
      (pb-set-field ?ls "state" ?state)
      (pb-add-list ?m "lights" ?ls)
    )
    (if (<> ?machine:puck-id 0) then
      (bind ?p (pb-create "llsf_msgs.Puck"))
      (do-for-fact ((?puck puck)) (= ?puck:id ?machine:puck-id)
        (pb-set-field ?p "id" ?puck:id)
	(pb-set-field ?p "state" ?puck:state)
      )
      (pb-set-field ?m "puck_under_rfid" ?p)
    )
    ; If we have a pose publish it
    (if (non-zero-pose ?machine:pose) then
      (bind ?p (pb-field-value ?m "pose"))
      (bind ?p-time (pb-field-value ?p "timestamp"))
      (pb-set-field ?p-time "sec" (nth$ 1 ?machine:pose-time))
      (pb-set-field ?p-time "nsec" (integer (* (nth$ 2 ?machine:pose-time) 1000)))
      (pb-set-field ?p "timestamp" ?p-time)
      (pb-set-field ?p "x" (nth$ 1 ?machine:pose))
      (pb-set-field ?p "y" (nth$ 2 ?machine:pose))
      (pb-set-field ?p "ori" (nth$ 3 ?machine:pose))
      (pb-set-field ?m "pose" ?p)
    )
      
    ; In exploration phase, indicate whether this was correctly reported
    (do-for-fact ((?gs gamestate)) (eq ?gs:phase EXPLORATION)
      (do-for-fact ((?report exploration-report)) (eq ?report:name ?machine:name)
	(pb-set-field ?m "correctly_reported" (neq ?report:type WRONG))
      )
    )
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  (do-for-all-facts ((?client network-client)) TRUE
    (pb-send ?client:id ?s)
  )
  (pb-destroy ?s)
)

(defrule net-broadcast-MachineInfo
  (time $?now)
  (gamestate (phase PRODUCTION))
  ?sf <- (signal (type machine-info-bc) (seq ?seq) (count ?count)
		 (time $?t&:(timeout ?now ?t (if (> ?count ?*BC-MACHINE-INFO-BURST-COUNT*)
					       then ?*BC-MACHINE-INFO-PERIOD*
					       else ?*BC-MACHINE-INFO-BURST-PERIOD*))))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))
  (bind ?s (pb-create "llsf_msgs.MachineInfo"))

  (do-for-all-facts ((?machine machine)) TRUE
    (bind ?m (pb-create "llsf_msgs.Machine"))

    (pb-set-field ?m "name" ?machine:name)
    (pb-set-field ?m "type" ?machine:mtype)
    (do-for-fact ((?mspec machine-spec)) (eq ?mspec:mtype ?machine:mtype)
      (foreach ?puck ?mspec:inputs (pb-add-list ?m "inputs" (str-cat ?puck)))
      (pb-set-field ?m "output" (str-cat ?mspec:output))
    )
    ; If we have a pose publish it
    (if (non-zero-pose ?machine:pose) then
      (bind ?p (pb-field-value ?m "pose"))
      (bind ?p-time (pb-field-value ?p "timestamp"))
      (pb-set-field ?p-time "sec" (nth$ 1 ?machine:pose-time))
      (pb-set-field ?p-time "nsec" (integer (* (nth$ 2 ?machine:pose-time) 1000)))
      (pb-set-field ?p "timestamp" ?p-time)
      (pb-set-field ?p "x" (nth$ 1 ?machine:pose))
      (pb-set-field ?p "y" (nth$ 2 ?machine:pose))
      (pb-set-field ?p "ori" (nth$ 3 ?machine:pose))
      (pb-set-field ?m "pose" ?p)
    )
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  (pb-broadcast ?s)
  (pb-destroy ?s)
)


(defrule net-send-PuckInfo
  (time $?now)
  ?f <- (signal (type puck-info) (time $?t&:(timeout ?now ?t ?*PUCKINFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?pi (pb-create "llsf_msgs.PuckInfo"))

  (do-for-all-facts
    ((?puck puck)) TRUE

    (bind ?p (pb-create "llsf_msgs.Puck"))
    (pb-set-field ?p "id" ?puck:id)
    (pb-set-field ?p "state" (str-cat ?puck:state))
    ; If we have a pose publish it
    (if (non-zero-pose ?puck:pose) then
      (bind ?ps (pb-field-value ?p "pose"))
      (bind ?ps-time (pb-field-value ?ps "timestamp"))
      (pb-set-field ?ps-time "sec" (nth$ 1 ?puck:pose-time))
      (pb-set-field ?ps-time "nsec" (integer (* (nth$ 2 ?puck:pose-time) 1000)))
      (pb-set-field ?ps "timestamp" ?ps-time)
      (pb-set-field ?ps "x" (nth$ 1 ?puck:pose))
      (pb-set-field ?ps "y" (nth$ 2 ?puck:pose))
      (pb-set-field ?ps "ori" (nth$ 3 ?puck:pose))
      (pb-set-field ?p "pose" ?ps)
    )
    (pb-add-list ?pi "pucks" ?p) ; destroys ?p
  )

  (do-for-all-facts ((?client network-client)) TRUE
    (pb-send ?client:id ?pi))
  (pb-destroy ?pi)
)

(deffunction net-create-OrderInfo ()
  (bind ?oi (pb-create "llsf_msgs.OrderInfo"))

  (do-for-all-facts
    ((?order order)) (eq ?order:active TRUE)

    (bind ?o (pb-create "llsf_msgs.Order"))

    (pb-set-field ?o "id" ?order:id)
    (pb-set-field ?o "product" ?order:product)
    (pb-set-field ?o "quantity_requested" ?order:quantity-requested)
    (pb-set-field ?o "quantity_delivered" ?order:quantity-delivered)
    (pb-set-field ?o "delivery_gate" ?order:delivery-gate)
    (pb-set-field ?o "delivery_period_begin" (nth$ 1 ?order:delivery-period))
    (pb-set-field ?o "delivery_period_end"   (nth$ 2 ?order:delivery-period))
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
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))

  (bind ?oi (net-create-OrderInfo))
  (do-for-all-facts ((?client network-client)) TRUE
    (pb-send ?client:id ?oi))
  (pb-broadcast ?oi)
  (pb-destroy ?oi)
)


(defrule net-send-VersionInfo
  (time $?now)
  ?sf <- (signal (type version-info) (seq ?seq)
		 (count ?count&:(< ?count ?*BC-VERSIONINFO-COUNT*))
		 (time $?t&:(timeout ?now ?t ?*BC-VERSIONINFO-PERIOD*)))
  =>
  (modify ?sf (time ?now) (seq (+ ?seq 1)) (count (+ ?count 1)))
  (bind ?vi (net-create-VersionInfo))
  (pb-broadcast ?vi)
  (pb-destroy ?vi)
)
