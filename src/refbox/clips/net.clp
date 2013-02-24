
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
  (points ?points)
  (network-client (id ?client-id))
  ?f <- (signal (type gamestate) (time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (if (debug 3) then (printout t "Sending GameState" crlf))
  (bind ?gamestate (pb-create "llsf_msgs.GameState"))
  (bind ?gamestate-time (pb-field-value ?gamestate "timestamp"))
  (if (eq (type ?gamestate-time) EXTERNAL-ADDRESS) then 
    (pb-set-field ?gamestate-time "sec" (nth$ 1 ?now))
    (pb-set-field ?gamestate-time "nsec" (* (nth$ 2 ?now) 1000))
    (pb-set-field ?gamestate "timestamp" ?gamestate-time) ; destroys ?gamestate-time!
  )
  (pb-set-field ?gamestate "state" (str-cat ?state))
  (pb-set-field ?gamestate "points" ?points)
  (pb-send ?client-id ?gamestate)
  (pb-destroy ?gamestate)
)

