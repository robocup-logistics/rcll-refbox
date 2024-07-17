
; Licensed under GPLv2. See LICENSE file. Copyright TC of the RoboCup Logistics League

;---------------------------------------------------------------------------
;  config.clp - CLIPS agent configuration tools
;
;  Created: Wed Dec 19 20:45:53 2012 (Train from Munich to Freiburg)
;  Copyright  2012  Tim Niemueller [www.niemueller.de]
;  Licensed under GPLv2+ license, cf. LICENSE file
;---------------------------------------------------------------------------

(deftemplate confval
  (slot path (type STRING))
  (slot type (type SYMBOL) (allowed-values FLOAT UINT INT BOOL STRING))
  (slot value)
  (slot is-list (type SYMBOL) (allowed-values TRUE FALSE) (default FALSE))
  (multislot list-value)
)

(defrule config-print-disabled-slide-counter-checks
	(machine (name ?n) (mtype RS))
	(confval (path ?p&:(eq ?p (str-cat"/llsfrb/mps/stations/" ?n "/connection")))
	         (type STRING) (is-list FALSE) (value "mockup"))
	(not (disabled-slide-printed ?n))
	=>
  (printout warn "Slide counter check for "
                 ?n " disabled, as it is using mockup mode" crlf)
	(assert (disabled-slide-printed ?n))
)

(defrule config-update-order-count
  (order)
  ?cf <- (confval (path "/llsfrb/globals/number-of-orders") (value ?count))
  =>
  (bind ?orders (find-all-facts ((?o order)) TRUE))
  (bind ?new-count (length$ ?orders))
  (if (neq ?count ?new-count) then
    (modify ?cf (value ?new-count))
  )
)

