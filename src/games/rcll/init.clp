
;---------------------------------------------------------------------------
;  init.clp - LLSF RefBox CLIPS  initialization file
;
;  Created: Thu Feb 07 19:13:49 2013
;  Copyright  2013  Tim Niemueller [www.niemueller.de]
;  Licensed under BSD license, cf. LICENSE file
;---------------------------------------------------------------------------

(defglobal
  ?*CLIPS_DIRS* = (get-clips-dirs)
  ?*DEBUG* = 2  ;debug levels: 0 ~ none, 1 ~ minimal, 2 ~ more, 3 ~ maximum
  ?*CONFIG_PREFIXES* = (create$ "/llsfrb")
  ?*START-TIME* = (now)
)

(deffunction resolve-file (?file)
  (foreach ?d ?*CLIPS_DIRS*
	   (bind ?fn (str-cat ?d ?file))
	   (if (open ?fn file-clips-tmp)
	    then
	     (close file-clips-tmp)
	     (return ?fn)
	   )
  )
  (return FALSE)
)
(load* (resolve-file globals.clp))

(load* (resolve-file facts.clp))
(load* (resolve-file utils.clp))
(load* (resolve-file time.clp))
(load* (resolve-file config.clp))
(load* (resolve-file protobuf.clp))


(load* (resolve-file priorities.clp))

(defrule load-websocket
  (init)
  (have-feature websocket)
  =>
  (load* (resolve-file websocket.clp))
)

(defrule load-config
  (init)
  =>
  (foreach ?p ?*CONFIG_PREFIXES*
    (load-config ?p)
  )
  (foreach ?global (get-defglobal-list)
    (bind ?global-val (eval (str-cat "?*" ?global "*")))
	(bind ?type (type ?global-val))
	(bind ?type-val ?global-val)
	(bind ?is-list FALSE)
	(bind ?val-slot-name "value")
	(if (eq ?type MULTIFIELD) then
		(bind ?type-val (nth$ 1 ?global-val))
		(bind ?is-list TRUE)
		(bind ?val-slot-name "list-value")
		(bind ?global-val (implode$ ?global-val))
	)
	(bind ?type (type ?type-val))
	(if (eq ?type INTEGER) then (bind ?type UINT))
	(if (and (eq ?type SYMBOL) (member$ ?global-val (create$ TRUE FALSE))) then (bind ?type BOOL))
	(if (eq ?type SYMBOL) then
		(bind ?type STRING)
		(bind ?global-val (str-cat "\"" ?global-val "\""))
	)
	(printout t ?global " has val " ?global-val " and type " ?type crlf)
	

    (str-assert (str-cat "(confval (path \"/llsfrb/globals/" (snake-case ?global) "\")"
	" (type " ?type ") (is-list " ?is-list ") (" ?val-slot-name " " ?global-val"))"))
  )
  (assert (config-loaded))
)

(defrule load-refbox
  (init)
  (confval (path "/llsfrb/clips/main") (type STRING) (value ?v))
  =>
  ;(printout t "Loading refbox main file '" ?v "'" crlf)
  (batch* (resolve-file (str-cat ?v ".clp")))
)

(defrule debug-level
  (init)
  (confval (path "/llsfrb/clips/debug-level") (type UINT) (value ?v))
  =>
  (printout t "Setting debug level to " ?v " (was " ?*DEBUG* ")" crlf)
  (bind ?*DEBUG* ?v)
)

(defrule announce-loading-done
  (declare (salience ?*PRIORITY_LAST*))
  (init)
  =>
  (printout t "RefBox loaded and ready to run" crlf)
)

(reset)
(seed (integer (time)))
