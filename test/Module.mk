################ Source files ##########################################

test/SRCS	:= $(wildcard test/test?.cc)
test/BVTS	:= $(addprefix $O,$(test/SRCS:.cc=))
test/OBJS	:= $(addprefix $O,$(test/SRCS:.cc=.o)) $Otest/stdmain.o
test/DEPS	:= ${test/OBJS:.o=.d}
test/OUTS	:= $(addprefix $O,$(test/SRCS:.cc=.out))
ifdef BUILD_SHARED
test/LIBS	:= -L$(abspath $O.) -l${NAME} -Wl,--rpath=$(abspath $O.)
else
test/LIBS	+= ${LIBA} ${LIBS}
endif
CXXFLAGS	+= -I.

################ Compilation ###########################################

.PHONY:	test/all test/run test/clean test/check

test/all:	${test/BVTS}

# The correct output of a test is stored in testXX.std
# When the test runs, its output is compared to .std
#
test/run:	${test/BVTS}
	@for i in 0 1 2; do \
	    echo "Running $Otest/test$$i"; \
	    TERM=xterm $Otest/test$$i < test/test$$i.cc &> $Otest/test$$i.out; \
	    cmp test/test$$i.std $Otest/test$$i.out && rm -f $Otest/test$$i.out; \
	done

${test/BVTS}: $Otest/%: $Otest/%.o $Otest/stdmain.o ${ALLTGTS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $< $Otest/stdmain.o ${test/LIBS}

################ Maintenance ###########################################

clean:	test/clean
test/clean:
	@if [ -d $Otest ]; then\
	    rm -f ${test/BVTS} ${test/OBJS} ${test/DEPS} ${test/OUTS};\
	    rmdir $Otest;\
	fi
check:		test/run
test/check:	check

${test/OBJS}: Makefile test/Module.mk Config.mk ${NAME}/config.h

-include ${test/DEPS}
