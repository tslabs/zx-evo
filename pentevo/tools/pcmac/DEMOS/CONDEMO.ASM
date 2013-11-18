;CONDEMO.ASM
;Program to demonstrate
;the directives
;#if
;#ifdef
;#ifndef
;#else
;#elsif
;#endif

;Some simple exapmles first:
#if 1
#message "The expression 1 is true"
#endif

#if 0
#message "No matter what this string is,"
#message "because it won't appear on the screen."
#endif

;Some more complex
var variable
variable := 1

#ifdef variable
#message "The identifier variable is defined."

#if variable
#message "The value of variable is true."
#else
#message "The value of variable is false."
#endif

#else
#message "The identifier variable is not defined."
#endif

;Three different cases to separate
variable := 3
#if variable = 1
#message "Variable = one."
#elsif variable =2
#message "Variable = two."
#else
#message "Variable greather than two."
#endif
;End of file CONDEMO.ASM
