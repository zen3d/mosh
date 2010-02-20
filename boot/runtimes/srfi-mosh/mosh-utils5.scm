(define ERRPORT (current-error-port))
(define (PCK . obj)
  (if %verbose
    (begin 
      (display "-> " ERRPORT)
      (for-each (lambda (e)
		  (display e ERRPORT))
		obj)
      (newline ERRPORT))))

;;------------------------------------------------
;; definitions
;;------------------------------------------------

(define (run-win32-np?) (string=? "win32" (host-os)))
(define CHR-ENVPATHSEP (if (run-win32-np?) #\; #\:))

(define pathfilter 
  (if (run-win32-np?) 
    (lambda (str) 
      (and (string? str) 
	   (list->string (map (lambda (e) (if (char=? e #\\) #\/ e)) (string->list str)))))
    (lambda (str) str)))

(define pathfinish 
  (if (run-win32-np?)
    (lambda (str) (and (string? str) (list->string (cdr (string->list str)))))
    (lambda (str) str)))

(define (nmosh-cache-dir) 
  (if (run-win32-np?) 
    (string-append (pathfilter (mosh-executable-path)) "cache") 
    (string-append (get-environment-variable "HOME") "/.nmosh-cache")))

(define mosh-cache-dir nmosh-cache-dir)

(define (nmosh-cache-path) 
  (string-append (nmosh-cache-dir) "/"))

(define absolute-path? 
  (if (run-win32-np?) ;FIXME: support UNC pathes
    (lambda (pl)
      (let ((a (car pl)))
	(and ; is a drive letter?
	  (= (string-length a) 2)
	  (char=? (cadr (string->list a)) #\:))))
    (lambda (pl) (= 0 (string-length (car pl))) )))

;;------------------------------------------------
;; utils
;;------------------------------------------------
(define (strsep str chr)
  (define (gather l) ;
    (define (itr cur rest0 rest1)
      (cond
	((not (pair? rest1)) (reverse cur))
	(else
	  (itr (cons (substring str
		       (+ 1 (car rest0)) 
		       (car rest1)) cur) 
	       (cdr rest0) 
	       (cdr rest1)))))
    (itr '() l (cdr l)))
  (define (spl l s)
    (define (itr idx cur rest)
      (cond
	((not (pair? rest)) (reverse (cons idx cur)))
	((char=? s (car rest))
	 (itr (+ idx 1) (cons idx cur) (cdr rest)))
	(else
	  (itr (+ idx 1) cur (cdr rest)))))
    (itr 0 (list -1) l))
  (if (string? str)
    (let* ((l (string->list str))
	   (m (spl l chr))
	   (r (gather m)))
      r )
    '()
    ))
;;------------------------------------------------
;; path handling
;;------------------------------------------------
(define RUNPATH (pathfilter (current-directory)))

(define (compose-path l)
  (define (fold-dotdot l)
    (define (itr cur rest)
      (if (pair? rest)
	(let ((a (car rest)))
	  (if (string=? ".." a)
	    (itr (cdr cur) (cdr rest)) ; drop top
	    (itr (cons a cur) (cdr rest))))
	(reverse cur)))
    (itr '() l))
  (define (omit-dot l)
    (define (itr cur rest)
      (if (pair? rest)
	(let ((a (car rest)))
	  (if (string=? "." a)
	    (itr cur (cdr rest)) ; drop "."
	    (itr (cons a cur) (cdr rest))))
	(reverse cur)))
    (itr '() l))
  (define (omit-zerolen l)
    (define (itr cur rest)
      (if (pair? rest)
	(let ((a (car rest)))
	  (if (= 0 (string-length a))
	    (itr cur (cdr rest))
	    (itr (cons a cur) (cdr rest))))
	(reverse cur)))
    (itr '() l))
  (define (insert-slash l)
    (define (itr cur rest)
      (if (pair? rest)
	(itr (cons "/" (cons (car rest) cur)) (cdr rest))
	(reverse (cdr cur)))) ;drop last "/"
    ;(PCK 'COMPOSING: l)
    (itr (list "/") l))

  (apply string-append (insert-slash (fold-dotdot (omit-dot (omit-zerolen l))))))

(define (make-absolute-path pth)
  (let ((pl (strsep (pathfilter pth) #\/)))
    (if (pair? pl)
      (pathfinish
	(compose-path (if (absolute-path? pl)
			(if (string=? "win32" (host-os)) ;FIXME FIXME FIXME!
			  pl ; Include Drive letter
			  (cdr pl)) ; Dispose heading /
			(append (strsep RUNPATH #\/) pl))))
      "")))

(define (pathsep str)
  (strsep str CHR-ENVPATHSEP))

; FIXME: handle exceptions
(define (ca-writeobj fn obj)
  (call-with-port (open-file-output-port fn) (lambda (p) (fasl-write! obj p))))
(define (ca-readobj fn)
  (call-with-port (open-file-input-port fn) fasl-read))



; load cache or generate cache (and load them)
(define (ca-filename->cachename fn)
  (define (escape str)
    (list->string (map (lambda (e) (cond
				     ((char=? e #\:) #\~)
				     ((char=? e #\/) #\~)
				     (else e)))
		       (string->list str))))
  (string-append (nmosh-cache-path) (escape fn) ".nmosh-fasl"))

(define nm:parachute #f) 

(define dbg-files '())
(define (dbg-addfile fn cfn dfn)
  (set! dbg-files (cons (list fn dfn) dbg-files)))

(define (ca-load/cache fn recompile? name)
  (define (reload)
    (PCK 'CACHE: 'RECOMPILE!!!)
    (set! nm:parachute #f)
    (ca-load fn #t name))
  (let* ((cfn (ca-filename->cachename fn))
	 (dfn (string-append cfn ".nmosh-dbg")))
    (dbg-addfile fn cfn dfn)
    (cond
      ((file-exists? cfn)
       (cond ((and (not recompile?) (file-newer? cfn fn))
	      (PCK 'CACHE: 'loading.. cfn)
	      ; try loading
	      (if (eq? 'nm:failure
		       (call/cc (lambda (k) ; FIXME: i assume a call/cc is much faster than an I/O
				  (unless nm:parachute (set! nm:parachute k))
				  (eval-compiled! (ca-readobj cfn)))))
		(reload)
		(set! nm:parachute #f))
	      (PCK 'CACHE: 'done))
	     (else
	       (PCK 'CACHE: 're-cache..)
	       (delete-file cfn)
	       (ca-load fn #f name))))
      (else
	(PCK 'CACHE: fn '=> cfn)
	(ex:expand-file-to-cache fn dfn cfn name)
	(PCK 'LOADING..)
	(ca-load fn #f name)))))

(define (ca-load fn recompile? name)
  (cond
    (%disable-acc 
      (PCK 'loading fn "(ACC disabled)")
      (ex:load fn))
    (else (ca-load/cache fn recompile? name))))

(define (ca-prepare-cache-dir)
  (unless (file-exists? (nmosh-cache-dir))
    (create-directory (nmosh-cache-dir))))

(define (ca-serialize fn l)
  (ca-prepare-cache-dir)
  (ca-writeobj fn (compile-w/o-halt l)))

(define (ca-write-debug-file sourcefile fn src syms)
  (ca-prepare-cache-dir)
  (when (file-exists? fn)
    (delete-file fn))
  (ca-writeobj fn (list 
	       (cons 'DBG-FILENAME sourcefile)
	       (cons 'DBG-SOURCE src)
	       (cons 'DBG-SYMS syms))))

(define (make-prefix-list)
  (define (append-prefix-x l str)
    (cond
      ((and str (file-exists? str))
       (map (lambda (e) (string-append (pathfilter str) "/" e)) l))
      (else '())))
  (define (append-prefix-l l lstr)
    (define (itr cur rest)
      (cond
	((not (pair? rest)) cur)
	(else
	  (itr (append cur (append-prefix-x l (car rest))) (cdr rest)))))
    (itr '() lstr))

  (define (append-prefix-curpath l)
    (append-prefix-x l (current-directory)))
  (define (append-prefix-execpath l)
    (append-prefix-x l (mosh-executable-path)))
  (define (append-prefix-stdlibpath l)
    (append-prefix-x l (standard-library-path)))
  (define (append-prefix l)
    (append
      (append-prefix-execpath l)
      (if %loadpath (append-prefix-l l (pathsep %loadpath)) '())
      (if (get-environment-variable "MOSH_LOADPATH")
	(append-prefix-l l (get-environment-variable "MOSH_LOADPATH")) 
	'())
      (append-prefix-curpath l)
      (append-prefix-stdlibpath l)
      l ;fallback
      ))
  (append-prefix (list "" "lib/")))

(define prefix-list (make-prefix-list))

; TODO: support multiple libraries
(define (library-name->filename name) ; => PATH or #f
  (define (expand-prefix str l)
    (map (lambda (e) (string-append e str)) l))
  (define (expand-suffix str l)
    (map (lambda (e) (string-append str e)) l))
  (define (nibblechar n)
    (cond
      ((<= 0 n 9) (integer->char (+ n #x30)))
      (else (integer->char (+ (- 10 n) #x61)))))
  (define (between? x y z) ; Gauche does not support 3 args char<=?
    (and (char<=? x y)
	 (char<=? y z)))
  (define (namesymbol s)
    (define (convc c)
      (cond ;;from psyntax
	((or (between? #\a c #\z)
	     (between? #\A c #\Z)
	     (between? #\0 c #\9)
	     ;(append-prefix-curpath l)
	     (memv c '(#\- #\. #\_ #\~))) (list c))
	(else (let ((i (char->integer c)))
		(list
		  #\%
		  (nibblechar (quotient i 16))
		  (nibblechar (remainder i 16)))))))
    (list->string (apply append (map convc (string->list (symbol->string s))))))

  (define (check-files l)
    (if (null? l)
      #f
      (begin
	(if (file-exists? (car l))
	  (car l)
	  (check-files (cdr l))))))
  ; e.g. for (example test) :
  ; 1. build base (example test) => "example/test"
  (define (basename name)
    (define (itr cur l)
      (if (null? l)
	cur
	(itr (string-append cur "/" (namesymbol (car l))) (cdr l))))
    (itr (namesymbol (car name)) (cdr name)))
  ; 2. expand pre/sufx
  (define (expand-name name)
    (apply append
	   (map (lambda (s) (expand-suffix s 
					   '(".nmosh.sls" ".nmosh.ss" ".nmosh.scm"
					     ".mosh.sls" ".mosh.ss" ".mosh.scm"
					     ".sls" ".ss" ".scm")))
		(expand-prefix name prefix-list))))

  (let* ((fn (check-files (expand-name (basename name))))
	 (cfn (make-absolute-path fn)))
    (if fn
      (begin
	;(PCK 'PATH fn '=> cfn)
	cfn)
      #f))
  )

