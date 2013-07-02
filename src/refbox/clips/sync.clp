
;---------------------------------------------------------------------------
;  sync.clp - LLSF RefBox CLIPS multi-refbox synchronization
;
;  Created: Tue Jun 11 16:01:48 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate sync
  (slot master-host (type STRING))
  (slot master-port (type INTEGER))
  (slot client-id (type INTEGER))
  (multislot last-disconnect (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot state (type SYMBOL) (allowed-values DOWN UP CONNECTING WAIT-RECONNECT) (default DOWN))
)

(defrule sync-enable
  (declare (salience ?*PRIORITY_FIRST*))
  (init)
  (confval (path "/llsfrb/sync/enable") (type BOOL) (value true))
  (confval (path "/llsfrb/sync/master-host") (type STRING) (value ?host))
  (confval (path "/llsfrb/sync/master-port") (type UINT) (value ?port))
  =>
  (assert (sync (master-host ?host) (master-port ?port)))
)

(defrule sync-finalize
  ?sf <- (sync)
  (finalize)
  =>
  (retract ?sf)
)

(defrule sync-connect
  ?sf <- (sync (client-id 0) (master-host ?host) (master-port ?port) (state DOWN))
  =>
  (bind ?client-id (pb-connect ?host ?port))
  (modify ?sf (client-id ?client-id) (state CONNECTING))
  (printout t "Connecting to master refbox @ " ?host ":" ?port crlf)
)

(defrule sync-connected
  ?sf <- (sync (client-id ?ci&~0))
  ?cf <- (protobuf-client-connected ?ci)
  =>
  (retract ?cf)
  (modify ?sf (state UP))
  (pb-send ?ci (pb-create "llsf_msgs.InitiateSync"))
  (printout t "Connected to master refbox" crlf)
)

(defrule sync-reconnect
  (time $?now)
  ?sf <- (sync (client-id 0) (state WAIT-RECONNECT)
	       (last-disconnect $?lc&:(timeout ?now ?lc ?*SYNC-RECONNECT-PERIOD*)))
  =>
  (modify ?sf (state DOWN))
)


(defrule sync-disconnected
  ?sf <- (sync (client-id ?ci&~0))
  ?cf <- (protobuf-client-disconnected ?ci)
  =>
  (retract ?cf)
  (modify ?sf (client-id 0) (state WAIT-RECONNECT) (last-disconnect (now)))
  (pb-disconnect ?ci)
  (printout warn "Disconnected from master refbox" crlf)
)


(defrule sync-master-initiate
  ?mf <- (protobuf-msg (type "llsf_msgs.InitiateSync") (ptr ?p) (rcvd-via STREAM)
		       (client-id ?client-id))
  ?cf <- (network-client (id ?client-id) (is-slave FALSE))
  =>
  (printout t "Client " ?client-id " is a sync slave" crlf)
  (retract ?mf)
  (modify ?cf (is-slave TRUE))
)

(defrule sync-is-master
  ?gf <- (gamestate (refbox-mode STANDALONE))
  (exists (network-client (is-slave TRUE)))
  =>
  (modify ?gf (refbox-mode MASTER))
)

(defrule sync-is-no-longer-master
  ?gf <- (gamestate (refbox-mode MASTER))
  (not (exists (network-client (is-slave TRUE))))
  =>
  (modify ?gf (refbox-mode STANDALONE))
)

(defrule sync-is-slave
  ?gf <- (gamestate (refbox-mode STANDALONE))
  (sync (state UP))
  =>
  (modify ?gf (refbox-mode SLAVE))
)

(defrule sync-is-no-longer-slave
  ?gf <- (gamestate (refbox-mode SLAVE))
  (sync (state ~UP))
  =>
  (modify ?gf (refbox-mode STANDALONE))
)

