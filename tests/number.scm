(import (rnrs)
        (mosh test))

(test-false (= +nan.0 +nan.0))

;; R6RS doesn't say about follwing, but Mosh returns #t for-all.
(test-true (equal? +nan.0 +nan.0))
(test-true (equal? +nan.0 -nan.0))
(test-true (equal? -nan.0 +nan.0))
(test-true (equal? +nan.0 (string->number "+nan.0")))
(test-true (equal? +nan.0 (string->number "-nan.0")))
(test-true (equal? -nan.0 (string->number "+nan.0")))
(test-true (equal? -nan.0 (string->number "-nan.0")))

(test-error assertion-violation? (bitwise-bit-field #xFF 2 1))

;; Issue 105
;;  http://code.google.com/p/mosh-scheme/issues/detail?id=105
;;  inexact prefix
(test-equal "3.0"      (number->string #i3.0))
(test-equal "3.0+0.0i" (number->string #i3.0+0i))
(test-equal "3.0+0.0i" (number->string #i3+0i))
(test-equal "3.0+0.0i" (number->string #i3.0-0i))
(test-equal "3.0+0.0i" (number->string #i3-0i))
(test-equal "0.0+0.0i" (number->string #i+0i))
(test-equal "0.0+2.0i" (number->string #i+2i))
(test-equal "0.0+0.0i" (number->string #i-0i))
(test-equal "0.0-2.0i" (number->string #i-2i))
;;  exact prefix
(test-equal "3" (number->string #e3.0+0i))
(test-equal "3" (number->string #e3+0i))
(test-equal "3" (number->string #e3.0-0i))
(test-equal "3" (number->string #e3-0i))
(test-equal "0" (number->string #e+0i))
(test-equal "0+2i" (number->string #e+2i))
(test-equal "0" (number->string #e-0i))
(test-equal "0-2i" (number->string #e-2i))
;;  no prefix
(test-equal "3.0"      (number->string 3.0-0i))
(test-equal "3"        (number->string 3-0i))
(test-equal "3.0"      (number->string 3.0+0i))
(test-equal "3"        (number->string 3+0i))
(test-equal "0" (number->string +0i))
(test-equal "0+2i" (number->string +2i))
(test-equal "0" (number->string -0i))
(test-equal "0-2i" (number->string -2i))
;; integer?, rational?
(test-true (integer? 3+0i))
(test-false (integer?  #i3.0+0i))
(test-false (rational? #i3.0+0i))
(test-true (integer?  3.0+0i))
(test-true (rational? 3.0+0i))
(test-true (integer?  3.0))
(test-true (rational? 3.0))
(test-true (integer?  #i3.0))
(test-true (rational? #i3.0))
(test-true (integer-valued? 3.0+0.0i))
(test-true (integer-valued? #i3.0+0i))

;; expt
(test-equal +2i (expt 1+i 2))
(test-equal -9+46i (expt 3+2i 3))
(test-equal 1/5-2/5i (expt 1+2i -1))

(test-true (fixnum? (/ 4 2)))

(test-error assertion-violation? (exact-integer-sqrt 0.0))
(test-error assertion-violation? (-))
(test-error assertion-violation? (apply - '()))

;; http://www.exploringbinary.com/java-hangs-when-converting-2-2250738585072012e-308/
2.2250738585072012e-308

(test-results)
