# djoy Makefile
# Charlotte d'Arcangelo <dressupgeekout@gmail.com>
.PHONY: all clean
.SUFFIXES: .c .o

LUA_CFLAGS=		-I/opt/pkg/include/lua-5.3
LUA_LDFLAGS=	-L/opt/pkg/lib
LUA_LIBS=		-llua5.3

SDL_CONFIG=		sdl2-config
SDL_CFLAGS=		`$(SDL_CONFIG) --cflags`
SDL_LDFLAGS=
SDL_LIBS=		`$(SDL_CONFIG) --libs`

SDL_MIXER_CONFIG=	pkg-config
SDL_MIXER_CFLAGS=	`$(SDL_MIXER_CONFIG) --cflags SDL2_mixer`
SDL_MIXER_LDFLAGS=
SDL_MIXER_LIBS=		`$(SDL_MIXER_CONFIG) --libs SDL2_mixer`

CFLAGS=		$(LUA_CFLAGS) $(SDL_CFLAGS) $(SDL_MIXER_CFLAGS)
LDFLAGS=	$(LUA_LDFLAGS) $(SDL_LDFLAGS) $(SDL_MIXER_LDFLAGS)
LIBS=		$(LUA_LIBS) $(SDL_LIBS) $(SDL_MIXER_LIBS)

PROG=	djoy

all: $(PROG)

$(PROG): djoy.o
	$(CC) -g -Wall -o $(.TARGET) $(.ALLSRC) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) -c -g -Wall $(CFLAGS) -o $(.TARGET) $(.ALLSRC)

clean:
	rm -f $(PROG) *.o *.so
