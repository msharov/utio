include ../Common.mk

DEMOS	= demo00 demo01
OBJS	= demo00.o demo01.o
SRCS	= demo00.cc demo01.cc
LIBS	= -l${LIBNAME} -lustl

.PHONY:	all run clean depend

all:	${DEMOS}

demo%:	demo%.o
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
 
