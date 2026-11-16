;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

(
    ;; For all modes
    (nil
        (indent-tabs-mode)
        (fill-column . 100)
        (whitespace-line-column . 100)
        )
    ;; For c++-mode
    (c++-mode
        (c-file-style . "bsd")
        (eval . (whitespace-mode 0))
        (whitespace-style . '(face empty tabs lines-tail trailing))
        (eval . (whitespace-mode 1))
        )
    )
