# Targets & general dependencies
PROGRAM = xsim1
HEADERS = xis.h xcpu.h xmem.h
OBJS = main.o xcpu.o xmem.o xcpuprnt.o
ADD_OBJS = 

# compilers, linkers, utilities, and flags
CC = gcc
CFLAGS = -Wall -g
COMPILE = $(CC) $(CFLAGS)
LINK = $(CC) $(CFLAGS) -o $@ 

# implicit rule to build .o from .c files
%.o: %.c $(HEADERS)
	$(COMPILE) -c -o $@ $<


# explicit rules
all: xas $(PROGRAM) 

$(PROGRAM): $(OBJS) $(ADD_OBJS)
	$(LINK) $(OBJS) $(ADD_OBJS)

xas: xas.o xreloc.o
	$(LINK) xas.o xreloc.o
	
clean:
	rm -f *.o *.xo *.xx $(PROGRAM) xas 
