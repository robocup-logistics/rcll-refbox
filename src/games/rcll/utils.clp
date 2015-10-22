
;---------------------------------------------------------------------------
;  utils.clp - CLIPS agent utility functions
;
;  Created: Sun Jun 17 12:19:34 2012 (Mexico City)
;  Copyright  2012  Tim Niemueller [www.niemueller.de]
;  Licensed under GPLv2+ license, cf. LICENSE file
;---------------------------------------------------------------------------

(deffunction debug (?level)
  (return (<= ?level ?*DEBUG*))
)

(deffunction append$ (?list $?items)
  (insert$ ?list (+ (length$ ?list) 1) ?items)
)

(deffunction randomize$ (?list)
  (bind ?l (length$ ?list))
  (loop-for-count 200 do
    (bind ?a (random 1 ?l))
    (bind ?b (random 1 ?l))
    (bind ?tmp (nth$ ?a ?list))
    (bind ?list (replace$ ?list ?a ?a (nth$ ?b ?list)))
    (bind ?list (replace$ ?list ?b ?b ?tmp))
  )
  (return ?list)
)

(deffunction pick-random$ (?list)
  (return (nth$ (random 1 (length$ ?list)) ?list)) 
)

(deffunction is-even-int (?num)
  (return (eq (mod ?num 2) 0))
)

(deffunction is-odd-int (?num)
  (return (eq (mod ?num 2) 1))
)

(deffunction non-zero-pose ($?pose)
  (foreach ?pf ?pose (if (<> ?pf 0.0) then (return TRUE)))
  (return FALSE)
)


(deffunction in-box (?pose ?box-center ?box-extent)
  (bind ?box-half-x (/ (nth$ 1 ?box-extent) 2))
  (bind ?box-half-y (/ (nth$ 2 ?box-extent) 2))

  (return (and (<= (abs (- (nth$ 1 ?pose) (nth$ 1 ?box-center))) ?box-half-x)
	       (<= (abs (- (nth$ 2 ?pose) (nth$ 2 ?box-center))) ?box-half-y)))
)

(deffunction string-gt (?s1 ?s2)
  (return (> (str-compare ?s1 ?s2) 0))
)


(deffunction gen-random-string (?l)
  (bind ?rv "")
  (bind ?random-set (create$ 33 35 36 45 46 58 61 64 95))
  (loop-for-count (?c 48  57) (bind ?random-set (append$ ?random-set ?c)))
  (loop-for-count (?c 65  90) (bind ?random-set (append$ ?random-set ?c)))
  (loop-for-count (?c 97 122) (bind ?random-set (append$ ?random-set ?c)))

  (loop-for-count (?c 1 ?l)
    (bind ?rv (str-cat ?rv (format nil "%c" (pick-random$ ?random-set))))
  )
  (return ?rv)
)


(deffunction ceil (?f)
  (bind ?rf (round ?f))
  (return (if (>= (- ?rf ?f) 0) then ?rf else (+ ?rf 1)))
)

(deffunction floor (?f)
  (bind ?rf (round ?f))
  (return (if (> (- ?rf ?f) 0) then (- ?rf 1) else ?rf))
)
