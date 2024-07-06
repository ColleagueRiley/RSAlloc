CC = gcc

LIBS :=-lgdi32 -lm -lopengl32 -lwinmm -ggdb 
EXT = .exe
LIB_EXT = .dll
STATIC =

WARNINGS = -Wall -Werror -Wstrict-prototypes -Wextra

ifneq (,$(filter $(CC),winegcc x86_64-w64-mingw32-gcc i686-w64-mingw32-gcc))
	STATIC = --static
    detected_OS := Windows
	LIB_EXT = .dll
else
	ifeq '$(findstring ;,$(PATH))' ';'
		detected_OS := Windows
	else
		detected_OS := $(shell uname 2>/dev/null || echo Unknown)
		detected_OS := $(patsubst CYGWIN%,Cygwin,$(detected_OS))
		detected_OS := $(patsubst MSYS%,MSYS,$(detected_OS))
		detected_OS := $(patsubst MINGW%,MSYS,$(detected_OS))
	endif
endif

ifeq ($(detected_OS),Windows)
	LIBS := -ggdb -ldwmapi -lshell32 -lwinmm -lgdi32 -lopengl32 $(STATIC)
	EXT = .exe
	LIB_EXT = .dll
endif
ifeq ($(detected_OS),Darwin)        # Mac OS X
	LIBS := -lm -framework Foundation -framework AppKit -framework OpenGL -framework CoreVideo$(STATIC)
	EXT = 
	LIB_EXT = .dylib
endif
ifeq ($(detected_OS),Linux)
    LIBS := -lXrandr -lX11 -lm -lGL -ldl -lpthread $(STATIC)
	EXT =
	LIB_EXT = .so
endif

all:
	$(CC) test.c $(WARNINGS)  -o test
	$(CC) test.c -D RSA_USE_MALLOC $(WARNIGNS) -o test-malloc
	$(CC) test.c $(LIBS) $(WARNINGS) -o test-bss -D RSA_BSS
	$(CC) RGFW-test.c $(LIBS) $(WARNINGS)  -o test-RGFW

debug:
	$(CC) test.c $(WARNINGS) -D RSA_DEBUG  -o test
	$(CC) test.c -D RSA_USE_MALLOC -D RSA_DEBUG $(WARNIGNS) -o test-malloc
	$(CC) test.c $(WARNINGS) -D RSA_DEBUG  -o test-bss -D RSA_BSS
	$(CC) RGFW-test.c $(LIBS) $(WARNINGS) -D RSA_DEBUG -o test-RGFW

	./test$(EXT)
	./test-malloc$(EXT)
	./test-bss$(EXT)
	./test-RGFW$(EXT)

clean:
	rm test$(EXT) test-bss$(EXT) test-RGFW$(EXT) *.exe