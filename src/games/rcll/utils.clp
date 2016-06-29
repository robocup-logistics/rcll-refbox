
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

(deffunction mirror-name (?zn)
  (bind ?team (sub-string 1 1 ?zn))
  (bind ?machine (sub-string 3 99 ?zn))
  (if (eq ?team "M") then
    (return (sym-cat "C-" ?machine))
  else
    (return (sym-cat "M-" ?machine))
  )
)

(deffunction mirror-zone (?zn)
  (bind ?team (sub-string 1 1 ?zn))
  (bind ?zone (sub-string 3 99 ?zn))
  (if (eq ?team "M") then
    (return (sym-cat "C_" ?zone))
  else
    (return (sym-cat "M_" ?zone))
  )
)

(deffunction want-mirrored-rotation (?mtype ?zone)
"According to the RCLL2017 rulebook, this is when a machine is mirrored"
  (bind ?zn (str-cat ?zone)) 
  (bind ?x (eval (sub-string 4 4 ?zn)))
  (bind ?y (eval (sub-string 5 5 ?zn)))

  (return (or (member$ ?mtype (create$ BS DS SS))
              (not (or (eq ?x 7) ; left or right
                       (eq ?y 8) ; top wall
                       (eq ?y 1) ; bottom wall
                       (and (member$ ?x (create$ 5 6 7)); insertion
                            (eq ?y 2)
                       )
                   )
              )
  ))
)

(deffunction mirror-orientation (?mtype ?zone ?ori)
  (bind ?zn (str-cat ?zone))
  (bind ?t (sub-string 1 1 ?zn))
  (if (want-mirrored-rotation ?mtype ?zone)
   then
    (if (eq ?t "C")
     then
      (do-for-fact ((?mo mirror-orientation)) (eq ?mo:cyan ?ori)
        (bind ?m-ori ?mo:magenta)
      )   
     else
      (do-for-fact ((?mo mirror-orientation)) (eq ?mo:magenta ?ori)
        (bind ?m-ori ?mo:cyan)
      )   
    )   
    (return ?m-ori)
   else
    (bind ?x (eval (sub-string 4 4 ?zn)))
    (bind ?y (eval (sub-string 5 5 ?zn)))

    (if (eq ?y 8) then
      (return 180)
    )   
    (if (or (eq ?y 1) (eq ?y 2)) then
      (return 0)
    )
    (if (and (eq ?x 7) (eq ?t "M")) then  ; this is the other way around, because I compare with the team color of the originalting machine
      (return 90)
    )
    (if (and (eq ?x 7) (eq ?t "C")) then
      (return 270)
    )
    (printout error "error in rotation of machines, checked all possible cases, but nothing cateched" crlf)
    (return ?ori)
  )
)

(deffunction pick-random$ (?list)
  (return (nth$ (random 1 (length$ ?list)) ?list)) 
)

(deffunction remove$ (?list ?item)
	(bind ?idx (member$ ?item ?list))
	(if ?idx
			then	(return (delete$ ?list ?idx ?idx))
			else  (return ?list))
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
