#
# Extra rules for building the emulation engine
#
#CPUFLAGS = -O3 -mpentium -fomit-frame-pointer -Wall -Wno-unused -Wno-format -W -Wmissing-prototypes -Wstrict-prototypes -DX86_ASSEMBLY -D__inline__=inline -DSTATFS_NO_ARGS=2 -DSTATBUF_BAVAIL=f_bavail

# trying to be portable... *TRYING*
#CFLAGS   = -mms-bitfields
CPUFLAGS = $(CFLAGS) -fomit-frame-pointer -Wno-unused -Wno-format -W -Wmissing-prototypes -Wstrict-prototypes -D__inline__=inline -DSTATFS_NO_ARGS=2 -DSTATBUF_BAVAIL=f_bavail #-Wall

gen: cpudefs.c cpustbl.c cputbl.h cpuemu.c

# WARNING: Don't try to build newcpu.o here, because newcpu.c references TiEmu include files.
all: cpudefs.o cpustbl.o readcpu.o fpp.o missing.o cpuemu1.o cpuemu2.o cpuemu3.o cpuemu4.o cpuemu5.o cpuemu6.o cpuemu7.o cpuemu8.o

# For cross-compiling (generators are run on host)
build68k_host.o: build68k.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?
gencpu_host.o: gencpu.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?
readcpu_host.o: readcpu.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?
cpudefs_host.o: cpudefs.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?
missing_host.o: missing.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?
xmalloc_host.o: xmalloc.c
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc -c -o $@ $?

# Build generators and files to generate
build68k: build68k_host.o
	@echo "-> Compiling 68k builder..."
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc $(LDFLAGS) -o $@ $?

gencpu: gencpu_host.o readcpu_host.o cpudefs_host.o missing_host.o xmalloc_host.o
	@echo "-> Compiling CPU generator..."
	C_INCLUDE_PATH="" LIBRARY_PATH="" gcc $(LDFLAGS) -o $@ gencpu_host.o readcpu_host.o cpudefs_host.o missing_host.o xmalloc_host.o

cpudefs.c: build68k table68k
	@echo "-> Building CPU definitions..."
	./build68k <table68k >cpudefs.c

cpuemu.c: gencpu
	@echo "-> Generating CPU tables..."
	./gencpu

# gencpu also creates cpustbl.c and cputbl.h
cpustbl.c: cpuemu.c
cputbl.h: cpuemu.c

# CPU engine
cpuemu1.o: cpuemu.c
	$(CC) -DPART_1 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu2.o: cpuemu.c
	$(CC) -DPART_2 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu3.o: cpuemu.c
	$(CC) -DPART_3 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu4.o: cpuemu.c
	$(CC) -DPART_4 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu5.o: cpuemu.c
	$(CC) -DPART_5 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu6.o: cpuemu.c
	$(CC) -DPART_6 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu7.o: cpuemu.c
	$(CC) -DPART_7 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpuemu8.o: cpuemu.c
	$(CC) -DPART_8 $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@

# Misc
cpudefs.o: cpudefs.c
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
cpustbl.o: cputbl.h
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $*.c
readcpu.o: readcpu.h
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $*.c
fpp.o: fpp.c
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@
missing.o: missing.c
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@ 
newcpu.o: newcpu.c newcpu.h
	$(CC) $(INCLUDES) -c $(INCDIRS) $(CPUFLAGS) $< -o $@

clean:
	$(RM) build68k gencpu *.o
	$(RM) build68k.exe gencpu.exe

distclean: clean
	$(RM) cpudefs.c cpustbl.c cputbl.? cpuemu.c cpuemu?.o
