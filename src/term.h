/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Local terminal registration shim that includes upstream gnuplot terminals and the qad terminal.
*/

/* start from upstream generated/content term.h */
#include "../gnuplot-main/src/term.h"
/* then add your terminal */
#include "qad.trm"
