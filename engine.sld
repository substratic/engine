;; Copyright (c) 2020 by David Wilson, All Rights Reserved.
;; Substratic Engine - https://github.com/substratic/engine
;;
;; This Source Code Form is subject to the terms of the Mozilla Public
;; License, v. 2.0. If a copy of the MPL was not distributed with this
;; file, You can obtain one at https://mozilla.org/MPL/2.0/.

(define-library (substratic engine)
  (import (gambit)
          (substratic engine loop)
          (substratic engine node)
          (substratic engine events)
          (substratic engine components component))
  (export game-loop
          make-node
          make-component
          make-event))
