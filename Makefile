MYFLAGS=-g
#MYFLAGS=-g -march=pentium3 -O2
CC=g++

#MYFLAGS=-march=pentiumiii -O2
#CC=/usr/local/intel/compiler70/ia32/bin/icc 

grapher: grapher.o graph_area.o func.o parse.o deriv.o
	${CC} `pkg-config --libs libglademm-2.0` `pkg-config --libs gtkmm-2.0` -o grapher grapher.o graph_area.o func.o parse.o deriv.o

grapher.o: grapher.cc func.h graph_area.h graph_area.o
	${CC} `pkg-config --cflags libglademm-2.0` `pkg-config --cflags gtkmm-2.0` ${MYFLAGS} -c grapher.cc

temp_graph: temp_graph.o graph_area.o func.o parse.o deriv.o
	${CC} `pkg-config --libs gtkmm-2.0` -o temp_graph temp_graph.o graph_area.o func.o parse.o deriv.o

temp_graph.o: temp_graph.cc func.h graph_area.h graph_area.o
	${CC} `pkg-config --cflags gtkmm-2.0` ${MYFLAGS} -c temp_graph.cc

graph_area.o: graph_area.h graph_area.cc func.h
	${CC} `pkg-config --cflags gtkmm-2.0` ${MYFLAGS} -c graph_area.cc

func.o: func.cc func.h parse.o
	${CC} ${MYFLAGS} -c func.cc

parse.o: parse.h func.h parse.cc
	${CC} ${MYFLAGS} -c parse.cc

deriv.o: deriv.h func.h deriv.cc
	${CC} ${MYFLAGS} -c deriv.cc

clean:
	rm -f grapher *.o
