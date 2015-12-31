/*
 * vim: noet
 *
 * djoy.c
 * Charlotte d'Arcangelo <dressupgeekout@gmail.com>
 *
 * NOTES
 *  - will only use the first available joystick.
 */

#include <err.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_image.h>

#define MAXCHUNKS 16
#define MAXIMAGES MAXCHUNKS

/* */

struct globalstate {
	char script_file[PATH_MAX];
	char joymap_file[PATH_MAX];
	SDL_Window *win;
	lua_State *L;
	SDL_Joystick *joy;
	Mix_Chunk *clips[MAXCHUNKS];
	SDL_Surface *images[MAXIMAGES];
	bool verbose;
};

static int parse_options(int *argc, char ***argv);
static void usage(FILE *stream);
static void globalstate_init(struct globalstate *global);
static bool globalstate_setup(struct globalstate *global);
static void globalstate_teardown(struct globalstate *global);
static void setup_audio(lua_State *L);
static void teardown_audio(void);
static void setup_images(lua_State *L);
static void teardown_images(void);
static void setup_luafuncs(lua_State *L);
static void add_joymapfile(struct globalstate *global);
static bool read_script(struct globalstate *global);
static void event_loop(void);
static void handle_joybutton(const SDL_JoyButtonEvent ev);
static void handle_joyaxis(const SDL_JoyAxisEvent ev);
static bool handle_keyboard(const SDL_KeyboardEvent ev);

static int lua_playclip(lua_State *L); 
static int lua_loopclip(lua_State *L);
static int lua_stopclip(lua_State *L);
static int lua_displayimg(lua_State *L);

/* */

static struct globalstate global;

/* */

int
main(int argc, char *argv[])
{
	int rv;

	globalstate_init(&global);
	if ((rv = parse_options(&argc, &argv)) >= 0) return rv;

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK);
	Mix_Init(MIX_INIT_FLAC|MIX_INIT_OGG|MIX_INIT_MP3);
	IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF);

	if (!globalstate_setup(&global))
		goto fail;

	if (strncmp(global.joymap_file, "", 1))
		add_joymapfile(&global);

	setup_luafuncs(global.L);

	if (!read_script(&global)) {
		warnx("%s", lua_tostring(global.L, -1));
		goto fail;
	}

	setup_audio(global.L);
	setup_images(global.L);
	event_loop();

	globalstate_teardown(&global);
	teardown_images();
	teardown_audio();
	IMG_Quit();
	Mix_Quit();
	SDL_Quit();
	return EXIT_SUCCESS;

fail:
	globalstate_teardown(&global);
	teardown_audio();
	IMG_Quit();
	Mix_Quit();
	SDL_Quit();
	return EXIT_FAILURE;
}


static int
parse_options(int *argc, char ***argv)
{
	int ch;

	while ((ch = getopt(*argc, *argv, "hm:v")) != -1) {
		switch (ch) {
		case 'h':
			usage(stdout);
			return EXIT_SUCCESS;
			break;
		case 'm':
			snprintf(global.joymap_file, PATH_MAX, "%s", optarg);
			break;
		case 'v':
			global.verbose = true;
			break;
		case '?': /* FALLTHROUGH */
		default:
			usage(stderr);
			return EXIT_FAILURE;
		}
	}
	(*argc) -= optind;
	(*argv) += optind;

	if ((*argc) < 1) {
		usage(stderr);
		return EXIT_FAILURE;
	}

	snprintf(global.script_file, PATH_MAX, "%s", (*argv)[0]);
	return -1;
}


static void
usage(FILE *stream)
{
	fprintf(stream, "%s: usage: %s [-hv] [-m mapfile] script\n",
		getprogname(), getprogname());
}


/*
 * Basically sets everything to NULL. We will really initialize everything
 * once we parse command line options OK.
 */
static void
globalstate_init(struct globalstate *global)
{
	global->win = NULL;
	global->joy = NULL;
	global->L = NULL;
	snprintf(global->script_file, 1, "%s", "");
	snprintf(global->joymap_file, 1, "%s", "");
	global->verbose = false;
}


static bool
globalstate_setup(struct globalstate *global)
{
	if (SDL_NumJoysticks() >= 1) {
		global->joy = SDL_JoystickOpen(0);
	} else {
		warnx("%s", "can't find any joysticks!");
		return false;
	}

	global->win = SDL_CreateWindow(getprogname(), SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, 150, 150, 0);
	global->L = luaL_newstate();
	luaL_openlibs(global->L);

	return true;
}


static void
globalstate_teardown(struct globalstate *global)
{
	if (global->win) SDL_DestroyWindow(global->win);
	if (global->joy) SDL_JoystickClose(global->joy);
	if (global->L) lua_close(global->L);
}


static void
setup_audio(lua_State *L)
{
	Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 2);

	/* Load in all the samples! */
	int i;
	char *file;

	lua_getglobal(L, "sample_map");
	lua_pushnil(L);

	while (lua_next(L, -2) != 0) {
		i = lua_tointeger(L, -2);

		if ((i < 0) || (i >= MAXCHUNKS)) {
			if (global.verbose)
				warnx("refusing to load sample %s\tto slot %d", file, i);
			lua_pop(L, 1);
			continue;
		}

		file = (char *)lua_tostring(L, -1);
		if (global.verbose) warnx("Loading sample %s\tto slot %d", file, i);
		global.clips[i] = Mix_LoadWAV(file);
		lua_pop(L, 1);
	}
}


static void
teardown_audio(void)
{
	int i;
	for (i = 0; i < MAXCHUNKS; i++) Mix_FreeChunk(global.clips[i]);
	Mix_CloseAudio();
}


