(File lxref.l)
(implode-fun lambda cons implode <& not cdr cxr list |1-| do)
(find-func lambda |1-| implode-fun |1+| eq cond if cdr memq <& not setq or cxr do)
(match lambda |1+| match car eq cxr cdr <& and null cond if)
(print-rec lambda print-rec patom not length tyo car null liszt-internal-do mapc eq =& setq terpr progn msg + cdr >& caar flatc let cond if)
(printrcd lambda print-rec quote sortcar let)
(anno-check lambda printrcd get find-func setq let quote match cond if)
(oversize-check lambda tyo and eq or tyi do cond if)
(flush-a-line lambda oversize-check)
(write-a-line lambda |1+| cdr tyo terpr oversize-check cond if <& not cxr do)
(read-a-line lambda return |1+| cdr rplacx >& cond if eq or tyi do setq)
(anno-it lambda anno-check write-a-line setq flush-a-line match cond if null read-a-line do)
(process-annotate-file lambda sys:link sys:unlink probef close anno-it outfile concat terpr patom progn msg infile setq errset null cond if let)
(generate-xref-file lambda cdar flatc + terprchk memq print not terpr progn msg nreverse or liszt-internal-do mapcar patom typeit ncons cxr getd cdr cadar caar concat cons sortcar length lessp cond If get car null do quote sort setq)
(typeit lambda car dtpr cxr getdisc bcdp cond)
(terprchk lambda patom terpr + setq cdr >& > cond)
(process-Doc lambda cadr cons setf push putprop get not cond If car setq close null read do)
(process-Chome lambda cdr cons setf push putprop get not cond If car setq close null read do)
(process-File lambda cdr cddr cadr cons setf push putprop get not cond If car setq close null read do let)
(process-xref-file lambda close illegal-file process-Doc process-Chome process-File quote dtpr read concat infile errset null cdr cons implode cadr car eq and If exploden nreverse setq let terpr patom progn msg cond if)
(illegal-file lambda terpr patom progn msg)
(lxref nlambda return generate-xref-file not process-xref-file process-annotate-file quote if null do explode cdr readlist fixp car getcharn eq and cond If gensym makereadtable setq prog)
(xrefinit lambda terpr patom exit Internal-bcdcall getdisc eq bcdp cxr getd symbolp and funcall errset car setq quote signal cond if command-line-args let)
