






                        CHAPTER  11


              The Joseph Lister Trace Package




     The Joseph Lister[] Trace package is an important  tool
for the interactive debugging of a Lisp program.  It  allows
you  to  examine selected  calls to a function or functions,
and optionally to stop execution  of  the  Lisp  program  to
examine the values of variables.

     The trace package is a set of Lisp programs located  in
the    Lisp   program   library   (usually   in   the   file
/usr/lib/lisp/trace.l).  There are two user  callable  func-
tions   in  the trace package: _t_r_a_c_e and _u_n_t_r_a_c_e.  The trace
package will be loaded automatically when you first use  the
_t_r_a_c_e  function.  Both  _t_r_a_c_eand _u_n_t_r_a_c_e are nlambdas (their
arguments are not evaluated).  The form of a call  to  _t_r_a_c_e
is
                   (trace _a_r_g_1 _a_r_g_2 _._._.)
where the _a_r_g_i have one of the following forms:


  foo - when foo is entered and exited, the  trace  informa-
       tion will be printed.


  (foo break) - when foo is entered  and  exited  the  trace
       information  will  be  printed.  Also, just after the
       trace information for foo is printed upon entry,  you
       will  be put in  a special break loop.  The prompt is
       `T>' and you may type any Lisp  expression,  and  see
       its  value printed.  The _ith argument to the function
       just called can be accessed as (arg _i).  To leave the
       trace  loop, just type ^D or (tracereturn) and execu-
       tion will continue.  Note that ^D will work  only  on
       UNIX systems.


  (foo if expression) - when foo is entered and the  expres-
       sion evaluates to non-nil, then the trace information
       will be printed for both exit and entry.  If  expres-
       sion evaluates to nil, then no trace information will
       be printed.

____________________
9   []_L_i_s_t_e_r, _J_o_s_e_p_h     1st  Baron  Lister  of  Lyme  Regis,
1827-1912; English surgeon: introduced antiseptic surgery.



9The Joseph Lister Trace Package                         11-1







The Joseph Lister Trace Package                         11-2


  (foo ifnot expression) -  when  foo  is  entered  and  the
       expression  evaluates to nil, then the trace informa-
       tion will be printed for both  entry  and  exit.   If
       both  if and _i_f_n_o_t are specified, then the _i_f expres-
       sion must evaluate to non nil AND the  _i_f_n_o_t  expres-
       sion  must  evaluate to nil for the trace information
       to be printed out.


  (foo evalin expression) - when foo is  entered  and  after
       the  entry  trace  information is printed, expression
       will be evaluated. Exit  trace  information  will  be
       printed when foo exits.


  (foo evalout expression) -  when  foo  is  entered,  entry
       trace  information  will be printed.  When foo exits,
       and before the exit  trace  information  is  printed,
       expression will be evaluated.


  (foo evalinout expression) - this has the same  effect  as
       (trace (foo evalin expression evalout expression)).


  (foo lprint) - this tells _t_r_a_c_e to use the  level  printer
       when  printing  the arguments to and the result of  a
       call to foo.  The level printer prints only  the  top
       levels  of  list structure. Any structure below three
       levels is printed as a &.  This allows you  to  trace
       functions with massive arguments or results.



     The following trace options permit one to have  greater
control  over  each action which takes place when a function
is traced.  These options are only meant to be used by  peo-
ple  who  need  special  hooks into the trace package.  Most
people should skip reading this section.


  (foo traceenter tefunc) - this tells _t_r_a_c_e that the  func-
       tion  to  be  called  when  foo is entered is tefunc.
       tefunc should be a lambda of two arguments, the first
       argument  will  be  bound to the name of the function
       being traced, foo in this case.  The second  argument
       will  be  bound to the list of arguments to which foo
       should be applied.  The function tefunc should  print
       some  sort  of "entering foo" message.  It should not
       apply foo to the arguments,  however.  That  is  done
       later on.

9

9                                   Printed: October 21, 1980







The Joseph Lister Trace Package                         11-3


  (foo traceexit txfunc) - this tells _t_r_a_c_e that  the  func-
       tion  to  be  called  when  foo  is exited is txfunc.
       txfunc should be a lambda of two arguments, the first
       argument  will  be  bound to the name of the function
       being traced, foo in this case.  The second  argument
       will  be bound to the result of the call to foo.  The
       function txfunc should print some  sort  of  "exiting
       foo" message.


  (foo evfcn evfunc) - this tells _t_r_a_c_e that the form evfunc
       should  be  evaluated to get the value of foo applied
       to its arguments. This option is a bit different from
       the  other  special options since evfunc will usually
       be an expression, not just the name  of  a  function,
       and  that  expression will be specific to the evalua-
       tion of  function  foo.   The  argument  list  to  be
       applied will be available as T-arglist.


  (foo printargs prfunc) - this tells _t_r_a_c_e to  used  prfunc
       to print the arguments  to be applied to the function
       foo.  prfunc should be a lambda of one argument.  You
       might  want  to use this option if you wanted a print
       function which could  handle  circular  lists.   This
       option  will work only if you do not specify your own
       _t_r_a_c_e_e_n_t_e_r function.  Specifying the option _l_p_r_i_n_t is
       just  a simple way of changing the printargs function
       to the level printer.


  (foo printres prfunc) - this tells _t_r_a_c_e to use prfunc  to
       print the result of evaluating foo.  prfunc should be
       a lambda of one argument.  This option will work only
       if  you  do  not specify your own _t_r_a_c_e_e_x_i_t function.
       Specifying the option _l_p_r_i_n_t changes printres to  the
       level printer.



     You may specify more than one option for each  function
traced. For example:

     (_t_r_a_c_e (_f_o_o _i_f (_e_q _3 (_a_r_g _1)) _b_r_e_a_k _l_p_r_i_n_t) (_b_a_r _e_v_a_l_i_n
(_p_r_i_n_t _x_y_z_z_y)))

This tells _t_r_a_c_e to trace two more functions, foo  and  bar.
Should  foo  be called with the first argument _e_q to 3, then
the entering foo message will  be  printed  with  the  level
printer.   Next  it  will enter a trace break loop, allowing
you to evaluate any lisp expressions.   When  you  exit  the
trace  break  loop, foo will be applied to its arguments and
the resulting value will be printed, again using  the  level


                                   Printed: October 21, 1980







The Joseph Lister Trace Package                         11-4


printer.   Bar is also traced, and each time bar is entered,
an entering bar message will be printed and then  the  value
of  xyzzy  will be printed.  Next bar will be applied to its
arguments and the result will be printed.  If you tell _t_r_a_c_e
to  trace  a function which is already traced, it will first
_u_n_t_r_a_c_e it.  Thus if you want to specify more than one trace
option for a function, you must do it all at once.  The fol-
lowing is _n_o_t equivalent to the preceding call to _t_r_a_c_e  for
foo:

(_t_r_a_c_e (_f_o_o _i_f (_e_q _3 (_a_r_g _1))) (_f_o_o _b_r_e_a_k) (_f_o_o _l_p_r_i_n_t))

In this example, only the last option, lprint,  will  be  in
effect.

     The function _t_r_a_c_e returns a list of functions  is  was
able  to  trace.   The function _u_n_t_r_a_c_e untraces those func-
tions its is argument list.  If the argument list  is  empty
then  all  functions  being  traced  are  untraced.  _U_n_t_r_a_c_e
returns a list of functions untraced.

     Generally the trace package has its own internal  names
for  the  the  lisp  functions it uses, so that you can feel
free to trace system functions like _c_o_n_d and not worry about
adverse  interaction  with the actions of the trace package.
You can trace any type of function: lambda,  nlambda,  lexpr
or  macro  whether  compiled or interpreted and you can even
trace array references (however you should  not  attempt  to
store in an array which has been traced).

     When tracing compiled code keep in mind that many func-
tion  calls  are translated directly to machine language  or
other equivalent  function calls.  A full list of open coded
functions  is  listed at the beginning of the liszt compiler
source.  _T_r_a_c_e will do a (_s_s_t_a_t_u_s _t_r_a_n_s_l_i_n_k _n_i_l)  to  insure
that  the  new  traced  definitions  it  defines  are called
instead of the old untraced ones.  You may notice that  com-
piled code will run slower after this is done.














9

9                                   Printed: October 21, 1980



