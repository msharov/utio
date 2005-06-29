include ../Common.mk

DEMOS	= demo00 demo01 demo02
OBJS	= demo00.o demo01.o demo02.o stdmain.o
SRCS	= demo00.cc demo01.cc demo02.cc stdmain.cc
LIBS	+= -l${LIBNAME} -lustl -lsupc++

.PHONY:	all run clean depend

all:	stdmain.o ${DEMOS}

demo%:	demo%.o stdmain.o
	@echo "Linking $@ ..."
	@${LD} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o:	%.cc
	@echo "    Compiling $< ..."
	@${CXX} ${CXXFLAGS} -o $@ -c $<

%.s:	%.cc
	@echo "    Compiling $< to assembly ..."
	@${CXX} ${CXXFLAGS} -S -o $@ -c $<

clean:
	@echo "Removing generated files ..."
	@${RM} -f ${OBJS} ${TOCLEAN} ${DEMOS}

depend: ${SRCS}
	@${CXX} ${CXXFLAGS} -M ${SRCS} > .depend

-include .depend
 
