;; Copyright (c) 2020 by David Wilson, All Rights Reserved.
;; Substratic Engine - https://github.com/substratic/engine
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at https://mozilla.org/MPL/2.0/.

(define-library (substratic engine components position)
  (import (gambit)
          (substratic engine alist)
          (substratic engine state)
          (substratic engine components component))
  (export position-component)

  (begin

    (define (position-component #!key
                                (pos-x 0)
                                (pos-y 0)
                                (scale 1.0))
      (make-component position
        (pos-x pos-x)
        (pos-y pos-y)
        (scale scale)))))
