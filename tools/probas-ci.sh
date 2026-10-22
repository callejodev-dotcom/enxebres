#!/bin/bash
# As probas de Barullo Enxebres na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows, linux ou sanitizadores, coa compilación en build/
set -eo pipefail
SO="$1"
EXE=""; [ "$SO" = windows ] && EXE=".exe"
CONF=Release; [ "$SO" = sanitizadores ] && CONF=RelWithDebInfo
RT="build/BERenderTest_artefacts/$CONF/BERenderTest$EXE"

echo "::group::Son (as nove ferramentas: o que fai cada mando, o teito, os bloques, o bypass e o estado)"
"$RT" proba.wav
echo "::endgroup::"
