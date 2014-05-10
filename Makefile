# Top level Makefile for tin
# - for configuration options read the doc/INSTALL file.
#
# Updated: 2007-09-24
#

PROJECT	= tin
LVER	= 1
PVER	= 9
SVER	= 3
VER	= $(LVER).$(PVER).$(SVER)
DVER	= 20080326
EXE	= tin

# directory structure
TOPDIR	= .
DOCDIR	= ./doc
INCDIR	= ./include
OBJDIR	= ./src
SRCDIR	= ./src
PCREDIR	= ./pcre
CANDIR	= ./libcanlock
TOLDIR	= ./tools
PODIR	= ./po
INTLDIR	= ./intl

HFILES	= \
	$(INCDIR)/bool.h \
	$(INCDIR)/bugrep.h \
	$(INCDIR)/debug.h \
	$(INCDIR)/extern.h \
	$(INCDIR)/keymap.h \
	$(INCDIR)/newsrc.h \
	$(INCDIR)/nntplib.h \
	$(INCDIR)/plp_snprintf.h \
	$(INCDIR)/policy.h \
	$(INCDIR)/proto.h \
	$(INCDIR)/rfc2046.h \
	$(INCDIR)/stpwatch.h \
	$(INCDIR)/tcurses.h \
	$(INCDIR)/tin.h \
	$(INCDIR)/tinrc.h \
	$(INCDIR)/tnntp.h \
	$(INCDIR)/trace.h \
	$(INCDIR)/version.h

CFILES	= \
	$(SRCDIR)/active.c \
	$(SRCDIR)/art.c \
	$(SRCDIR)/attrib.c \
	$(SRCDIR)/auth.c \
	$(SRCDIR)/charset.c \
	$(SRCDIR)/color.c \
	$(SRCDIR)/config.c \
	$(SRCDIR)/cook.c \
	$(SRCDIR)/curses.c \
	$(SRCDIR)/debug.c\
	$(SRCDIR)/envarg.c \
	$(SRCDIR)/feed.c \
	$(SRCDIR)/filter.c \
	$(SRCDIR)/getline.c \
	$(SRCDIR)/global.c \
	$(SRCDIR)/group.c \
	$(SRCDIR)/hashstr.c \
	$(SRCDIR)/header.c \
	$(SRCDIR)/help.c\
	$(SRCDIR)/inews.c \
	$(SRCDIR)/init.c \
	$(SRCDIR)/joinpath.c \
	$(SRCDIR)/keymap.c \
	$(SRCDIR)/lang.c \
	$(SRCDIR)/langinfo.c \
	$(SRCDIR)/list.c \
	$(SRCDIR)/lock.c \
	$(SRCDIR)/mail.c \
	$(SRCDIR)/main.c \
	$(SRCDIR)/makecfg.c \
	$(SRCDIR)/memory.c \
	$(SRCDIR)/mimetypes.c \
	$(SRCDIR)/misc.c \
	$(SRCDIR)/newsrc.c\
	$(SRCDIR)/nntplib.c \
	$(SRCDIR)/nrctbl.c \
	$(SRCDIR)/options_menu.c \
	$(SRCDIR)/page.c \
	$(SRCDIR)/parsdate.y \
	$(SRCDIR)/plp_snprintf.c \
	$(SRCDIR)/pgp.c \
	$(SRCDIR)/post.c \
	$(SRCDIR)/prompt.c \
	$(SRCDIR)/read.c \
	$(SRCDIR)/refs.c \
	$(SRCDIR)/regex.c \
	$(SRCDIR)/rfc1524.c \
	$(SRCDIR)/rfc2045.c \
	$(SRCDIR)/rfc2046.c \
	$(SRCDIR)/rfc2047.c \
	$(SRCDIR)/save.c \
	$(SRCDIR)/screen.c \
	$(SRCDIR)/search.c \
	$(SRCDIR)/select.c \
	$(SRCDIR)/sigfile.c \
	$(SRCDIR)/signal.c \
	$(SRCDIR)/strftime.c \
	$(SRCDIR)/string.c \
	$(SRCDIR)/tags.c \
	$(SRCDIR)/tcurses.c \
	$(SRCDIR)/tmpfile.c \
	$(SRCDIR)/my_tmpfile.c \
	$(SRCDIR)/thread.c \
	$(SRCDIR)/trace.c \
	$(SRCDIR)/version.c \
	$(SRCDIR)/wildmat.c \
	$(SRCDIR)/xface.c \
	$(SRCDIR)/xref.c