static void
setup_images(lua_State *L)
{
	int i;
	char *file;

	lua_getglobal(L, "image_map");
	lua_pushnil(L);

	while (lua_next(L, -2) != 0) {
		i = lua_tointeger(L, -2);

		if ((i < 0) || (i >= MAXIMAGES)) {
			if (global.verbose)
				warnx("refusing to load image %s\tto slot %d", file, i);
			lua_pop(L, 1);
			continue;
		}

		file = (char *)lua_tostring(L, -1);
		if (global.verbose) warnx("Loading image %s\tto slot %d", file, i);
		global.images[i] = IMG_Load(file);
		lua_pop(L, 1);
	}
}


static void
teardown_images(void)
{
	int i;
	for (i = 0; i < MAXIMAGES; i++) SDL_FreeSurface(global.images[i]);
}


static void
setup_luafuncs(lua_State *L)
{
	lua_register(L, "play", lua_playclip);
	lua_register(L, "loop", lua_loopclip);
	lua_register(L, "stop", lua_stopclip);
	lua_register(L, "display", lua_displayimg);
}


static void
add_joymapfile(struct globalstate *global)
{
	int n;
	n = SDL_GameControllerAddMappingsFromFile(global->joymap_file);

	if (n >= 0)
		warnx("added %d gamepad mapping(s) from %s", n, global->joymap_file);
	else
		warnx("could not read mapping file %s", global->joymap_file);
}


/*
 * Returns true if successful, false otherwise.
 */
static bool
read_script(struct globalstate *global)
{
	return luaL_dofile(global->L, global->script_file) ? false : true;
}


static void
event_loop(void)
{
	SDL_Event ev;
	bool done = false;

	while (!done) {
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_KEYDOWN:
				if (!handle_keyboard(ev.key))
					done = true;
				break;
			case SDL_JOYBUTTONDOWN: /* FALLTHROUGH */
			case SDL_JOYBUTTONUP:
				handle_joybutton(ev.jbutton);
				break;
			case SDL_JOYAXISMOTION:
				handle_joyaxis(ev.jaxis);
				break;
			default:
				continue;
			}
		}
	}
}


/*
 * Calls the Lua function onbuttondown(table) or onbuttonup(table) where the
 * table describes the joystick button event looks like this:
 *
 *     {
 *       timestamp = int,
 *       which = int,
 *       button = int,
 *     }
 */
static void
handle_joybutton(const SDL_JoyButtonEvent ev)
{
	if (ev.type == SDL_JOYBUTTONDOWN) {
		lua_getglobal(global.L, "onbuttondown");
	} else {
		lua_getglobal(global.L, "onbuttonup");
	}

	lua_createtable(global.L, 3, 0);
	lua_pushinteger(global.L, ev.timestamp);
	lua_setfield(global.L, -2, "timestamp");
	lua_pushinteger(global.L, ev.which);
	lua_setfield(global.L, -2, "which");
	lua_pushinteger(global.L, ev.button);
	lua_setfield(global.L, -2, "button");

	if (lua_pcall(global.L, 1, 0, 0) != LUA_OK) 
		warnx("%s", lua_tostring(global.L, -1));
}


/*
 * Calls the Lua function onaxis(table) where the table describing the axis
 * event looks like this:
 *
 *     {
 *       timestamp = int,
 *       which = int,
 *       axis = int,
 *       value = int,
 *     }
 */
static void
handle_joyaxis(const SDL_JoyAxisEvent ev)
{
	lua_getglobal(global.L, "onaxis");
	lua_createtable(global.L, 4, 0);
	lua_pushinteger(global.L, ev.timestamp);
	lua_setfield(global.L, -2, "timestamp");
	lua_pushinteger(global.L, ev.which);
	lua_setfield(global.L, -2, "which");
	lua_pushinteger(global.L, ev.axis);
	lua_setfield(global.L, -2, "axis");
	lua_pushinteger(global.L, ev.value);
	lua_setfield(global.L, -2, "value");

	if (lua_pcall(global.L, 1, 0, 0) != LUA_OK) 
		warnx("%s", lua_tostring(global.L, -1));
}


/*
 * Returning 'false' means it's time to quit the event loop.
 */
static bool
handle_keyboard(const SDL_KeyboardEvent ev)
{
	switch (ev.keysym.sym) {
	case SDLK_ESCAPE: /* FALLTHROUGH */
	case SDLK_q:
		return false;
		break;
	}
	return true;
}

/* */

/*
 * play(int)
 */
static int
lua_playclip(lua_State *L)
{
	int i;

	i = luaL_checkinteger(L, 1);
	lua_pop(L, 1);
	Mix_PlayChannel(i, global.clips[i], 0);
	return 0;
}


/*
 * loop(int)
 */
static int
lua_loopclip(lua_State *L)
{
	int i;

	i = luaL_checkinteger(L, 1);
	lua_pop(L, 1);
	Mix_PlayChannel(i, global.clips[i], -1);
	return 0;
}


/*
 * stop(int)
 */
static int
lua_stopclip(lua_State *L)
{
	int i;

	i = luaL_checkinteger(L, 1);
	lua_pop(L, 1);
	Mix_HaltChannel(i);
	return 0;
}


/*
 * display(int)
 */
static int
lua_displayimg(lua_State *L)
{
	int i;

	i = luaL_checkinteger(L, 1);
	lua_pop(L, 1);
	SDL_BlitSurface(global.images[i], NULL, SDL_GetWindowSurface(global.win), NULL);
	SDL_UpdateWindowSurface(global.win);
	return 0;
}