(defrule config-sync-config-with-challenges-width
  "If the challange mode is enabled, synchronize overlapping configs"
  (confval (path "/llsfrb/challenges/enable") (type BOOL) (value TRUE))
  ?cv <- (confval (path "/llsfrb/game/field/width") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-width") (value ?v2))
  (confval (path "/llsfrb/challenges/field/width") (value ?new-v&:(neq ?v ?new-v)))
  =>
  (modify ?cv (value ?new-v))
  (modify ?cv2 (value ?new-v))
)

(defrule config-sync-config-with-challenges-height
  "If the challange mode is enabled, synchronize overlapping configs"
  (confval (path "/llsfrb/challenges/enable") (type BOOL) (value TRUE))
  ?cv <- (confval (path "/llsfrb/game/field/height") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-height") (value ?v2))
  (confval (path "/llsfrb/challenges/field/height") (value ?new-v&:(neq ?v ?new-v)))
  =>
  (modify ?cv (value ?new-v))
  (modify ?cv2 (value ?new-v))
)

(defrule config-sync-config-with-challenges-mirror
  "If the challange mode is enabled, synchronize overlapping configs
   Current configs have a bug as mirroring is called mirror, keep this rule for now.
  "
  (confval (path "/llsfrb/challenges/enable") (type BOOL) (value TRUE))
  ?cv <- (confval (path "/llsfrb/game/field/mirrored") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-mirrored") (value ?v2))
  (confval (path "/llsfrb/challenges/field/mirror") (value ?new-v&:(neq ?v ?new-v)))
  =>
  (modify ?cv (value ?new-v))
  (modify ?cv2 (value ?new-v))
)

(defrule config-sync-config-with-challenges-mirrored
  "If the challange mode is enabled, synchronize overlapping configs"
  (confval (path "/llsfrb/challenges/enable") (type BOOL) (value TRUE))
  ?cv <- (confval (path "/llsfrb/game/field/mirrored") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-mirrored") (value ?v2))
  (confval (path "/llsfrb/challenges/field/mirrored") (value ?new-v&:(neq ?v ?new-v)))
  =>
  (modify ?cv (value ?new-v))
  (modify ?cv2 (value ?new-v))
)

; Some of the globals actually directly resemble a config value.
; In that case synchronize the values so that the actual global
; variables are always up-to-date
(defrule config-sync-config-with-global-config-width
  ?cv  <- (confval (path "/llsfrb/game/field/width") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-width") (value ?v2))
  (test (neq ?v ?v2))
  =>
  (if (< (fact-index ?cv) (fact-index ?cv2)) then
    (modify ?cv (value ?v2))
  else
    (modify ?cv2 (value ?v))
  )
)

(defrule config-sync-config-with-global-config-height
  ?cv  <- (confval (path "/llsfrb/game/field/height") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-height") (value ?v2))
  (test (neq ?v ?v2))
  =>
  (if (< (fact-index ?cv) (fact-index ?cv2)) then
    (modify ?cv (value ?v2))
  else
    (modify ?cv2 (value ?v))
  )
)

(defrule config-sync-config-with-global-config-mirrored
  ?cv  <- (confval (path "/llsfrb/game/field/mirrored") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/field-mirrored") (value ?v2))
  (test (neq ?v ?v2))
  =>
  (if (< (fact-index ?cv) (fact-index ?cv2)) then
    (modify ?cv (value ?v2))
  else
    (modify ?cv2 (value ?v))
  )
)

(defrule config-sync-config-with-global-config-exploriation-time
  ?cv  <- (confval (path "/llsfrb/game/exploration-time") (value ?v))
  ?cv2 <- (confval (path "/llsfrb/globals/exploration-time") (value ?v2))
  (test (neq ?v ?v2))
  =>
  (if (< (fact-index ?cv) (fact-index ?cv2)) then
    (modify ?cv (value ?v2))
  else
    (modify ?cv2 (value ?v))
  )
)

; ----------------------------------------------------------------------------

(defrule config-sync-confval-with-parser
" The parser is used while handling incoming confval updates, hence keep it
  up-to-date
"
  (confval (path ?p) (is-list FALSE) (value ?v) (type ?type))
  =>
  (switch ?type
    (case UINT then (config-update-uint ?p ?v))
    (case FLOAT then (config-update-float ?p ?v))
    (case INT then (config-update-int ?p ?v))
    (case BOOL then (config-update-bool ?p ?v))
    (case STRING then (config-update-string ?p ?v))
    (default (printout error "Unknown confval type " ?type crlf))
  )
  (bind ?mps-prefix "/llsfrb/mps/stations/")
  (if (and (str-index ?mps-prefix ?p) (str-index "connection" ?p)) then
    (bind ?m-start (+ 1 (length$ ?mps-prefix)))
	(bind ?m-end (str-index "/" (sub-string ?m-start (length$ ?p) ?p)))
    (bind ?m-end (- (+ ?m-start ?m-end) 2))
    (bind ?machine-name (sym-cat (sub-string ?m-start ?m-end ?p)))
    (if (not (do-for-fact ((?m machine)) (and (eq ?m:name ?machine-name) (member$ ?m:state (create$ IDLE READY-AT-OUTPUT)))
      (reconfigure-machine ?machine-name)
      (mps-reset ?machine-name)
    )) then
      (printout error "Failed to re-initialize unknown machine " ?machine-name crlf)
    )
  )
)

(defrule config-sync-global-confval-with-global-var
" Override a global variable given from a confval.
  As confvals do not distinguish between STRING and SYMBOLS, preserve the
  type upon update.
  This is also why list-values are not supported as an empty list would void
  the type.
"
  ?cv <- (confval (path ?p&:(str-index "/llsfrb/globals/" ?p)) (is-list FALSE) (value ?v))
  =>
  (bind ?prefix-index (+ 1 (length$ "/llsfrb/globals/")))
  (bind ?var (upcase (sub-string ?prefix-index (length$ ?p) ?p)))
  (if (member$ (sym-cat ?var) (get-defglobal-list)) then
    (bind ?val ?v)
    (bind ?old-val (eval (str-cat "?*" ?var "*")))
    ; preserve type
    (if (eq (eval (str-cat "(type ?*"?var "*)")) STRING) then
      (bind ?val (str-cat "\"" ?v "\""))
    )
    ; override defglobal
    (bind ?str (str-cat "(defglobal ?*" ?var"* = " ?val ")"))
    (build ?str)
    (bind ?new-val (eval (str-cat "?*" ?var "*")))
    (if (neq (sym-cat ?old-val) (sym-cat ?new-val)) then
      (printout t "Changing " ?var " from " ?old-val " to " ?new-val crlf)
    )
  else
    (printout warn "confval" ?p " has no associated defglobal" crlf)
  )
)

(defrule config-sync-global-confval-with-global-var-warning
" Warn in case a confval corresponding to a multifield global is unknown.
"
  ?cv <- (confval (path ?p&:(str-index "/llsfrb/globals/" ?p)) (is-list TRUE) (list-value $?v))
  =>
  (bind ?prefix-index (+ 1 (length$ "/llsfrb/globals/")))
  (bind ?var (upcase (sub-string ?prefix-index (length$ ?p) ?p)))
  (if (not (member$ (sym-cat ?var) (get-defglobal-list))) then
    (printout warn "confval" ?p " has no associated defglobal" crlf)
  )
)


; Detrermine which config values should be sent via protobuf
(defrule config-add-pb-conf-field-width
  (confval (path ?p&"/llsfrb/game/field/width") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_width") (value ?v))
  )
)

(defrule config-add-pb-conf-field-height
  (confval (path ?p&"/llsfrb/game/field/height") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_height") (value ?v))
  )
)

(defrule config-add-pb-conf-field-mirrored
  (confval (path ?p&"/llsfrb/game/field/mirrored") (type ?t) (value ?v))
  (not (public-pb-conf (path ?p) (value ?v)))
  =>
  (do-for-all-facts ((?old-pb-conf public-pb-conf)) (eq ?old-pb-conf:path ?p)
    (retract ?old-pb-conf)
  )
  (bind ?type (confval-to-pb-type ?t))
  (assert (public-pb-conf (path ?p) (type ?type)
    (mapped-to "field_mirrored") (value ?v))
  )
)

(defrule config-sync-pb-conf
" Make sure the values sent via protobuf stay in sync with the confval values."
  (confval (path ?path) (type ?t) (value ?v))
  ?pb-conf <- (public-pb-conf (path ?path) (value ?old-v&:(neq ?v ?old-v)))
  =>
  (bind ?type (confval-to-pb-type ?t))
  (modify ?pb-conf (type ?type) (value ?v))
)
;(defrule print-confval
;  (confval (path ?p) (type ?t) (value ?v) (is-list ?is-list) (list-value $?lv))
;  =>
;  (if (debug 2) then
;    (printout t "confval path: " ?p "  type: " ?t
;	      "  value: " (if (eq ?is-list TRUE) then ?lv else ?v) crlf))
;)
