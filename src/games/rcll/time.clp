
;---------------------------------------------------------------------------
;  time.clp - time utils
;
;  Created: Sat Jun 16 15:45:57 2012 (Mexico City)
;  Copyright  2011-2012  Tim Niemueller [www.niemueller.de]
;             2011       SRI International
;             2011       RWTH Aachen University (KBSG)
;             2011       Carnegie Mellon University
;  Licensed under GPLv2+ license, cf. LICENSE file of cedar
;---------------------------------------------------------------------------

;(defmodule TIME-UTILS)

(defglobal
  ?*PRIORITY_TIME_RETRACT*    = -10000
)

; This assumes Fawkes-style time, i.e. sec and usec

(deffunction time-diff (?t1 ?t2)
  (bind ?sec  (- (nth$ 1 ?t1) (nth$ 1 ?t2)))
  (bind ?usec (- (nth$ 2 ?t1) (nth$ 2 ?t2)))
  (if (< ?usec 0)
      then (bind ?sec (- ?sec 1)) (bind ?usec (+ 1000000 ?usec)))
  ;(printout t "time-diff called: " ?t1 " - " ?t2 " = " (create$ ?sec ?usec) crlf)
  (return (create$ ?sec ?usec))
)

(deffunction time-diff-sec (?t1 ?t2)
  (bind ?td (time-diff ?t1 ?t2))
  (return (+ (float (nth$ 1 ?td)) (/ (float (nth$ 2 ?td)) 1000000.)))
)

(deffunction timeout (?now ?time ?timeout)
  (return (> (time-diff-sec ?now ?time) ?timeout))
)

(deffunction timeout-sec (?now ?time ?timeout)
  (return (> (- ?now ?time) ?timeout))
)

(deffunction time-from-sec (?t)
  (return (create$ (integer ?t) (integer (* (- ?t (integer ?t)) 1000000.))))
)

(deffunction time-to-sec (?t)
  (return (+ (nth$ 1 ?t) (/ (nth$ 2 ?t) 1000000.)))
)

(deffunction time-sec-format (?time-sec)
  (bind ?hour (div ?time-sec 3600))
  (bind ?min  (div (- ?time-sec (* ?hour 3600)) 60))
  (bind ?sec  (- ?time-sec (* ?hour 3600) (* ?min 60)))
  (if (> ?hour 0)
   then
    (return (format nil "%02d:%02d:%02d" ?hour ?min ?sec))
   else
    (return (format nil "%02d:%02d" ?min ?sec))
  )
)

(deffunction time-in-range (?time $?range)
  (return (and (>= ?time (nth$ 1 ?range)) (<= ?time (nth$ 2 ?range))))
)

; Check if two time ranges overlap each other.
; The parameters are two time ranges start and end times respectively.
; The units do not matter as long as they are the same for all values.
(deffunction time-range-overlap (?r1-start ?r1-end ?r2-start ?r2-end)
  (return (or (and (>= ?r1-start ?r2-start) (<= ?r1-start ?r2-end))
	      (and (>= ?r1-end ?r2-start) (<= ?r1-end ?r2-end))))
)

(deffunction time-nonzero (?t)
  (return (or (<> (nth$ 1 ?t) 0) (<> (nth$ 2 ?t) 0)))
)

(deffunction time-range-overlap-length (?t1-start ?t1-end ?t2-start ?t2-end)
  (if (and (>= ?t1-start ?t2-start) (<= ?t1-start ?t2-end))
   then ; start time of t1 is within t2
    (if (<= ?t1-end ?t2-end)
     then ; even end time is, so length of t1 is overlap
      (return (- ?t1-end ?t1-start))
     else ; t1 end is beyond t2 end, use that as boundary
      (return (- ?t2-end ?t1-start))
    )
   else
    (if (and (>= ?t1-end ?t2-start) (<= ?t1-end ?t2-end))
     then ; t1 end is within t2, t1 start cannot, otherwise
          ;case above would have been used
      (return (- ?t1-end ?t2-start))
     else
      (if (and (<= ?t1-start ?t2-start) (>= ?t1-end ?t2-end))
       then ; t1 includes t2
         (return (- ?t2-end ?t2-start))
       else
        (if (and (<= ?t2-start ?t1-start) (>= ?t2-end ?t1-end))
         then ; t2 includes t1
          (return (- ?t1-end ?t1-start))
        )
      )
    )
  )
  (return 0.0)
)

(deffunction time-range-overlap-ratio (?t1-start ?t1-end ?t2-start ?t2-end)
  (return (/ (time-range-overlap-length ?t1-start ?t1-end ?t2-start ?t2-end)
	     (- ?t1-end ?t1-start)))
)

(deffunction time-add (?t1 ?t2)
  (bind ?sec (+ (nth$ 1 ?t1) (nth$ 1 ?t2)))
  (bind ?usec (+ (nth$ 2 ?t1) (nth$ 2 ?t2)))
  (if (> ?usec 1000000)
    then
    (bind ?usec (- ?usec 1000000))
    (bind ?sec (+ ?sec 1))
  )
  (return (create$ ?sec ?usec))
)

(deffunction time-max (?t1 ?t2)
  (if (> (nth$ 1 ?t1) (nth$ 1 ?t2))
    then
    (return ?t1)
    else
    (if (< (nth$ 1 ?t1) (nth$ 1 ?t2))
      then
      (return ?t2)
      else
      (if (> (nth$ 2 ?t1) (nth$ 2 ?t2))
	then
	(return ?t1)
	else
	(return ?t2)
      )
    )
  )
)

; --- RULES - general housekeeping
(defrule retract-time
  (declare (salience ?*PRIORITY_TIME_RETRACT*))
  ?f <- (time $?)
  =>
  (retract ?f)
)
