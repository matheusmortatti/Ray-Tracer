# ####################################################
# Hello World
# ####################################################

# -omptargets=x86_64-unknown-linux-spark -fopenmp-targets=x86_64-unknown-linux-gnu
#changed as this option was given in path
CC = clang++
CCFLAGS = -fopenmp -omptargets=x86_64-unknown-linux-spark
LDFLAGS = -lm `sdl2-config --cflags --libs`

TARGET = raytracer
INC_DIR := -I.

SRCS := main.cpp maths.cpp
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