DOC	= \
	$(DOCDIR)/ABOUT-NLS \
	$(DOCDIR)/CHANGES \
	$(DOCDIR)/CHANGES.old \
	$(DOCDIR)/CREDITS \
	$(DOCDIR)/DEBUG_REFS \
	$(DOCDIR)/INSTALL \
	$(DOCDIR)/TODO \
	$(DOCDIR)/WHATSNEW \
	$(DOCDIR)/art_handling.txt \
	$(DOCDIR)/article.txt \
	$(DOCDIR)/auth.txt \
	$(DOCDIR)/config-anomalies \
	$(DOCDIR)/filtering \
	$(DOCDIR)/good-netkeeping-seal \
	$(DOCDIR)/internals.txt \
	$(DOCDIR)/iso2asc.txt \
	$(DOCDIR)/keymap.sample \
	$(DOCDIR)/mailcap.sample \
	$(DOCDIR)/mbox.5 \
	$(DOCDIR)/mime.types \
	$(DOCDIR)/mmdf.5 \
	$(DOCDIR)/newsoverview.5 \
	$(DOCDIR)/nov_tests \
	$(DOCDIR)/opt-case.1 \
	$(DOCDIR)/plp_snprintf.3 \
	$(DOCDIR)/pgp.txt \
	$(DOCDIR)/rcvars.txt \
	$(DOCDIR)/reading-mail.txt \
	$(DOCDIR)/umlaute.txt \
	$(DOCDIR)/umlauts.txt \
	$(DOCDIR)/url_handler.1 \
	$(DOCDIR)/tin.1 \
	$(DOCDIR)/tin.5 \
	$(DOCDIR)/tin.defaults \
	$(DOCDIR)/tinews.1 \
	$(DOCDIR)/tools.txt \
	$(DOCDIR)/w2r.1 \
	$(DOCDIR)/wildmat.3

TOL	= \
	$(TOLDIR)/expiretover \
	$(TOLDIR)/metamutt \
	$(TOLDIR)/opt-case.pl \
	$(TOLDIR)/tinlock \
	$(TOLDIR)/tinews.pl \
	$(TOLDIR)/url_handler.pl \
	$(TOLDIR)/url_handler.sh \
	$(TOLDIR)/w2r.pl \
	$(TOLDIR)/expand_aliases.tgz

TOP	= \
	$(TOPDIR)/Makefile \
	$(TOPDIR)/MANIFEST \
	$(TOPDIR)/README \
	$(TOPDIR)/README.MAC \
	$(TOPDIR)/README.WIN \
	$(TOPDIR)/aclocal.m4 \
	$(TOPDIR)/conf-tin \
	$(TOPDIR)/config.guess \
	$(TOPDIR)/config.sub \
	$(TOPDIR)/configure \
	$(TOPDIR)/configure.in \
	$(TOPDIR)/install.sh \
	$(TOPDIR)/mkdirs.sh \
	$(TOPDIR)/tin.spec

