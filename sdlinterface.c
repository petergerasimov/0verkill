/* SDL2 INTERFACE */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "console.h"
#include "cfg.h"
#include "kbd.h"
#include "x.h"

#define DEFAULT_PIXEL_W		1024
#define DEFAULT_PIXEL_H		768

int console_ok = 1;

SDL_Window *sdlwindow;
SDL_Renderer *renderer;
SDL_Texture *backbuff;

#ifdef TRI_D
SDL_Window *sdlwindow2;
SDL_Renderer *renderer2;
SDL_Texture *backbuff2;
Uint32 sdlwindow2_id;
#endif

Uint32 sdlwindow_id;

TTF_Font *font;

int FONT_X_SIZE, FONT_Y_SIZE;
char *x_font_name;

static const SDL_Color sdl_color[16] = {
	{   0,   0,   0, 255 },	/* black     */
	{ 139,   0,   0, 255 },	/* red4      */
	{   0, 139,   0, 255 },	/* green4    */
	{ 139,  69,  19, 255 },	/* brown4    */
	{   0,   0, 139, 255 },	/* blue4     */
	{ 138,  43, 226, 255 },	/* dark vio. */
	{   0, 139, 139, 255 },	/* cyan4     */
	{ 102, 102, 102, 255 },	/* gray40    */
	{  51,  51,  51, 255 },	/* gray20    */
	{ 255,   0,   0, 255 },	/* red1      */
	{   0, 255,   0, 255 },	/* green1    */
	{ 255, 255,   0, 255 },	/* yellow    */
	{   0,   0, 255, 255 },	/* blue1     */
	{ 255,   0, 255, 255 },	/* magenta1  */
	{   0, 255, 255, 255 },	/* cyan1     */
	{ 255, 255, 255, 255 }	/* white     */
};

int sdl_current_x, sdl_current_y;
int sdl_current_color;
int sdl_current_bgcolor;
int sdl_width = DEFAULT_X_SIZE, sdl_height = DEFAULT_Y_SIZE;
int display_width, display_height;

/*
 * Pick the active backbuffer / renderer based on the global tri_d
 * flag. When 3D is off there is only ever the primary one.
 */
static SDL_Renderer *active_renderer(void)
{
#ifdef TRI_D
	if (TRI_D_ON && tri_d)
		return renderer2;
#endif
	return renderer;
}

static SDL_Texture *active_backbuff(void)
{
#ifdef TRI_D
	if (TRI_D_ON && tri_d)
		return backbuff2;
#endif
	return backbuff;
}

static void create_backbuffer(SDL_Renderer *r, SDL_Texture **tex)
{
	if (*tex)
		SDL_DestroyTexture(*tex);
	*tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888,
				 SDL_TEXTUREACCESS_TARGET,
				 FONT_X_SIZE * sdl_width,
				 FONT_Y_SIZE * (sdl_height + 1));
	if (!*tex)
		return;
	SDL_SetRenderTarget(r, *tex);
	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	SDL_SetRenderTarget(r, NULL);
}

/*
 * Recompute the character dimensions after the user resized one of
 * the windows, then rebuild the backbuffer textures so subsequent
 * draw calls land in a buffer of the right size. Called from the
 * keyboard event pump in sdlkbd.c.
 */
void sdl_handle_resize(Uint32 window_id, int new_pixel_w, int new_pixel_h)
{
	int new_w, new_h;

	if (FONT_X_SIZE <= 0 || FONT_Y_SIZE <= 0)
		return;

	new_w = new_pixel_w / FONT_X_SIZE;
	new_h = new_pixel_h / FONT_Y_SIZE;
	if (new_w < X_MIN_WIDTH)
		new_w = X_MIN_WIDTH;
	if (new_h < X_MIN_HEIGHT)
		new_h = X_MIN_HEIGHT;
	if (new_h > 0)
		new_h--; /* compensate for the +1 row reserved at the top */

	if (new_w == sdl_width && new_h == sdl_height)
		return;

	sdl_width = new_w;
	sdl_height = new_h;

	create_backbuffer(renderer, &backbuff);
#ifdef TRI_D
	if (TRI_D_ON)
		create_backbuffer(renderer2, &backbuff2);
#endif

	/* Keep both windows in sync when only one was resized. */
#ifdef TRI_D
	if (TRI_D_ON) {
		int pw = sdl_width * FONT_X_SIZE;
		int ph = (sdl_height + 1) * FONT_Y_SIZE;
		if (window_id != sdlwindow_id)
			SDL_SetWindowSize(sdlwindow, pw, ph);
		if (window_id != sdlwindow2_id)
			SDL_SetWindowSize(sdlwindow2, pw, ph);
	}
#else
	(void)window_id;
#endif
}

