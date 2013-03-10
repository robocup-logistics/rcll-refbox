
;---------------------------------------------------------------------------
;  protobuf.clp - protobuf message templates
;
;  Created: Fri Feb 08 15:42:52 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate protobuf-msg
  (slot type (type STRING))
  (slot comp-id (type INTEGER))
  (slot msg-type (type INTEGER))
  (slot rcvd-via (type SYMBOL) (allowed-values STREAM BROADCAST))
  (multislot rcvd-from (cardinality 2 2))
  (slot client-id (type INTEGER))
  (slot ptr (type EXTERNAL-ADDRESS))
)

(deffunction pb-is-broadcast (?rcvd-via)
  (eq ?rcvd-via BROADCAST)
)

; (defrule protobuf-client-connected
;   (declare (salience 1))
;   (protobuf-client-connected ?id ?host ?port)
;   =>
;   (if (debug 2) then
;     (printout t "Client " ?id " connected from " ?host ":" ?port crlf))
; )

; (defrule protobuf-client-disconnected
;   (declare (salience 1))
;   (protobuf-client-disconnected ?id)
;   =>
;   (if (debug 2) then (printout t "Client " ?id " disconnected" crlf))
; )

