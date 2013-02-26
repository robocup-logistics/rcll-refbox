
;---------------------------------------------------------------------------
;  net.clp - LLSF RefBox CLIPS network handling
;
;  Created: Thu Feb 14 17:26:27 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------


(defrule net-client-connected
  ?cf <- (protobuf-client-connected ?client-id ?host ?port)
  =>
  (retract ?cf)
  (assert (network-client (id ?client-id) (host ?host) (port ?port)))
)

(defrule net-client-disconnected
  ?cf <- (protobuf-client-disconnected ?client-id)
  ?nf <- (network-client (id ?client-id))
  =>
  (retract ?cf ?nf)
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
  (pb-set-field ?beacon-time "nsec" (* (nth$ 2 ?now) 1000))
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
  (printout t "Received beacon from known " ?from-host ":" ?from-port crlf)
  (bind ?time (pb-field-value ?p "time"))
  (modify ?rf (last-seen (create$ (pb-field-value ?time "sec")
				  (/ (pb-field-value ?time "nsec") 1000))))
)

(defrule net-recv-beacon-unknown
  (declare (salience -10))
  (time $?now)
  ?mf <- (protobuf-msg (type "llsf_msgs.BeaconSignal") (ptr ?p)
		       (rcvd-from ?from-host ?from-port) (rcvd-via ?via))
  =>
  (retract ?mf) ; message will be destroyed after rule completes
  (printout t "Received beacon from " ?from-host ":" ?from-port crlf)
  (bind ?time (pb-field-value ?p "time"))
  (assert (robot (team (pb-field-value ?p "team_name"))
		 (name (pb-field-value ?p "peer_name"))
		 (host ?from-host) (port ?from-port)
		 (last-seen (create$ (pb-field-value ?time "sec")
				     (/ (pb-field-value ?time "nsec") 1000)))))
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
  ?sf <- (state ?state)
  ?mf <- (protobuf-msg (type "llsf_msgs.SetGameState") (ptr ?p)
		       (rcvd-from ?from-host ?from-port)
		       (rcvd-via ?via) (client-id ?client-id))
  =>
  (retract ?mf ?sf) ; message will be destroyed after rule completes
  (assert (state (sym-cat (pb-field-value ?p "state"))))
)

(defrule net-send-GameState
  (time $?now)
  (state ?state)
  (gamestate (game-time ?game-time) (points ?points))
  (network-client (id ?client-id))
  ?f <- (signal (type gamestate) (time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending GameState" crlf))
  (bind ?gamestate (pb-create "llsf_msgs.GameState"))
  (bind ?gamestate-time (pb-field-value ?gamestate "timestamp"))
  (if (eq (type ?gamestate-time) EXTERNAL-ADDRESS) then 
    (bind ?gt (time-from-sec ?game-time))
    (pb-set-field ?gamestate-time "sec" (nth$ 1 ?gt))
    (pb-set-field ?gamestate-time "nsec" (* (nth$ 2 ?gt) 1000))
    (pb-set-field ?gamestate "timestamp" ?gamestate-time) ; destroys ?gamestate-time!
  )
  (pb-set-field ?gamestate "state" (str-cat ?state))
  (pb-set-field ?gamestate "points" ?points)
  (pb-send ?client-id ?gamestate)
  (pb-destroy ?gamestate)
)

(defrule net-send-RobotInfo
  (time $?now)
  (network-client (id ?client-id))
  ?f <- (signal (type robot-info) (time $?t&:(timeout ?now ?t ?*ROBOTINFO-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?p (pb-create "llsf_msgs.RobotInfo"))

  (do-for-all-facts
    ((?robot robot)) TRUE

    (bind ?r (pb-create "llsf_msgs.Robot"))
    (bind ?r-time (pb-field-value ?r "last_seen"))
    (if (eq (type ?r-time) EXTERNAL-ADDRESS) then 
      (pb-set-field ?r-time "sec" (nth$ 1 ?robot:last-seen))
      (pb-set-field ?r-time "nsec" (* (nth$ 2 ?robot:last-seen) 1000))
      (pb-set-field ?r "last_seen" ?r-time) ; destroys ?r-time!
    )
    (pb-set-field ?r "name" ?robot:name)
    (pb-set-field ?r "team" ?robot:team)
    (pb-add-list ?p "robots" ?r) ; destroys ?r
  )

  (pb-send ?client-id ?p)
  (pb-destroy ?p)
)

(defrule net-send-MachineSpecs
  (network-client (id ?client-id))
  (machine)
  =>
  (bind ?s (pb-create "llsf_msgs.MachineSpecs"))

  (do-for-all-facts
    ((?machine machine)) TRUE

    (bind ?m (pb-create "llsf_msgs.MachineSpec"))

    (pb-set-field ?m "name" ?machine:name)
    (pb-set-field ?m "type" ?machine:mtype)
    (do-for-fact ((?mspec machine-spec)) (eq ?mspec:mtype ?machine:mtype)
      (foreach ?puck ?mspec:inputs (pb-add-list ?m "inputs" (str-cat ?puck)))
      (pb-set-field ?m "output" (str-cat ?mspec:output))
    )
    (foreach ?puck ?machine:loaded-with (pb-add-list ?m "loaded_with" (str-cat ?puck)))
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
      (do-for-fact ((?puck puck)) (= ?puck:id ?machine:puck-id)
		   (pb-set-field ?m "puck_under_rfid" ?puck:state))
    )
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  (pb-send ?client-id ?s)
  (pb-destroy ?s)
)
