ARCH	= 64

CC	= tcc -m$(ARCH)
CFLAGS = -O

PROG	= mk.exe
OBJ	= check.o input.o macro.o main.o make.o reader.o rules.o archive.o


all:	$(PROG)

$(PROG):	$(OBJ)
	$(CC) -o $@ $(OBJ)

clean:
	$(RM) *.o *.bak core

clobber: clean
	$(RM) $(PROG)
