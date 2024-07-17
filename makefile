CC=gcc
CFLAGS=-g -Wall

all: clean compile_src compile_test make_disk 

compile_src:
	$(CC) src/jnfss.c $(CFLAGS) -c -o jnfss.obj

compile_test:
	$(CC) test/test.c jnfss.obj $(CFLAGS) -o testexec

make_disk:
	dd if=/dev/zero of=testdisk.img bs=512 count=2880

clean:
	rm testexec jnfss.obj testdisk.img