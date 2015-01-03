((c++-mode
  (c-file-style . "Linux")
  (flycheck-c/c++-gcc-executable . "/s/gcc-4.9.0/bin/gcc")
  (flycheck-disabled-checkers c/c++-clang)
  (flycheck-gcc-definitions
   "__STDC_CONSTANT_MACROS"
   "__STDC_FORMAT_MACROS"
   "__STDC_LIMIT_MACROS")
  (flycheck-gcc-include-path
   "/p/polyglot/public/tools/llvm/install/include"
   "/unsup/boost-1.55.0/include")
  (flycheck-gcc-language-standard . "c++11")))

;; Local variables:
;; flycheck-disabled-checkers: (emacs-lisp)
;; End:
