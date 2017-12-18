# ####################################################
# Hello World
# ####################################################

#changed as this option was given in path
CC = g++
CFLAGS = -lm `sdl2-config --cflags --libs` -fopenmp
LDFLAGS =

TARGET = raytracer
INC_DIR := -I.

SRCS := main.cpp maths.cpp
OBJS := $(SRCS:.c=.o)

#------------------------------------------------------
.SILENT:

#------------------------------------------------------
.SUFFIXES: .o .cpp .hpp

#------------------------------------------------------
all: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)
#------------------------------------------------------
clean:
	rm -f *.o *~ $(TARGET)
#------------------------------------------------------
distclean: clean
#------------------------------------------------------
.c.o:
	$(CC) -c $(CFLAGS) $(INC_DIR) -c $<