(defrule sync-master-send-SyncInfo
  (game-parameterized)
  (exists (network-client (is-slave TRUE)))
  =>
  (bind ?s (pb-create "llsf_msgs.SyncInfo"))

  ; Add machine specifications
  (do-for-all-facts ((?machine machine)) TRUE
    (bind ?m (pb-create "llsf_msgs.MachineTypeSpec"))
    (pb-set-field ?m "name" (str-cat ?machine:name))
    (pb-set-field ?m "type" (str-cat ?machine:mtype))
    (pb-add-list ?s "machines" ?m) ; destroys ?m
  )

  ; Add machine type light signals
  (do-for-all-facts ((?ms machine-spec) (?lc machine-light-code))
    (eq ?ms:light-code ?lc:id)

    (bind ?mls (pb-create "llsf_msgs.MachineLightSpec"))
    (pb-set-field ?mls "machine_type" (str-cat ?ms:mtype))
    (foreach ?l ?lc:code
      (bind ?ls (pb-create "llsf_msgs.LightSpec"))
      (bind ?dashidx (str-index "-" ?l))
      (bind ?color (sub-string 1 (- ?dashidx 1) ?l))
      (bind ?state (sub-string (+ ?dashidx 1) (str-length ?l) ?l))
      (pb-set-field ?ls "color" ?color)
      (pb-set-field ?ls "state" ?state)
      (pb-add-list ?mls "lights" ?ls)
    )
    (pb-add-list ?s "exploration_lights" ?mls)
  )

  ; Processing times
  (do-for-all-facts ((?ms machine-spec)) TRUE
    (bind ?mpt (pb-create "llsf_msgs.MachineProcTime"))
    (pb-set-field ?mpt "machine_type" ?ms:mtype)
    (pb-set-field ?mpt "proc_time" ?ms:proc-time)
    (pb-add-list ?s "proc_times" ?mpt)
  )

  ; Add order specification
  (do-for-all-facts ((?order order)) TRUE
    (bind ?o (net-create-Order ?order))
    (pb-set-field ?o "late_order" ?order:late-order)
    (pb-add-list ?s "orders" ?o) ; destroys ?o
  )

  ; Add machine down times
  (do-for-all-facts ((?machine machine))
    (and (>= (nth$ 1 ?machine:down-period) 0.0) (>= (nth$ 2 ?machine:down-period) 0.0))

    (printout t "Adding down time for " ?machine:name ": " ?machine:down-period crlf)
    (bind ?ms (pb-create "llsf_msgs.MachineTimeSpec"))
    (pb-set-field ?ms "machine_name" ?machine:name)
    (pb-set-field ?ms "gt_from" (nth$ 1 ?machine:down-period))
    (pb-set-field ?ms "gt_to" (nth$ 2 ?machine:down-period))
    (pb-add-list ?s "down_times" ?ms)
  )

  ; Add delivery gate times
  (do-for-all-facts ((?dp delivery-period)) TRUE
    (bind ?ms (pb-create "llsf_msgs.MachineTimeSpec"))
    (pb-set-field ?ms "machine_name" (str-cat ?dp:delivery-gate))
    (pb-set-field ?ms "gt_from" (nth$ 1 ?dp:period))
    (pb-set-field ?ms "gt_to" (nth$ 2 ?dp:period))
    (pb-add-list ?s "delivery_gates" ?ms)
  )

  (do-for-all-facts ((?client network-client)) ?client:is-slave
    (printout t "Sending SyncInfo to client " ?client:id crlf)
    (pb-send ?client:id ?s)
  )
  (pb-destroy ?s)
)


