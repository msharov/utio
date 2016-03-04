-include Config.mk

################ Source files ##########################################

SRCS	:= $(wildcard *.cc)
INCS	:= $(wildcard *.h)
OBJS	:= $(addprefix $O,$(SRCS:.cc=.o))
DEPS	:= ${OBJS:.o=.d}
MKDEPS	:= Makefile Config.mk config.h $O.d
ONAME	:= $(notdir $(abspath $O))
LIBA_R	:= $Olib${NAME}.a
LIBA_D	:= $Olib${NAME}_d.a
ifdef DEBUG
LIBA	:= ${LIBA_D}
else
LIBA	:= ${LIBA_R}
endif

################ Compilation ###########################################

.PHONY: all clean check distclean maintainer-clean

ALLTGTS	:= ${MKDEPS} ${LIBA}
all:	${ALLTGTS}

${LIBA}:	${OBJS}
	@echo "Linking $@ ..."
	@rm -f $@
	@${AR} qc $@ ${OBJS}
	@${RANLIB} $@

$O%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

include test/Module.mk

################ Installation ##########################################

.PHONY:	install uninstall install-incs uninstall-incs

####### Install headers

ifdef INCDIR	# These ifdefs allow cold bootstrap to work correctly
LIDIR	:= ${INCDIR}/${NAME}
INCSI	:= $(addprefix ${LIDIR}/,$(filter-out ${NAME}.h,${INCS}))
RINCI	:= ${LIDIR}.h

install:	install-incs
install-incs: ${INCSI} ${RINCI}
${INCSI}: ${LIDIR}/%.h: %.h
	@echo "Installing $@ ..."
	@${INSTALLDATA} $< $@
${RINCI}: ${NAME}.h
	@echo "Installing $@ ..."
	@${INSTALLDATA} $< $@
uninstall:	uninstall-incs
uninstall-incs:
	@if [ -d ${LIDIR} -o -f ${RINCI} ]; then\
	    echo "Removing ${LIDIR}/ and ${RINCI} ...";\
	    rm -f ${INCSI} ${RINCI};\
	    ${RMPATH} ${LIDIR};\
	fi
endif

####### Install libraries (shared and/or static)

ifdef LIBDIR
LIBAI	:= ${LIBDIR}/$(notdir ${LIBA})
install:	${LIBAI}
${LIBAI}:	${LIBA}
	@echo "Installing $@ ..."
	@${INSTALLLIB} $< $@

uninstall:
	@if [ -f ${LIBAI} ]; then\
	    echo "Removing library from ${LIBDIR} ...";\
	    rm -f ${LIBAI};\
	    ${RMPATH} ${LIBDIR};\
	fi
endif

################ Maintenance ###########################################

clean:
	@if [ -h ${ONAME} ]; then\
	    rm -f ${OBJS} ${DEPS} ${LIBA_R} ${LIBA_D} $O.d ${ONAME};\
	    ${RMPATH} ${BUILDDIR};\
	fi

distclean:	clean
	@rm -f Config.mk config.h config.status

maintainer-clean: distclean

$O.d:	${BUILDDIR}/.d
	@[ -h ${ONAME} ] || ln -sf ${BUILDDIR} ${ONAME}
${BUILDDIR}/.d:	Makefile
	@mkdir -p ${BUILDDIR} && touch ${BUILDDIR}/.d

${OBJS}:		${MKDEPS}
Config.mk:		Config.mk.in
config.h:		config.h.in
Config.mk config.h:	configure
	@if [ -x config.status ]; then			\
	    echo "Reconfiguring ..."; ./config.status;	\
	else						\
	    echo "Running configure ..."; ./configure;	\
	fi

-include ${DEPS}
