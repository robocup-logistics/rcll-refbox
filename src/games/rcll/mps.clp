
;---------------------------------------------------------------------------
;  mps.clp - RCLL RefBox CLIPS mps controll
;
;  Created: Tue Jun 11 16:01:48 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;             2017  Tobias Neumann
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate mps
  (slot name (type STRING) (default "NOT-SET"))
  (slot master-host (type STRING))
  (slot master-port (type INTEGER))
  (slot client-id (type INTEGER) (default 0))
  (multislot last-disconnect (type INTEGER) (cardinality 2 2) (default 0 0))
  (slot state (type SYMBOL) (allowed-values DOWN UP CONNECTING WAIT-RECONNECT) (default DOWN))
)

(defrule mps-enable
  (declare (salience ?*PRIORITY_FIRST*))
  (init)
  (config-loaded)
;  (confval (path "/llsfrb/mps/enable") (type BOOL) (value true))
  ?mps <- (machine (name ?machine) (comm-cfg-checked false))
  (confval (path =(str-cat "/llsfrb/mps/stations/" ?machine "/active")) (type BOOL) (value true))
  (confval (path =(str-cat "/llsfrb/mps/stations/" ?machine "/host")) (type STRING) (value ?host))
  (confval (path =(str-cat "/llsfrb/mps/stations/" ?machine "/port")) (type UINT) (value ?port))
  =>
  (assert (mps (name (str-cat ?machine)) (master-host ?host) (master-port ?port)))
  (modify ?mps (comm-cfg-checked true))
)

(defrule mps-not-enable
  (declare (salience ?*PRIORITY_FIRST*))
  (init)
  (config-loaded)
;  (confval (path "/llsfrb/mps/enable") (type BOOL) (value true))
  ?mps <- (machine (name ?machine) (comm-cfg-checked false))
  (confval (path ?pa&:(eq ?pa (str-cat "/llsfrb/mps/stations/" ?machine "/active"))) (type BOOL) (value false))
  =>
  (printout warn "Do not enable " ?machine crlf)
  (modify ?mps (comm-cfg-checked true))
)

(defrule mps-finalize
  ?sf <- (mps)
  (finalize)
  =>
  (retract ?sf)
)

(defrule mps-connect
  ?sf <- (mps (name ?n) (client-id 0) (master-host ?host) (master-port ?port) (state DOWN))
  =>
  (bind ?client-id (pb-connect ?host ?port))
  (modify ?sf (client-id ?client-id) (state CONNECTING))
  (printout t "Connecting to mps " ?n " @ " ?host ":" ?port crlf)
)

(defrule mps-connected
  ?sf <- (mps (name ?n) (client-id ?ci&~0) (master-host ?host) (master-port ?port))
  ?cf <- (protobuf-client-connected ?ci)
  =>
  (retract ?cf)
  (modify ?sf (state UP))
;  (pb-send ?ci (pb-create "llsf_msgs.InitiateSync"))
  (printout t "Connected to mps " ?n " @ " ?host ":" ?port  crlf)
)

(defrule mps-reconnect
  (time $?now)
  ?sf <- (mps (client-id 0) (state WAIT-RECONNECT)
	       (last-disconnect $?lc&:(timeout ?now ?lc ?*MPS-RECONNECT-PERIOD*)))
  =>
  (modify ?sf (state DOWN))
)


(defrule mps-disconnected
  ?sf <- (mps (name ?n) (client-id ?ci&~0) (master-host ?host) (master-port ?port))
  ?cf <- (protobuf-client-disconnected ?ci)
  =>
  (retract ?cf)
  (modify ?sf (client-id 0) (state WAIT-RECONNECT) (last-disconnect (now)))
;  (pb-disconnect ?ci)
  (printout warn "Disconnected from mps " ?n " @ " ?host ":" ?port crlf)
)

