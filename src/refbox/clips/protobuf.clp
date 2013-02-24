
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

(deffunction pb-client-id ($?rcvd-from)
  (if (>= (length$ ?rcvd-from) 3) then (nth$ 3 ?rcvd-from) else 0)
)

(deffunction pb-is-broadcast (?rcvd-via)
  (eq ?rcvd-via BROADCAST)
)

(defrule protobuf-client-connected
  (declare (salience 1))
  (protobuf-client-connected ?id ?host ?port)
  =>
  (if (debug 2) then
    (printout t "Protobuf client " ?id " connected from " ?host ":" ?port crlf))
)

(defrule protobuf-client-disconnected
  (declare (salience 1))
  (protobuf-client-disconnected ?id)
  =>
  (if (debug 2) then (printout t "Protobuf client " ?id " disconnected" crlf))
)


;(defrule print-protobuf-msg
;  ?mf <- (protobuf-msg (type ?t) (ptr ?p) (rcvd-from ?from-host ?from-port)
;		       (rcvd-via ?via) (client-id ?client-id))
;  =>
;  (retract ?mf)
;  (printout t "Got message of type " ?t " (ptr " ?p ") from " ?from-host ":" ?from-port crlf)
;  (foreach ?f (pb-field-names ?p)
;	   (if (pb-field-is-list ?p ?f) then
;	     (bind ?v (pb-field-list ?p ?f))
;	    else
;	     (bind ?v (pb-field-value ?p ?f))
;	   )
;	   (printout t "  " ?f " [" (pb-field-label ?p ?f)
;		     ", " (pb-field-type ?p ?f) ", " (type ?v) "]: " ?v crlf))
;  (bind ?reply (pb-create "llsf_msgs.Person"))
;  (pb-set-field ?reply "id" 123)
;  (pb-set-field ?reply "name" (pb-field-value ?p "name"))
;  (pb-set-field ?reply "email" (pb-field-value ?p "email"))
;  (pb-set-field ?reply "has_dog" TRUE)
;  (printout t "Reply " ?reply "  id: " (pb-field-value ?reply "id") crlf)
;  (pb-send ?reply ?client-id)
;  (pb-destroy ?reply)
;)