(defrule sync-slave-receive
  ?mf <- (protobuf-msg (type "llsf_msgs.SyncInfo") (ptr ?p) (rcvd-via STREAM))
  (sync (state UP))
  =>
  (printout t "SyncInfo received" crlf)
  (assert (game-parameterized))
  (assert (machines-initialized))

  ; Set machine types
  ;(printout t "Setting machine types" crlf)
  (foreach ?m (pb-field-list ?p "machines")
    (do-for-fact ((?machine machine))
      (eq ?machine:name (sym-cat (pb-field-value ?m "name")))

      (modify ?machine (mtype (sym-cat (pb-field-value ?m "type"))))
    )
  )

  ; Orders
  ;(printout t "Adding orders" crlf)
  (delayed-do-for-all-facts ((?order order)) TRUE (retract ?order))
  (foreach ?o (pb-field-list ?p "orders")
    (bind ?delivery-begin (pb-field-value ?o "delivery_period_begin"))
    (bind ?activate-at (max (- ?delivery-begin ?*LATE-ORDER-ACTIVATION-PRE-TIME*) 0))
    (bind ?points 10)
    (if (pb-field-value ?o "late_order") then (bind ?points 20))

    (assert (order (id (pb-field-value ?o "id"))
		   (product (sym-cat (pb-field-value ?o "product")))
		   (late-order (pb-field-value ?o "late_order"))
		   (points ?points)
		   (quantity-requested (pb-field-value ?o "quantity_requested"))
		   (delivery-period ?delivery-begin (pb-field-value ?o "delivery_period_end"))
		   (activate-at ?activate-at)
		   (active (not (pb-field-value ?o "late_order")))
		   (delivery-gate (sym-cat (pb-field-value ?o "delivery_gate")))))
  )


  ; down times
  ;(printout t "Setting down times" crlf)
  (foreach ?dt (pb-field-list ?p "down_times")
    (do-for-fact ((?machine machine))
      (eq ?machine:name (sym-cat (pb-field-value ?dt "machine_name")))

      (modify ?machine (down-period (pb-field-value ?dt "gt_from")
				    (pb-field-value ?dt "gt_to")))
    )
  )

  ; delivery gate times
  ;(printout t "Setting delivery gate times" crlf)
  (delayed-do-for-all-facts ((?dp delivery-period)) TRUE (retract ?dp))
  (foreach ?dp (pb-field-list ?p "delivery_gates")
    (assert (delivery-period (delivery-gate (sym-cat (pb-field-value ?dp "machine_name")))
			     (period (pb-field-value ?dp "gt_from")
				     (pb-field-value ?dp "gt_to"))))
  )

  ; processing times
  ;(printout t "Setting processing times" crlf)
  (foreach ?pt (pb-field-list ?p "proc_times")
    (do-for-fact ((?mspec machine-spec)) (eq ?mspec:mtype (pb-field-value ?pt "machine_type"))
      (modify ?mspec (proc-time (pb-field-value ?pt "proc_time")))
    )
  )

  ; Exploration light codes
  ;(printout t "Setting exploration light codes" crlf)
  (foreach ?el (pb-field-list ?p "exploration_lights")
    (bind ?lights (create$))
    (progn$ (?l (pb-field-list ?el "lights"))
      (bind ?light (sym-cat (pb-field-value ?l "color") "-" (pb-field-value ?l "state")))
      (bind ?lights (append$ ?lights ?light))
    )

    (bind ?lc-id 0)
    (if (any-factp ((?lc machine-light-code))
		   (and (subsetp ?lights ?lc:code) (subsetp ?lc:code ?lights)))
    then
      (do-for-fact ((?lc machine-light-code))
        (and (subsetp ?lights ?lc:code) (subsetp ?lc:code ?lights))

        (bind ?lc-id ?lc:id)
      )
    else
      (printout t "Unknown light code " ?lights " received, creating a new one" crlf)
      (do-for-fact ((?lc machine-light-code)) TRUE
        (bind ?lc-id (max ?lc-id ?lc:id))
      )
      (bind ?lc-id (+ ?lc-id 1))
      (assert (machine-light-code (id ?lc-id) (code ?lights)))
    )

    ;(printout t "Setting light code ID " ?lc-id " for " (pb-field-value ?el "machine_type") crlf)
    (do-for-fact ((?mspec machine-spec))
      (eq ?mspec:mtype (sym-cat (pb-field-value ?el "machine_type")))

      (modify ?mspec (light-code ?lc-id))
    )
  )
)

(deffunction sync-send-GameState-to-slaves (?state)
  (bind ?sgs (pb-create "llsf_msgs.SetGameState"))
  (pb-set-field ?sgs "state" ?state)
  (do-for-all-facts ((?client network-client)) ?client:is-slave
    (pb-send ?client:id ?sgs)
  )
  (pb-destroy ?sgs)
)

(defrule sync-master-start-game
  (gamestate (state RUNNING) (prev-state WAIT_START))
  (exists (network-client (is-slave TRUE)))
  (game-parameterized)
  (not (sync-game-started))
  =>
  (assert (sync-game-started))
  (sync-send-GameState-to-slaves RUNNING)
)


