CFLAGS=$(CFLAGS) -DSTANDALONE= -I../../win32

dismips.exe: dismips.obj
	link $**

dismips.obj: dismips.c mipsdasm.c
