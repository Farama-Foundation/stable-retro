TARGET := datmagic

# Attempt to detect target platform
ifeq '$(findstring ;,$(PATH))' ';'
	UNAME := Windows
else
	UNAME := $(shell uname 2>/dev/null || echo Unknown)
	UNAME := $(patsubst CYGWIN%,Cygwin,$(UNAME))
	UNAME := $(patsubst MSYS%,MSYS,$(UNAME))
	UNAME := $(patsubst MINGW%,MSYS,$(UNAME))
endif

# Add '.exe' extension on Windows platforms
ifeq ($(UNAME), Windows)
	TARGET := datmagic.exe
endif
ifeq ($(UNAME), MSYS)
	TARGET := datmagic.exe
endif

CFLAGS := -O2 -Wall -Wextra

all: $(TARGET)

clean:
	rm -f $(TARGET) mame2003-plus.html

$(TARGET): datmagic.c
	$(CC) $(CFLAGS) -o $@ $<