(defrule sync-slave-send-GameState
  (time $?now)
  ?gs <- (gamestate (refbox-mode SLAVE))
  (sync (client-id ?client-id) (state UP))
  ?f <- (signal (type sync-gamestate)
		(time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?gamestate (net-create-GameState ?gs))
  (pb-send ?client-id ?gamestate)
  (pb-destroy ?gamestate)
)


(defrule sync-master-send-GameState
  (time $?now)
  ?gs <- (gamestate (refbox-mode MASTER))
  ?f <- (signal (type sync-gamestate)
		(time $?t&:(timeout ?now ?t ?*GAMESTATE-PERIOD*)) (seq ?seq))
  =>
  (modify ?f (time ?now) (seq (+ ?seq 1)))
  (bind ?gamestate (net-create-GameState ?gs))
  (do-for-all-facts ((?client network-client)) ?client:is-slave
    (pb-send ?client:id ?gamestate)
  )
  (pb-destroy ?gamestate)
)

(defrule sync-master-recv-GameState
  ?gs <- (gamestate (refbox-mode MASTER) (phase PRODUCTION) (state PAUSED) (points ?points)
		    (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  ?mf <- (protobuf-msg (type "llsf_msgs.GameState") (ptr ?p) (rcvd-via STREAM))
  =>
  (retract ?mf)
  (bind ?p-gtime  (pb-field-value ?p "game_time"))

  (bind ?r-phase  (pb-field-value ?p "phase"))
  (bind ?r-state  (pb-field-value ?p "state"))
  (bind ?r-points (pb-field-value ?p "points"))
  (bind ?r-gtime  (time-to-sec (create$ (pb-field-value ?p-gtime "sec")
					(/ (pb-field-value ?p-gtime "nsec") 1000))))

  ;(printout t "Remote game: " ?r-phase " " ?r-state " " ?r-gtime crlf)

  (if (and (eq ?r-phase PRODUCTION) (eq ?r-state PAUSED) (> ?r-gtime ?*PRODUCTION-TIME*))
    then ; remote has finished regular game time as well
    (if (and (= ?r-points ?points) (<> ?points 0))
      then ; equal but non-zero points, extension time
        (assert (attention-message "Entering overtime" 5))
	(modify ?gs (state RUNNING) (prev-state PAUSED) (over-time TRUE))
	(sync-send-GameState-to-slaves RUNNING)

      else ; teams have differing or both have zero points, game is over
        (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION))
    )
    ; else game still running on other side
  )
)
  

(defrule sync-slave-recv-GameState
  (declare (salience ?*PRIORITY_HIGH*))
  ?gs <- (gamestate (refbox-mode SLAVE) (phase PRODUCTION) (state PAUSED)
		    (over-time FALSE)
		    (game-time ?game-time&:(>= ?game-time ?*PRODUCTION-TIME*)))
  (sync (client-id ?client-id))
  ?mf <- (protobuf-msg (type "llsf_msgs.GameState") (ptr ?p)
		       (rcvd-via STREAM) (client-id ?client-id))
  =>
  (retract ?mf)
  (bind ?p-gtime  (pb-field-value ?p "game_time"))

  (bind ?r-phase  (pb-field-value ?p "phase"))
  (bind ?r-state  (pb-field-value ?p "state"))
  (bind ?r-points (pb-field-value ?p "points"))
  (bind ?r-gtime  (time-to-sec (create$ (pb-field-value ?p-gtime "sec")
					(/ (pb-field-value ?p-gtime "nsec") 1000))))

  ;(printout t "Remote game: " ?r-phase " " ?r-state " " ?r-gtime crlf)

  (if (and (eq ?r-phase PRODUCTION) (eq ?r-state RUNNING) (> ?r-gtime ?*PRODUCTION-TIME*))
   then ; master has decided for overtime
    (modify ?gs (over-time TRUE) (state RUNNING))
    (assert (attention-message "Entering overtime" 5))
  )
  (if (and (eq ?r-phase POST_GAME) (> ?r-gtime ?*PRODUCTION-TIME*))
   then ; master has decided for NO overtime
    (modify ?gs (phase POST_GAME) (prev-phase PRODUCTION))
  )
  ; else still waiting for remote side to complete game
)

