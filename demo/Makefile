include ../Common.mk

DEMOCCS	= $(wildcard demo*.cc)
SRCS	= ${DEMOCCS} stdmain.cc

DEMOS	= $(DEMOCCS:.cc=)
OBJS	= $(SRCS:.cc=.o)
LIBS	+= -l${LIBNAME}

.PHONY:	all clean depend

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
 
