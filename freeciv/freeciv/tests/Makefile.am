## Process this file with automake to produce Makefile.in

# Currently the "src-check" directive creates a check-output file containing
# the results of the checks.  It might be better to actually fail the make run
# if the check fails.
src-check:
	rm -f check-output check-output_
	$(srcdir)/check_macros.sh $(top_srcdir) >> check-output_
	$(srcdir)/copyright.sh $(top_srcdir) >> check-output_
	$(srcdir)/fcintl.sh $(top_srcdir) >> check-output_
	$(srcdir)/header_guard.sh $(top_srcdir) >> check-output_
	$(srcdir)/va_list.sh $(top_srcdir) >> check-output_
	cat check-output_ | sed "s,$(top_srcdir)/,," > check-output
	rm -f check-output_

.PHONY: src-check

CLEANFILES = check-output

EXTRA_DIST =	check_macros.sh			\
		copyright.sh			\
		fcintl.sh			\
		header_guard.sh			\
		rulesets_not_broken.sh.in	\
		rs_test_res/ruleset_is.lua	\
		rs_test_res/ruleset_list.txt	\
		rs_test_res/ruleset_loads.sh.in	\
		va_list.sh