void c_refresh(void)
{
	SDL_SetRenderTarget(renderer, NULL);
	SDL_RenderCopy(renderer, backbuff, NULL, NULL);
	SDL_RenderPresent(renderer);

#ifdef TRI_D
	if (TRI_D_ON) {
		SDL_SetRenderTarget(renderer2, NULL);
		SDL_RenderCopy(renderer2, backbuff2, NULL, NULL);
		SDL_RenderPresent(renderer2);
	}
#endif
}

/* initialize console */
void c_init(int w, int h)
{
	const char *fontname = x_font_name ? x_font_name : DEFAULT_FONT_NAME;
	SDL_DisplayMode mode;
	int minX, maxX, minY, maxY, advance;
	int font_pt;
	int win_w, win_h;

	(void)w;
	(void)h;

	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	if (TTF_Init() == -1) {
		fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
		SDL_Quit();
		exit(1);
	}

	font_pt = DEFAULT_PIXEL_H / sdl_height;
	font = TTF_OpenFont(fontname, font_pt);
	if (!font) {
		fprintf(stderr, "TTF_OpenFont(%s): %s\n",
			fontname, TTF_GetError());
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	TTF_SetFontKerning(font, 0);

	TTF_GlyphMetrics(font, 'A', &minX, &maxX, &minY, &maxY, &advance);
	FONT_X_SIZE = advance > 0 ? advance : (maxX - minX);
	FONT_Y_SIZE = TTF_FontHeight(font);
	if (FONT_X_SIZE <= 0)
		FONT_X_SIZE = 1;
	if (FONT_Y_SIZE <= 0)
		FONT_Y_SIZE = 1;

	win_w = FONT_X_SIZE * sdl_width;
	win_h = FONT_Y_SIZE * (sdl_height + 1);

	sdlwindow = SDL_CreateWindow("0verkill",
				     SDL_WINDOWPOS_UNDEFINED,
				     SDL_WINDOWPOS_UNDEFINED,
				     win_w, win_h,
				     SDL_WINDOW_SHOWN |
				     SDL_WINDOW_RESIZABLE);
	if (!sdlwindow) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		TTF_CloseFont(font);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	sdlwindow_id = SDL_GetWindowID(sdlwindow);

	renderer = SDL_CreateRenderer(sdlwindow, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(sdlwindow);
		TTF_CloseFont(font);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

#ifdef TRI_D
	if (TRI_D_ON) {
		sdlwindow2 = SDL_CreateWindow("0verkill 3D",
					      SDL_WINDOWPOS_UNDEFINED,
					      SDL_WINDOWPOS_UNDEFINED,
					      win_w, win_h,
					      SDL_WINDOW_SHOWN |
					      SDL_WINDOW_RESIZABLE);
		if (!sdlwindow2) {
			fprintf(stderr, "SDL_CreateWindow(3D): %s\n",
				SDL_GetError());
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(sdlwindow);
			TTF_CloseFont(font);
			TTF_Quit();
			SDL_Quit();
			exit(1);
		}
		sdlwindow2_id = SDL_GetWindowID(sdlwindow2);

		renderer2 = SDL_CreateRenderer(sdlwindow2, -1,
					       SDL_RENDERER_ACCELERATED);
		if (!renderer2) {
			fprintf(stderr, "SDL_CreateRenderer(3D): %s\n",
				SDL_GetError());
			SDL_DestroyWindow(sdlwindow2);
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(sdlwindow);
			TTF_CloseFont(font);
			TTF_Quit();
			SDL_Quit();
			exit(1);
		}
	}
#endif

	if (SDL_GetCurrentDisplayMode(0, &mode) == 0) {
		display_width = mode.w;
		display_height = mode.h;
	} else {
		display_width = win_w;
		display_height = win_h;
	}

	create_backbuffer(renderer, &backbuff);
#ifdef TRI_D
	if (TRI_D_ON)
		create_backbuffer(renderer2, &backbuff2);
#endif

	kbd_init();
	console_ok = 0;
}

/* close console */
void c_shutdown(void)
{
	kbd_close();

	if (backbuff) {
		SDL_DestroyTexture(backbuff);
		backbuff = NULL;
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if (sdlwindow) {
		SDL_DestroyWindow(sdlwindow);
		sdlwindow = NULL;
	}

#ifdef TRI_D
	if (backbuff2) {
		SDL_DestroyTexture(backbuff2);
		backbuff2 = NULL;
	}
	if (renderer2) {
		SDL_DestroyRenderer(renderer2);
		renderer2 = NULL;
	}
	if (sdlwindow2) {
		SDL_DestroyWindow(sdlwindow2);
		sdlwindow2 = NULL;
	}
#endif

	if (font) {
		TTF_CloseFont(font);
		font = NULL;
	}
	TTF_Quit();
	SDL_Quit();

	console_ok = 1;
}

/* move cursor to [x,y] */
void c_goto(int x, int y)
{
	sdl_current_x = x;
	sdl_current_y = y;
}

/* set foreground color */
void c_setcolor(unsigned char a)
{
	sdl_current_color = a & 15;
}

/* set foreground and background color */
void c_setcolor_bg(unsigned char a, unsigned char b)
{
	sdl_current_color = a & 15;
	sdl_current_bgcolor = b & 7;
}

/* set background color */
void c_setbgcolor(unsigned char a)
{
	sdl_current_bgcolor = a & 7;
}

/* set highlight color and background */
void c_sethlt_bg(unsigned char a, unsigned char b)
{
	sdl_current_color = (sdl_current_color & 7) | ((!!a) << 3);
	sdl_current_bgcolor = b & 7;
}

/* set highlight color */
void c_sethlt(unsigned char a)
{
	sdl_current_color = (sdl_current_color & 7) | ((!!a) << 3);
}

/* set 3 bit foreground color and background color */
void c_setcolor_3b_bg(unsigned char a, unsigned char b)
{
	sdl_current_color = (sdl_current_color & 8) | (a & 7);
	sdl_current_bgcolor = b & 7;
}

/* set 3 bit foreground color */
void c_setcolor_3b(unsigned char a)
{
	sdl_current_color = (sdl_current_color & 8) | (a & 7);
}

/* print on the cursor position */
#ifdef HAVE_INLINE
inline
#endif
void c_print_l(char *text, int l)
{
	SDL_Renderer *r = active_renderer();
	SDL_Texture *target = active_backbuff();
	SDL_Color fg = sdl_color[sdl_current_color];
	SDL_Color bg = sdl_color[sdl_current_bgcolor];
	SDL_Surface *surface;
	SDL_Texture *tex;
	SDL_Rect dst;
	char *buf;

	if (!text || l <= 0 || !target)
		return;

	buf = alloca(l + 1);
	memcpy(buf, text, l);
	buf[l] = '\0';

	SDL_SetRenderTarget(r, target);

	dst.x = sdl_current_x * FONT_X_SIZE;
	dst.y = (sdl_current_y + 1) * FONT_Y_SIZE;
	dst.w = l * FONT_X_SIZE;
	dst.h = FONT_Y_SIZE;

	SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
	SDL_RenderFillRect(r, &dst);

	surface = TTF_RenderText_Solid(font, buf, fg);
	if (!surface) {
		SDL_SetRenderTarget(r, NULL);
		sdl_current_x += l;
		return;
	}
	tex = SDL_CreateTextureFromSurface(r, surface);
	if (tex) {
		SDL_Rect text_dst = dst;
		text_dst.w = surface->w;
		text_dst.h = surface->h;
		SDL_RenderCopy(r, tex, NULL, &text_dst);
		SDL_DestroyTexture(tex);
	}
	SDL_FreeSurface(surface);

	SDL_SetRenderTarget(r, NULL);
	sdl_current_x += l;
}

#ifdef HAVE_INLINE
inline
#endif
void c_print(char *text)
{
	c_print_l(text, strlen(text));
}

/* print char on the cursor position */
void c_putc(char c)
{
	char s[2] = { c, 0 };

	c_print(s);
}

static void clear_target(SDL_Renderer *r, SDL_Texture *target)
{
	if (!target)
		return;
	SDL_SetRenderTarget(r, target);
	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
	SDL_RenderClear(r);
	SDL_SetRenderTarget(r, NULL);
}

/* clear the screen */
void c_cls(void)
{
	clear_target(renderer, backbuff);
#ifdef TRI_D
	if (TRI_D_ON)
		clear_target(renderer2, backbuff2);
#endif
}

/*
 * Clear a character rectangle on the screen.
 * Preconditions: x2 >= x1 && y2 >= y1.
 */
void c_clear(int x1, int y1, int x2, int y2)
{
	int w = x2 - x1 + 1;
	int h = y2 - y1 + 1;
	SDL_Rect rect;

	rect.x = x1 * FONT_X_SIZE;
	rect.y = (y1 + 1) * FONT_Y_SIZE;
	rect.w = w * FONT_X_SIZE;
	rect.h = h * FONT_Y_SIZE;

	if (backbuff) {
		SDL_SetRenderTarget(renderer, backbuff);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &rect);
		SDL_SetRenderTarget(renderer, NULL);
	}
#ifdef TRI_D
	if (TRI_D_ON && backbuff2) {
		SDL_SetRenderTarget(renderer2, backbuff2);
		SDL_SetRenderDrawColor(renderer2, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer2, &rect);
		SDL_SetRenderTarget(renderer2, NULL);
	}
#endif
}

void c_update_kbd(void)
{
	kbd_update();
}

int c_pressed(int k)
{
	return kbd_is_pressed(k);
}

int c_was_pressed(int k)
{
	return kbd_was_pressed(k);
}

void c_wait_for_key(void)
{
	kbd_wait_for_key();
}

/* set cursor shape */
void c_cursor(int type)
{
	(void)type;
}

/* ring the bell */
void c_bell(void)
{
}

/* get screen dimensions */
void c_get_size(int *x, int *y)
{
	*x = sdl_width;
	*y = sdl_height;
}
