
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

(load* (resolve-file utils.clp))
(load* (resolve-file time.clp))
(load* (resolve-file config.clp))
(load* (resolve-file protobuf.clp))

(load* (resolve-file globals.clp))
(load* (resolve-file priorities.clp))
(load* (resolve-file facts.clp))

(defrule load-config
  (init)
  =>
  (foreach ?p ?*CONFIG_PREFIXES*
    (load-config ?p)
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

(defrule enable-debug
  (init)
  (confval (path "/llsfrb/clips/debug") (type BOOL) (value ?v))
  =>
  (if (eq ?v true) then
    (printout t "CLIPS debugging enabled, watching facts and rules" crlf)
    (watch facts)
    (watch rules)
    ;(dribble-on "trace.txt")
  )
)

(defrule add-fake-pucks
  (init)
  (confval (path "/llsfrb/sps/enable") (type BOOL) (value false))
  =>
  (loop-for-count (?i 22) do (assert (puck (index ?i) (id ?i) (team CYAN))))
  (loop-for-count (?i 23 44) do (assert (puck (index ?i) (id ?i) (team MAGENTA))))
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