PCRE	= \
	$(PCREDIR)/AUTHORS \
	$(PCREDIR)/COPYING \
	$(PCREDIR)/ChangeLog \
	$(PCREDIR)/INSTALL \
	$(PCREDIR)/LICENCE \
	$(PCREDIR)/Makefile.in \
	$(PCREDIR)/Makefile.in-old \
	$(PCREDIR)/NEWS \
	$(PCREDIR)/NON-UNIX-USE \
	$(PCREDIR)/README \
	$(PCREDIR)/RunTest.in \
	$(PCREDIR)/config.h \
	$(PCREDIR)/configure.in \
	$(PCREDIR)/dftables.c \
	$(PCREDIR)/pcre-config.in \
	$(PCREDIR)/pcre.h \
	$(PCREDIR)/pcre_compile.c \
	$(PCREDIR)/pcre_config.c \
	$(PCREDIR)/pcre_dfa_exec.c \
	$(PCREDIR)/pcre_exec.c \
	$(PCREDIR)/pcre_fullinfo.c \
	$(PCREDIR)/pcre_get.c \
	$(PCREDIR)/pcre_globals.c \
	$(PCREDIR)/pcre_info.c \
	$(PCREDIR)/pcre_internal.h \
	$(PCREDIR)/pcre_maketables.c \
	$(PCREDIR)/pcre_newline.c \
	$(PCREDIR)/pcre_ord2utf8.c \
	$(PCREDIR)/pcre_printint.src \
	$(PCREDIR)/pcre_refcount.c \
	$(PCREDIR)/pcre_study.c \
	$(PCREDIR)/pcre_tables.c \
	$(PCREDIR)/pcre_try_flipped.c \
	$(PCREDIR)/pcre_ucp_searchfuncs.c \
	$(PCREDIR)/pcre_valid_utf8.c \
	$(PCREDIR)/pcre_version.c \
	$(PCREDIR)/pcre_xclass.c \
	$(PCREDIR)/pcredemo.c \
	$(PCREDIR)/pcregrep.c \
	$(PCREDIR)/pcreposix.c \
	$(PCREDIR)/pcreposix.h \
	$(PCREDIR)/pcretest.c \
	$(PCREDIR)/perltest \
	$(PCREDIR)/ucp.h \
	$(PCREDIR)/ucpinternal.h \
	$(PCREDIR)/ucptable.c \
	$(PCREDIR)/doc/pcre.3 \
	$(PCREDIR)/doc/pcrepattern.3 \
	$(PCREDIR)/testdata/testinput1 \
	$(PCREDIR)/testdata/testinput2 \
	$(PCREDIR)/testdata/testinput3 \
	$(PCREDIR)/testdata/testinput4 \
	$(PCREDIR)/testdata/testinput5 \
	$(PCREDIR)/testdata/testinput6 \
	$(PCREDIR)/testdata/testinput7 \
	$(PCREDIR)/testdata/testinput8 \
	$(PCREDIR)/testdata/testinput9 \
	$(PCREDIR)/testdata/testoutput1 \
	$(PCREDIR)/testdata/testoutput2 \
	$(PCREDIR)/testdata/testoutput3 \
	$(PCREDIR)/testdata/testoutput4 \
	$(PCREDIR)/testdata/testoutput5 \
	$(PCREDIR)/testdata/testoutput6 \
	$(PCREDIR)/testdata/testoutput7 \
	$(PCREDIR)/testdata/testoutput8 \
	$(PCREDIR)/testdata/testoutput9

CAN	= \
	$(CANDIR)/CHANGES \
	$(CANDIR)/HOWTO \
	$(CANDIR)/README \
	$(CANDIR)/Makefile.in \
	$(CANDIR)/src/base64.c \
	$(CANDIR)/src/canlock.c \
	$(CANDIR)/src/hmac_sha1.c \
	$(CANDIR)/src/sha1.c \
	$(CANDIR)/include/base64.h \
	$(CANDIR)/include/canlock.h \
	$(CANDIR)/include/hmac_sha1.h \
	$(CANDIR)/include/sha1.h \
	$(CANDIR)/t/canlocktest.c \
	$(CANDIR)/t/hmactest.c \
	$(CANDIR)/t/canlocktest.shouldbe \
	$(CANDIR)/t/hmactest.shouldbe

MISC	= \
	$(INCDIR)/autoconf.hin \
	$(SRCDIR)/Makefile.in \
	$(SRCDIR)/tincfg.tbl

