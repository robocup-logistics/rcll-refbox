
;---------------------------------------------------------------------------
;  protobuf.clp - protobuf message templates
;
;  Created: Fri Feb 08 15:42:52 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate protobuf-msg
  (slot type (type SYMBOL))
  (slot comp-id (type INTEGER))
  (slot msg-type (type INTEGER))
  (slot rcvd-via (type SYMBOL) (allowed-values STREAM BROADCAST))
  (multislot rcvd-from (type STRING) (cardinality 2 2))
  (slot ptr (type EXTERNAL-ADDRESS))
)

(defrule protobuf-client-connected
  ?f <- (protobuf-client-connected ?id ?host ?port)
  =>
  (retract ?f)
  (if (debug 2) then
    (printout t "Protobuf client " ?id " connected from " ?host ":" ?port crlf))
)

(defrule protobuf-client-disconnected
  ?f <- (protobuf-client-disconnected ?id)
  =>
  (retract ?f)
  (if (debug 2) then (printout t "Protobuf client " ?id " disconnected" crlf))
)

(defrule print-protobuf-msg
  ?mf <- (protobuf-msg (type ?t) (ptr ?p) (rcvd-from ?from-host ?from-port))
  =>
  (retract ?mf)
  (printout t "Got message of type " ?t " (ptr " ?p ") from " ?from-host ":" ?from-port crlf)
  (foreach ?f (pb-field-names ?p)
	   (if (pb-field-is-list ?p ?f) then
	     (bind ?v (pb-field-list ?p ?f))
	    else
	     (bind ?v (pb-field-value ?p ?f))
	   )
	   (printout t "  " ?f " [" (pb-field-label ?p ?f)
		     ", " (pb-field-type ?p ?f) ", " (type ?v) "]: " ?v crlf))
)
