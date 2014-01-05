################ Source files ##########################################

test/SRCS	:= $(wildcard test/test?.cc)
test/BVTS	:= $(test/SRCS:.cc=)
test/OBJS	:= $(addprefix $O,$(test/SRCS:.cc=.o)) $Otest/stdmain.o
test/DEPS	:= ${test/OBJS:.o=.d}
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
	@for i in ${test/BVTS}; do \
	    echo "Running $$i"; \
	    TERM=xterm ./$$i < $$i.cc &> $$i.out; \
	    cmp $$i.std $$i.out && rm -f $$i.out; \
	done

${test/BVTS}: test/%: $Otest/%.o $Otest/stdmain.o ${ALLTGTS}
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $< $Otest/stdmain.o ${test/LIBS}

################ Maintenance ###########################################

clean:	test/clean
test/clean:
	@if [ -d $Otest ]; then\
	    rm -f ${test/BVTS} ${test/OBJS} ${test/DEPS};\
	    rmdir $Otest;\
	fi
check:		test/run
test/check:	check

${test/OBJS}: Makefile test/Module.mk Config.mk ${NAME}/config.h

-include ${test/DEPS}