INTLFILES = \
	$(INTLDIR)/bindtextdom.c \
	$(INTLDIR)/ChangeLog \
	$(INTLDIR)/config.charset \
	$(INTLDIR)/dcgettext.c \
	$(INTLDIR)/dcigettext.c \
	$(INTLDIR)/dcngettext.c \
	$(INTLDIR)/dgettext.c \
	$(INTLDIR)/dngettext.c \
	$(INTLDIR)/explodename.c \
	$(INTLDIR)/finddomain.c \
	$(INTLDIR)/gettext.c \
	$(INTLDIR)/gettext.h \
	$(INTLDIR)/gettextP.h \
	$(INTLDIR)/hash-string.h \
	$(INTLDIR)/intl-compat.c \
	$(INTLDIR)/l10nflist.c \
	$(INTLDIR)/libgettext.h \
	$(INTLDIR)/libgnuintl.h \
	$(INTLDIR)/loadinfo.h \
	$(INTLDIR)/loadmsgcat.c \
	$(INTLDIR)/localcharset.c \
	$(INTLDIR)/locale.alias \
	$(INTLDIR)/localealias.c \
	$(INTLDIR)/Makefile.in \
	$(INTLDIR)/ngettext.c \
	$(INTLDIR)/plural.c \
	$(INTLDIR)/plural.y \
	$(INTLDIR)/ref-add.sin \
	$(INTLDIR)/ref-del.sin \
	$(INTLDIR)/textdomain.c \
	$(INTLDIR)/VERSION

POFILES = \
	$(PODIR)/Makefile.inn \
	$(PODIR)/POTFILES.in \
	$(PODIR)/$(PROJECT).pot \
	$(PODIR)/de.gmo \
	$(PODIR)/de.po \
	$(PODIR)/en_GB.gmo \
	$(PODIR)/en_GB.po \
	$(PODIR)/et.gmo \
	$(PODIR)/et.po \
	$(PODIR)/fr.gmo \
	$(PODIR)/fr.po \
	$(PODIR)/ru.gmo \
	$(PODIR)/ru.po \
	$(PODIR)/rw.gmo \
	$(PODIR)/rw.po \
	$(PODIR)/sv.gmo \
	$(PODIR)/sv.po \
	$(PODIR)/tr.gmo \
	$(PODIR)/tr.po


ALL_FILES = $(TOP) $(DOC) $(TOL) $(HFILES) $(CFILES) $(PCRE) $(MISC) $(CAN) $(INTLFILES) $(POFILES)

ALL_DIRS = $(TOPDIR) $(DOCDIR) $(SRCDIR) $(INCDIR) $(PCREDIR) $(PCREDIR)/doc $(PCREDIR)/testdata $(CANDIR) $(INTLDIR) $(PODIR)

# standard commands
CD	= cd
CHMOD	= chmod
CP	= cp -p
ECHO	= echo
LS	= ls
MAKE	= make
MV	= mv
NROFF	= groff -Tascii
RM	= rm
SHELL	= /bin/sh
TAR	= tar
GZIP	= gzip
BZIP2	= bzip2
WC	= wc
SED	= sed
TR	= tr
TEST	= test


all:
	@$(ECHO) "Top level Makefile for the $(PROJECT) v$(VER) Usenet newsreader."
	@$(ECHO) " "
	@$(ECHO) "To compile the source code type 'make build' or change to the"
	@$(ECHO) "source directory by typing 'cd src' and then type 'make'."
	@$(ECHO) " "
	@$(ECHO) "This Makefile offers the following general purpose options:"
	@$(ECHO) " "
	@$(ECHO) "    make build           [ Compile $(PROJECT) ]"
	@$(ECHO) "    make clean           [ Delete all object and backup files ]"
	@$(ECHO) "    make dist            [ Create a gzipped & bzipped distribution tar file ]"
	@$(ECHO) "    make distclean       [ Delete all config, object and backup files ]"
	@$(ECHO) "    make install         [ Install the binary and the manual page ]"
	@$(ECHO) "    make install_sysdefs [ Install the system-wide-defaults file ]"
	@$(ECHO) "    make manpage         [ Create nroff version of manual page ]"
	@$(ECHO) "    make manifest        [ Create MANIFEST ]"
	@$(ECHO) " "

