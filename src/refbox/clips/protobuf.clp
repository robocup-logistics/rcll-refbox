
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

(defrule print-protobuf-msg
  ?mf <- (protobuf-msg (type ?t) (ptr ?p))
  =>
  (printout t "Asserted message of type " ?t " (ptr " ?p ")" crlf)
  (retract ?mf)
)
