# Generated automatically from Makefile.in by configure.
#
# $Header: /usr/local/rcs/ForUtil/Commons/RCS/Makefile.in,v 1.1 1996/08/07 21:10:47 koen Exp koen $
#
# Makefile.in for the ForUtil common utilities
#
# (C)Copyright 1995-1996 Ripley Software Development
# All Rights Reserved
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 

CC = gcc
CFLAGS= -g
CPPFLAGS=
LDFLAGS=
EXE = .exe

LIBS= -L. -L../lib $(LOADLIBES) -lforUtil -lgdbm

DEFS = -DHAVE_CONFIG_H

ALL_INCLUDES= -I. -I../. -I../lib $(INCLUDES) 
ALL_CPPFLAGS= $(CPPFLAGS) $(ALL_INCLUDES) $(DEFS)

RM = rm -f
AR = ar crv
RANLIB = ar srv
DEPEND = gccmakedep

LEX = flex
LEXFLAGS = -i
LEXLIB= -lfl

# Installation program
install=.././install-sh -c

SRCS = scan_commons.c get_common.c list_commons.c 

OBJS = scan_commons.o get_common.o list_commons.o 

ALLSRC= $(SRCS) lib_commons.c

ALLOBJ= $(OBJS) lib_commons.o

TARGETS= get_common$(EXE) list_commons$(EXE) scan_commons$(EXE)

#### End of Configuration section

#### Start of Compilation rules 

#rule to create .o files from .c files 
.c.o:
	$(RM) $@
	$(CC) $(CFLAGS) $(ALL_CPPFLAGS) -c $< -o $@

#rule to create .c files from .l files
.l.c:
	$(RM) $@ 
	$(LEX) $(LEXFLAGS) $<
	mv lex.yy.c $@

#### End of Compilation Rules

#### Start of possible targets

all:: $(TARGETS) 

list_commons$(EXE):: lib_commons.o list_commons.o
	$(RM) $@ \ 
	$(CC) -o $@ $(LDFLAGS) list_commons.o lib_commons.o $(LIBS) 

get_common$(EXE):: lib_commons.o get_common.o
	$(RM) $@ \ 
	$(CC) -o $@ $(LDFLAGS) get_common.o lib_commons.o $(LIBS) 

scan_commons$(EXE):: lib_commons.o scan_commons.o 
	$(RM) $@ \ 
	$(CC) -o $@ $(LDFLAGS) scan_commons.o lib_commons.o $(LIBS) $(LEXLIB)

install:: $(TARGETS)
	$(install) $(bininst_flags) scan_commons $(bindir)
	$(install) $(bininst_flags) list_commons $(bindir)
	$(install) $(bininst_flags) get_common   $(bindir)

uninstall::
	$(RM) $(bindir)/get_common
	$(RM) $(bindir)/list_commons
	$(RM) $(bindir)/scan_commons

depend: $(ALLSRC) 
	$(DEPEND) $(ALL_CPPFLAGS) $(ALLSRC)

mostlyclean:
	$(RM) $(ALLOBJ)
	$(RM) *.bak *.out core a.out 

clean:: mostlyclean 
	$(RM) $(TARGETS) 

distclean: clean
	$(RM) Makefile 

#dependencies
scan_commons.c: scan_commons.l

