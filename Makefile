include Config.mk

################ Source files ##########################################

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)
INCS	= $(filter-out bsconf.%,$(wildcard *.h))
DOCT	= utiodoc.in

################ Library link names ####################################

TOCLEAN	+= ${LIBSO} ${LIBA} ${LIBSOBLD}

ALLINST	= install-incs
ifdef BUILD_SHARED
ALLLIBS	+= ${LIBSOBLD}
ALLINST	+= install-shared
endif
ifdef BUILD_STATIC
ALLLIBS	+= ${LIBA}
ALLINST	+= install-static
endif

################ Compilation ###########################################

.PHONY:	all html

all:	${ALLLIBS}

${LIBA}:	${OBJS}
	@echo "Linking $@ ..."
	@${AR} r $@ $?
	@${RANLIB} $@

${LIBSOBLD}:	${OBJS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} ${SHBLDFL} -o $@ $^ ${LIBS}

%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

html:
	@${DOXYGEN} ${DOCT}

################ Installation ##########################################

.PHONY: install uninstall install-incs uninstall-incs
.PHONY: install-static install-shared uninstall-static uninstall-shared

install:	${ALLINST}
uninstall:	$(subst install,uninstall,${ALLINST})

INSTALLDIR	= ${INSTALL} -d
INSTALLLIB	= ${INSTALL} -p -m 644
INSTALLEXE	= ${INSTALL} -p -m 755
INSTALLDATA	= ${INSTALL} -p -m 644

install-shared: ${LIBSOBLD} install-incs
	@echo "Installing ${LIBSOBLD} to ${LIBDIR} ..."
	@${INSTALLDIR} ${LIBDIR}
	@${INSTALLLIB} ${LIBSOBLD} ${LIBDIR}
	@(cd ${LIBDIR}; ${LN} -sf ${LIBSOBLD} ${LIBSO}; ${LN} -sf ${LIBSOBLD} ${LIBSOLNK})

uninstall-shared: uninstall-incs
	@echo "Removing ${LIBSOBLD} from ${LIBDIR} ..."
	@${RM} -f ${LIBDIR}/${LIBSO} ${LIBDIR}/${LIBSOLNK} ${LIBDIR}/${LIBSOBLD}

install-static: ${LIBA} install-incs
	@echo "Installing ${LIBA} to ${LIBDIR} ..."
	@${INSTALLDIR} ${LIBDIR}
	@${INSTALLLIB} ${LIBA} ${LIBDIR}

uninstall-static: uninstall-incs
	@echo "Removing ${LIBA} from ${LIBDIR} ..."
	@${RM} -f ${LIBDIR}/${LIBA}

install-incs: ${INCS}
	@echo "Installing headers to ${INCDIR} ..."
	@${INSTALLDIR} ${INCDIR}/${LIBNAME}
	@for i in $(filter-out ${LIBNAME}.h,${INCS}); do	\
	    ${INSTALLDATA} $$i ${INCDIR}/${LIBNAME}/$$i;	\
	done;
	@${INSTALLDATA} ${LIBNAME}.h ${INCDIR}

uninstall-incs:
	@echo "Removing headers from ${INCDIR} ..."
	@${RM} -rf ${INCDIR}/${LIBNAME} ${INCDIR}/${LIBNAME}.h

################ Maintenance ###########################################

.PHONY:	clean depend dist distclean maintainer-clean

clean:
	@echo "Removing generated files ..."
	@${RM} -f ${OBJS} ${TOCLEAN}
	@+${MAKE} -C demo clean

depend: ${SRCS}
	@${CXX} ${CXXFLAGS} -M ${SRCS} > .depend;

TMPDIR	= /tmp
DISTDIR	= ${HOME}/stored
DISTNAM	= ${LIBNAME}-${MAJOR}.${MINOR}
DISTTAR	= ${DISTNAM}.${BUILD}.tar.bz2

dist:
	mkdir ${TMPDIR}/${DISTNAM}
	cp -r . ${TMPDIR}/${DISTNAM}
	+${MAKE} -C ${TMPDIR}/${DISTNAM} distclean
	(cd ${TMPDIR}/${DISTNAM}; rm -rf `find . -name .svn`)
	(cd ${TMPDIR}; tar jcf ${DISTDIR}/${DISTTAR} ${DISTNAM}; rm -rf ${DISTNAM})

distclean:	clean
	@rm -f Config.mk config.h ${LIBNAME}.spec bsconf.o bsconf .depend demo/.depend

maintainer-clean: distclean
	@rm -rf docs/html

-include .depend
 
