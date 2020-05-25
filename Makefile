-include Config.mk

################ Source files ##########################################

srcs	:= $(wildcard *.cc)
incs	:= $(filter-out ${name}.h,$(sort $(wildcard *.h) config.h))
objs	:= $(addprefix $O,$(srcs:.cc=.o))
deps	:= ${objs:.o=.d}
docs	:= $(wildcard docs/*)
confs	:= Config.mk config.h ${name}.pc
oname   := $(notdir $(abspath $O))
liba_r	:= $Olib${name}.a
liba_d	:= $Olib${name}_d.a
ifdef debug
liba	:= ${liba_d}
else
liba	:= ${liba_r}
endif

################ Compilation ###########################################

.SUFFIXES:
.PHONY: all clean distclean maintainer-clean

all:	${liba}

${liba}:	${objs}
	@echo "Linking $@ ..."
	@rm -f $@
	@${AR} qc $@ $^
	@${RANLIB} $@

$O%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${cxxflags} -MMD -MT "$(<:.cc=.s) $@" -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${cxxflags} -S -o $@ -c $<

################ Installation ##########################################

.PHONY:	install installdirs install-incs install-html
.PHONY:	uninstall uninstall-incs uninstall-lib uninstall-pc uninstall-html

ifdef includedir
incsd	:= ${DESTDIR}${includedir}/${name}
incsi	:= $(addprefix ${incsd}/,${incs})
incr	:= ${incsd}.h

${incsd}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${incsi}: ${incsd}/%.h: %.h | ${incsd}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@
${incr}:	${name}.h | ${incsd}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@

installdirs:	${incsd}
install:	${incsi} ${incr}
uninstall:	uninstall-incs
uninstall-incs:
	@if [ -d ${incsd} ]; then\
	    echo "Removing headers ...";\
	    rm -f ${incsi} ${incr};\
	    rmdir ${incsd};\
	fi
endif

ifdef libdir
libad	:= ${DESTDIR}${libdir}
libai	:= ${libad}/$(notdir ${liba})
libai_r	:= ${libad}/$(notdir ${liba_r})
libai_d	:= ${libad}/$(notdir ${liba_d})

${libad}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${libai}:	${liba} | ${libad}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@

installdirs:	${libad}
install:	${libai}
uninstall:	uninstall-lib
uninstall-lib:
	@if [ -f ${libai_r} -o -f ${libai_d} ]; then\
	    echo "Removing ${libai} ...";\
	    rm -f ${libai_r} ${libai_d};\
	fi
endif

ifdef docdir
docsd	:= ${DESTDIR}${docdir}/${name}
docsi	:= $(addprefix ${docsd}/,$(notdir ${docs}))

${docsd}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${docsi}: ${docsd}/%: docs/% | ${docsd}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@

install:	install-html
install-html:	${docsi}
installdirs:	${docsd}
uninstall:	uninstall-html
uninstall-html:
	@if [ -d ${docsd} ]; then\
	    echo "Removing documentation ...";\
	    rm -f ${docsi};\
	    rmdir ${docsd};\
	fi
endif

ifdef pkgconfigdir
pcd	:= ${DESTDIR}${pkgconfigdir}
pci	:= ${pcd}/${name}.pc

${pcd}:
	@echo "Creating $@ ..."
	@${INSTALL} -d $@
${pci}:	${name}.pc | ${pcd}
	@echo "Installing $@ ..."
	@${INSTALL_DATA} $< $@

installdirs:	${pcd}
install:	${pci}
uninstall:	uninstall-pc
uninstall-pc:
	@if [ -f ${pci} ]; then\
	    echo "Removing ${pci} ...";\
	    rm -f ${pci};\
	fi
endif

################ Maintenance ###########################################

include test/Module.mk

clean:
	@if [ -d ${builddir} ]; then\
	    rm -f ${liba_r} ${liba_d} ${objs} ${deps} $O.d;\
	    rmdir ${builddir};\
	fi

distclean:	clean
	@rm -f ${oname} ${confs} config.status

maintainer-clean: distclean

$O.d:	${builddir}/.d
	@[ -h ${oname} ] || ln -sf ${builddir} ${oname}
$O%/.d:	$O.d
	@[ -d $(dir $@) ] || mkdir $(dir $@)
	@touch $@
${builddir}/.d:	Makefile
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@touch $@

Config.mk:	Config.mk.in
config.h:	config.h.in
${name}.pc:	${name}.pc.in
${objs}:	Makefile ${confs} $O.d
${confs}:	configure
	@if [ -x config.status ]; then echo "Reconfiguring ...";\
	    ./config.status;\
	else echo "Running configure ...";\
	    ./configure;\
	fi

-include ${deps}
