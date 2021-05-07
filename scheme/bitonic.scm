(define vmax
  (lambda (a b)
    (map max a b)))

(define vmin
  (lambda (a b)
    (map min a b)))

(define vpmax
  (lambda (a b)
    (list (max (car a) (cadr a))
          (max (caddr a) (cadddr a))
          (max (car b) (cadr b))
          (max (caddr b) (cadddr b)))))

(define vpmin
  (lambda (a b)
    (list (min (car a) (cadr a))
          (min (caddr a) (cadddr a))
          (min (car b) (cadr b))
          (min (caddr b) (cadddr b)))))
