#
# ForUtil/man/Makefile
#
# Makefile for building each of the manual page directories from the
# groff source in man/man
#
# Needed to build these things are:
# man2html
# groff
# add_urls.sh
#

#
# Commands used in this Makefile
GROFF=groff -Tascii -man
SED=sed
RM=rm -f
MAKE=make
ADDURL=./add_urls.sh
MAN2HTML= man2html -title

#
# groff source files for the ForUtil manual pages
SRCS= man/fflow.1 \
	man/ffscan.1 \
	man/ftags.1 \
	man/get_common.1 \
	man/scan_commons.1 \
	man/list_commons.1 \
	man/scan_commons.1

#
# Compilation actions
all: rest-in-groff text html
.PHONY: all

#
# This is to prevent unneccesary creation of the preformat/* manual pages
# if nothing in man/* has been touched.
# (See the GNU make info on Empty Targets for more information)
.PHONY: rest-in-groff
rest-in-groff: stmp-preformat

stmp-preformat: $(SRCS)
	$(MAKE) format
	touch $@

#
# Rule to create formatted manual pages from groff source
# This rule is never instanciated directly.
format::
	@(set -x; for file in man/*.1 ; do \
		$(GROFF) $$file > preformat/`basename $$file .1`.1 ; \
	done )

#
# Rule to create plain text version of formatted manual pages in preformat
text:: rest-in-groff
	@(set -x; for file in preformat/*.1 ; do \
		$(SED) -e 's/.//g' $$file > doc/`basename $$file .1`.txt ; \
	done )

#
# Rule to create html versions of formatted manual pages in preformat
html:: rest-in-groff
	@(set -x; for file in preformat/*.1 ; do \
		base=`basename $$file .1` ; \
		$(MAN2HTML) "$$base Manual Page" < $$file > html/$$base.html ; \
	done ; \
	$(ADDURL) )

clean::
	$(RM) preformat/*.1
	$(RM) doc/*.txt
	$(RM) html/*.html
	$(RM) stmp-preformat

#
# No dependencies
