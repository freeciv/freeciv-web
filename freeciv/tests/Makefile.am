## Process this file with automake to produce Makefile.in

# Currently the "src-check" directive creates a check-output file containing
# the results of the checks.  It might be better to actually fail the make run
# if the check fails.
src-check:
	rm -f check-output check-output_
	$(srcdir)/header_guard.sh $(top_srcdir) >> check-output_
	$(srcdir)/va_list.sh $(top_srcdir) >> check-output_
	$(srcdir)/copyright.sh $(top_srcdir) >> check-output_
	cat check-output_ | sed "s\\$(top_srcdir)/\\\\" > check-output
	rm -f check-output_

.PHONY: src-check

CLEANFILES = check-output

EXTRA_DIST =					\
		copyright.sh			\
		header_guard.sh			\
		va_list.sh