build:
	@-if $(TEST) -r $(SRCDIR)/Makefile ; then $(CD) $(SRCDIR) && $(MAKE) ; else $(ECHO) "You need to run configure first - didn't you read README?" ; fi

install:
	@$(CD) $(SRCDIR) && $(MAKE) install

install_sysdefs:
	@$(CD) $(SRCDIR) && $(MAKE) install_sysdefs

clean:
	@-$(RM) -f \
	*~ \
	$(DOCDIR)/*~ \
	$(INCDIR)/*~ \
	$(SRCDIR)/*~ \
	$(PCREDIR)/*~
	@-if $(TEST) -r $(PCREDIR)/Makefile ; then $(CD) $(PCREDIR) && $(MAKE) clean ; fi
	@-if $(TEST) -r $(INTLDIR)/Makefile ; then $(CD) $(INTLDIR) && $(MAKE) clean ; fi
	@-if $(TEST) -r $(PODIR)/Makefile ; then $(CD) $(PODIR) && $(MAKE) clean ; fi
	@-if $(TEST) -r $(SRCDIR)/Makefile ; then $(CD) $(SRCDIR) && $(MAKE) clean ; fi
	@-if $(TEST) -r $(CANDIR)/Makefile ; then $(CD) $(CANDIR) && $(MAKE) clean ; fi

man:
	@$(MAKE) manpage

manpage:
	@$(ECHO) "Creating $(NROFF) man page for $(EXE)-$(VER)..."
	@$(NROFF) -man $(DOCDIR)/tin.1 > $(DOCDIR)/$(EXE).nrf

# Use 2 passes for creating MANIFEST because its size changes (it's not likely
# that we'll need 3 passes, since that'll happen only when the grand total's
# digits change).
manifest:
	@$(ECHO) "Creating MANIFEST..."
	@$(ECHO) "MANIFEST for $(PROJECT)-$(VER) (`date`)" > MANIFEST.tmp
	@$(ECHO) "----------------------------------------------------" >> MANIFEST.tmp
	@$(CP) MANIFEST.tmp MANIFEST
	@$(WC) -c $(ALL_FILES) >> MANIFEST
	@$(WC) -c $(ALL_FILES) >> MANIFEST.tmp
	@$(MV) MANIFEST.tmp MANIFEST

chmod:
	@$(ECHO) "Setting the file permissions..."
	@$(CHMOD) 644 $(ALL_FILES)
	@$(CHMOD) 755 \
	$(ALL_DIRS) \
	$(TOPDIR)/conf-tin \
	$(TOPDIR)/config.guess \
	$(TOPDIR)/config.sub \
	$(TOPDIR)/configure \
	$(TOPDIR)/install.sh \
	$(TOPDIR)/mkdirs.sh \
	$(TOLDIR)/expiretover \
	$(TOLDIR)/metamutt \
	$(TOLDIR)/opt-case.pl \
	$(TOLDIR)/tinlock \
	$(TOLDIR)/tinews.pl \
	$(TOLDIR)/url_handler.pl \
	$(TOLDIR)/url_handler.sh \
	$(TOLDIR)/w2r.pl \
	$(PCREDIR)/perltest

tar:
	@$(ECHO) "Generating gzipped tar file..."
	@-$(RM) -f $(PROJECT)-$(VER).tar.gz
	@$(TAR) cvf $(PROJECT)-$(VER).tar -C ../ \
	`$(ECHO) $(ALL_FILES) \
	| $(TR) -s '[[:space:]]' "[\012*]" \
	| $(SED) 's,^\./,$(PROJECT)-$(VER)/,' \
	| $(TR) "[\012]" " "`
	@$(GZIP) -9 $(PROJECT)-$(VER).tar
	@$(CHMOD) 644 $(PROJECT)-$(VER).tar.gz
	@$(LS) -l $(PROJECT)-$(VER).tar.gz

bzip2:
	@$(ECHO) "Generating bzipped tar file..."
	@-$(RM) -f $(PROJECT)-$(VER).tar.bz2
	@$(TAR) cvf $(PROJECT)-$(VER).tar -C ../ \
	`$(ECHO) $(ALL_FILES) \
	| $(TR) -s '[[:space:]]' "[\012*]" \
	| $(SED) 's,^\./,$(PROJECT)-$(VER)/,' \
	| $(TR) "[\012]" " "`
	@$(BZIP2) -9 $(PROJECT)-$(VER).tar
	@$(CHMOD) 644 $(PROJECT)-$(VER).tar.bz2
	@$(LS) -l $(PROJECT)-$(VER).tar.bz2

#
# I know it's ugly, but it works
#
name:
	@DATE=`date +%Y%m%d` ; NAME=`basename \`pwd\`` ;\
	if $(TEST) $$NAME != "$(PROJECT)-$(VER)" ; then \
		$(MV) ../$$NAME ../$(PROJECT)-$(VER) ;\
	fi ;\
	$(SED) "s,^PACKAGE=[[:print:]]*,PACKAGE=$(PROJECT)," ./configure.in > ./configure.in.out && \
	$(SED) "s,^VERSION=[[:print:]]*,VERSION=$(VER)," ./configure.in.out > ./configure.in && \
	$(RM) ./configure.in.out ;\
	$(SED) "s,^DVER[[:space:]]*=[[:print:]]*,DVER	= $$DATE," ./Makefile > ./Makefile.tmp && \
	$(MV) ./Makefile.tmp ./Makefile ;\
	$(SED) "s,RELEASEDATE[[:space:]]*\"[[:print:]]*\",RELEASEDATE	\"$$DATE\"," $(INCDIR)/version.h > $(INCDIR)/version.h.tmp && \
	$(SED) "s, VERSION[[:space:]]*\"[[:print:]]*\", VERSION		\"$(VER)\"," $(INCDIR)/version.h.tmp > $(INCDIR)/version.h && \
	$(RM) $(INCDIR)/version.h.tmp ;\
	$(MAKE) configure

dist:
	@$(MAKE) name
	@-if $(TEST) -r $(PODIR)/Makefile ; then $(CD) $(PODIR) && $(MAKE) ; fi
	@$(MAKE) manifest
	@$(MAKE) chmod
	@$(MAKE) tar
	@$(MAKE) bzip2

version:
	@$(ECHO) "$(PROJECT)-$(VER)"

distclean:
	@-$(MAKE) clean
	@-if $(TEST) -r $(PODIR)/Makefile ; then $(CD) $(PODIR) && $(MAKE) distclean ; fi
	@-if $(TEST) -r $(INTLDIR)/Makefile ; then $(CD) $(INTLDIR) && $(MAKE) distclean ; fi
	@-if $(TEST) -r $(PCREDIR)/Makefile ; then $(CD) $(PCREDIR) && $(MAKE) distclean ; fi
	@-if $(TEST) -r $(CANDIR)/Makefile ; then $(CD) $(CANDIR) && $(MAKE) distclean ; fi
	@-$(RM) -f \
	$(TOPDIR)/config.cache \
	$(TOPDIR)/config.log \
	$(TOPDIR)/config.status \
	$(TOPDIR)/td-conf.out \
	$(INCDIR)/autoconf.h \
	$(SRCDIR)/Makefile \
	$(PCREDIR)/Makefile \
	$(CANDIR)/Makefile \
	$(INTLDIR)/po2tbl.sed

configure: configure.in aclocal.m4
	autoconf

config.status: configure
	$(TOPDIR)/config.status --recheck
