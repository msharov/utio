include Common.mk

SRCS	= $(wildcard *.cc)
OBJS	= $(SRCS:.cc=.o)
INCS	= $(filter-out bsconf.%,$(wildcard *.h))
DOCT	= utiodoc.in

########################################################################

.PHONY:	all install uninstall install-incs uninstall-inst

ifdef BUILD_SHARED
ifdef BUILD_STATIC
all:		${LIBA} ${LIBSOBLD}
install:	install-static install-shared
uninstall:	uninstall-static uninstall-shared
else
all:		${LIBSOBLD}
install:	install-shared
uninstall:	uninstall-shared
endif
else
all:		${LIBA}
install:	install-static
uninstall:	uninstall-static
endif

${LIBA}:	${OBJS}
	@echo "Linking $@ ..."
	@${AR} r $@ $?
	@${RANLIB} $@

${LIBSOBLD}:	${OBJS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} ${SHBLDFL} -o $@ $^ ${LIBS}

.PHONY: install-static install-shared uninstall-static uninstall-shared

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


%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

.PHONY:	gch clean depend dox dist dist-clean maintainer-clean

clean:
	@echo "Removing generated files ..."
	@${RM} -f ${OBJS} ${TOCLEAN} *.rpo
	@+${MAKE} -C demo clean

depend: ${SRCS}
	@${CXX} ${CXXFLAGS} -M ${SRCS} > .depend;

dox:
	@${DOXYGEN} ${DOCT}

TMPDIR	= /tmp
DISTDIR	= ${HOME}/stored
DISTNAM	= ${LIBNAME}-${MAJOR}.${MINOR}
DISTTAR	= ${DISTNAM}-${BUILD}.tar.bz2

dist:
	mkdir ${TMPDIR}/${DISTNAM}
	cp -r . ${TMPDIR}/${DISTNAM}
	+${MAKE} -C ${TMPDIR}/${DISTNAM} dist-clean
	(cd ${TMPDIR}/${DISTNAM}; rm -rf CVS; cd docs; rm -rf CVS)
	(cd ${TMPDIR}; tar jcf ${DISTDIR}/${DISTTAR} ${DISTNAM}; rm -rf ${DISTNAM})

dist-clean:	clean
	@rm -f Common.mk config.h ${LIBNAME}.spec bsconf.o bsconf .depend demo/.depend

maintainer-clean: dist-clean
	@rm -rf docs/html

-include .depend
 
