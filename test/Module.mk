################ Source files ##########################################

test/srcs	:= $(wildcard test/?????.cc)
test/bins	:= $(addprefix $O,$(test/srcs:.cc=))
test/objs	:= $(addprefix $O,$(test/srcs:.cc=.o)) $Otest/stdmain.o
test/deps	:= ${test/objs:.o=.d}
test/outs	:= $(addprefix $O,$(test/srcs:.cc=.out))

################ Compilation ###########################################

.PHONY:	test/all test/run test/clean test/check

test/all:	${test/bins}

# The correct output of a test is stored in testXX.std
# When the test runs, its output is compared to .std
#
check:		test/check
test/check:	${test/bins}
	@for i in ${test/bins}; do \
	    echo "Running $$i"; \
	    TERM=xterm COLUMNS=80 LINES=24 $$i < test/`basename $$i`.cc > $$i.out 2>&1; \
	    diff test/`basename $$i`.std $$i.out && rm -f $$i.out; \
	done

${test/bins}: $Otest/%: $Otest/%.o $Otest/stdmain.o ${liba}
	@echo "Linking $@ ..."
	@${CC} ${ldflags} -o $@ $^ ${libs}

################ Maintenance ###########################################

clean:	test/clean
test/clean:
	@if [ -d ${builddir}/test ]; then\
	    rm -f ${test/bins} ${test/objs} ${test/deps} ${test/outs} $Otest/.d;\
	    rmdir ${builddir}/test;\
	fi

${test/objs}:	Makefile test/Module.mk ${confs} $Otest/.d

-include ${test/deps}
