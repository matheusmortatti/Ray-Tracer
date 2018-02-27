# CCFLAGS options for omp targets
# -omptargets=x86_64-unknown-linux-spark -fopenmp-targets=x86_64-unknown-linux-gnu `sdl2-config --cflags --libs`

###################################################
# Compile Flag Options:
#
# -> UNLIT: whether or not the scene is unlit
# -> BENCHMIN: benchmark easy mode (~1 min)
# -> BENCHMID: benchmark medium mode (~6 min)
# -> BENCHMAX: benchmark hard mode (~15 min)
###################################################

CC = clang++
CCFLAGS = -fopenmp -std=c++0x -pthread -omptargets=x86_64-unknown-linux-spark -DBENCHMIN
LDFLAGS = -lm

TARGET = raytracer
INC_DIR := -I.

SRCS := main.cpp maths.cpp renderer.cpp
OBJS := $(SRCS:.c=.o)

#------------------------------------------------------
.SUFFIXES: .o .cpp .hpp

#------------------------------------------------------
all: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o $(TARGET) $(LDFLAGS)
#------------------------------------------------------
clean:
	rm -f *.o *~ $(TARGET)
	rm -rf *.scala *.sbt project target spark-warehouse *.ii *.ii.tgt* *.s *.s.tgt* *.bc *.bc.tgt* *.so.tgt*
#------------------------------------------------------
.c.o:
	$(CC) -c $(CCFLAGS) $(INC_DIR) -c $<
