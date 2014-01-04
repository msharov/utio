-include Config.mk

################ Source files ##########################################

SRCS	:= $(wildcard *.cc)
INCS	:= $(wildcard *.h)
OBJS	:= $(addprefix $O,$(SRCS:.cc=.o))
DEPS	:= ${OBJS:.o=.d}

################ Compilation ###########################################

.PHONY: all clean check distclean maintainer-clean

all:	Config.mk config.h ${NAME}/config.h
ALLTGTS	:= Config.mk config.h ${NAME}/config.h

SLIBL	:= $O$(call slib_lnk,${NAME})
SLIBS	:= $O$(call slib_son,${NAME})
SLIBT	:= $O$(call slib_tgt,${NAME})
ifdef BUILD_SHARED
ALLTGTS	+= ${SLIBT} ${SLIBS} ${SLIBL}

all:	${SLIBT} ${SLIBS} ${SLIBL}
${SLIBT}:	${OBJS}
	@echo "Linking $(notdir $@) ..."
	@${LD} -fPIC ${LDFLAGS} $(call slib_flags,$(subst $O,,${SLIBS})) -o $@ $^ ${LIBS}
${SLIBS} ${SLIBL}:	${SLIBT}
	@(cd $(dir $@); rm -f $(notdir $@); ln -s $(notdir $<) $(notdir $@))

endif
ifdef BUILD_STATIC
LIBA	:= $Olib${NAME}.a
ALLTGTS	+= ${LIBA}

all:	${LIBA}
${LIBA}:	${OBJS}
	@echo "Linking $@ ..."
	@rm -f $@
	@${AR} qc $@ ${OBJS}
	@${RANLIB} $@
endif

$O%.o:	%.cc
	@echo "    Compiling $< ..."
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
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
	    echo "Removing ${LIDIR}/ and ${LIDIR}.h ...";\
	    rm -f ${INCSI} ${RINCI};\
	    rmdir ${LIDIR};\
	fi
endif

####### Install libraries (shared and/or static)

ifdef LIBDIR
ifdef BUILD_SHARED
LIBTI	:= ${LIBDIR}/$(notdir ${SLIBT})
LIBLI	:= ${LIBDIR}/$(notdir ${SLIBS})
LIBSI	:= ${LIBDIR}/$(notdir ${SLIBL})
install:	${LIBTI} ${LIBLI} ${LIBSI}
${LIBTI}:	${SLIBT}
	@echo "Installing $@ ..."
	@${INSTALLLIB} $< $@
${LIBLI} ${LIBSI}: ${LIBTI}
	@(cd ${LIBDIR}; rm -f $@; ln -s $(notdir $<) $(notdir $@))
endif
ifdef BUILD_STATIC
LIBAI	:= ${LIBDIR}/$(notdir ${LIBA})
install:	${LIBAI}
${LIBAI}:	${LIBA}
	@echo "Installing $@ ..."
	@${INSTALLLIB} $< $@
endif

uninstall:
	@echo "Removing library from ${LIBDIR} ..."
	@rm -f ${LIBTI} ${LIBLI} ${LIBSI} ${LIBAI}
endif

################ Maintenance ###########################################

clean:
	@if [ -d $O ]; then\
	    rm -f ${SLIBS} ${SLIBT} ${SLIBL} ${LIBA} ${OBJS} ${DEPS};\
	    rmdir $O;\
	fi

distclean:	clean
	@rm -f Config.mk config.h config.status ${NAME}

maintainer-clean: distclean

INPLACE_INCS := $(addprefix ${NAME}/,$(filter-out config.h,${INCS}))
${INPLACE_INCS}: ${NAME}/%:	${NAME}/config.h
${NAME}/config.h:	config.h
	@echo "    Linking inplace header location ..."
	@rm -f ${NAME}; ln -s . ${NAME}

${OBJS}:		Makefile Config.mk config.h
Config.mk:		Config.mk.in
config.h:		config.h.in
Config.mk config.h:	configure
	@if [ -x config.status ]; then echo "Reconfiguring ..."; ./config.status; \
	else echo "Running configure ..."; ./configure; fi

-include ${DEPS}
