/* X INTERFACE */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
// #include <SDL2/SDL_mixer.h>

#include "console.h"
#include "cfg.h"
#include "kbd.h"
#include "x.h"

#define BORDER_WIDTH 4


int console_ok=1;

SDL_Window *sdlwindow;
SDL_Renderer *renderer;
TTF_Font* font;
SDL_Event e;

int FONT_X_SIZE,FONT_Y_SIZE;
char *x_font_name;
SDL_Color sdl_color[] = {
	{0, 0, 0, 255},        // "black"
	{139, 0, 0, 255},      // "red4"
	{0, 139, 0, 255},      // "green4"
	{139, 69, 19, 255},    // "brown4"
	{0, 0, 139, 255},      // "blue4"
	{138, 43, 226, 255},   // "dark violet"
	{0, 139, 139, 255},    // "cyan4"
	{102, 102, 102, 255},  // "gray40"
	{51, 51, 51, 255},     // "gray20"
	{255, 0, 0, 255},      // "red1"
	{0, 255, 0, 255},      // "green1"
	{255, 255, 0, 255},    // "yellow"
	{0, 0, 255, 255},      // "blue1"
	{255, 0, 255, 255},    // "magenta1"
	{0, 255, 255, 255},    // "cyan1"
	{255, 255, 255, 255}   // "white"
};
int sdl_screen;
int sdl_current_x,sdl_current_y;
int sdl_current_color=0;
int sdl_current_bgcolor=0;
int sdl_width=DEFAULT_X_SIZE,sdl_height=DEFAULT_Y_SIZE;
int display_height,display_width;

SDL_Texture* backbuff = NULL;

void c_refresh(void)
{
	SDL_SetRenderTarget(renderer, NULL); // Set back to the default target
	SDL_RenderCopy(renderer, backbuff, NULL, NULL);
	SDL_RenderPresent(renderer);
	//SDL_RenderClear(renderer);
}


/* initialize console */
void c_init(int w,int h)
{
	int a;
	char *fontname=x_font_name?x_font_name:DEFAULT_FONT_NAME;

	w=w;h=h;
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		exit(1);	
	}

	if (TTF_Init() == -1) {
		printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
		SDL_Quit();
		exit(1);	
	}

	int font_y = 768 / sdl_height;
	int font_x = (font_y * 2) / 3;

	sdlwindow = SDL_CreateWindow("0verkill", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_SHOWN);
	if (!sdlwindow) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	renderer = SDL_CreateRenderer(sdlwindow, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(sdlwindow);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	font = TTF_OpenFont(fontname, font_y);
	if (font == NULL) {
		printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(sdlwindow);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	TTF_SetFontKerning(font, 0);

	SDL_DisplayMode displayMode;

	if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0) {
		display_height=displayMode.w;
		display_width=displayMode.h;
	} else {
		printf("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(sdlwindow);
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	int minX, maxX, minY, maxY, advance;
	TTF_GlyphMetrics(font, 'A', &minX, &maxX, &minY, &maxY, &advance);

	FONT_X_SIZE = maxX-minX;
	FONT_Y_SIZE = TTF_FontHeight(font);;
	printf("font sz: %d %d\n", FONT_X_SIZE, FONT_Y_SIZE);
	printf("fixed? %d\n", TTF_FontFaceIsFixedWidth(font));
	SDL_SetWindowSize(sdlwindow, 1024, 768);

	backbuff = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, FONT_X_SIZE * sdl_width, FONT_Y_SIZE * (sdl_height + 1));
	SDL_SetRenderTarget(renderer, backbuff);
	SDL_RenderClear(renderer);
	SDL_SetRenderTarget(renderer, NULL);
}


/* close console */
void c_shutdown(void)
{  
	if (backbuff) {
        	SDL_DestroyTexture(backbuff);
        	backbuff = NULL;
    	}
	SDL_DestroyWindow(sdlwindow);
	sdlwindow = NULL;

	SDL_DestroyRenderer(renderer);
	renderer = NULL;

	SDL_Quit();

	kbd_close();
	console_ok=1;
}


/* move cursor to [x,y] */
void c_goto(int x,int y)
{
	sdl_current_x=x;
	sdl_current_y=y;
}


/* set foreground color */
void c_setcolor(unsigned char a)
{
	sdl_current_color=(a&15);
}


/* set foreground and background color */
void c_setcolor_bg(unsigned char a,unsigned char b)
{
	sdl_current_color=(a&15);
	sdl_current_bgcolor=(b&7);
}


/* set background color */
void c_setbgcolor(unsigned char a)
{
	sdl_current_bgcolor=a&7;
}


/* set highlight color and background */
void c_sethlt_bg(unsigned char a,unsigned char b)
{
	sdl_current_color=(sdl_current_color&7)|(!!a)<<3;
	sdl_current_bgcolor=b&7;
}


/* set highlight color */
void c_sethlt(unsigned char a)
{
	sdl_current_color=(sdl_current_color&7)|(!!a)<<3;
}


/* set 3 bit foreground color and background color */
void c_setcolor_3b_bg(unsigned char a,unsigned char b)
{
	sdl_current_color=(sdl_current_color&8)|(a&7);
	sdl_current_bgcolor=b&7;
}


/* set 3 bit foreground color */
void c_setcolor_3b(unsigned char a)
{
	sdl_current_color=(sdl_current_color&8)|(a&7);
}

/* print on the cursor position */
#ifdef HAVE_INLINE
inline
#endif
void c_print_l(char *text, int l)
{
	if (!text || l <= 0) return;
	SDL_SetRenderTarget(renderer, backbuff);
	SDL_Color fgColor = sdl_color[sdl_current_color];
	SDL_Color bgColor = sdl_color[sdl_current_bgcolor];

	// Create a substring limited to l characters
	char buffer[l + 1];
	strncpy(buffer, text, l);
	buffer[l] = '\0';

	SDL_Surface* textSurface = TTF_RenderText_Solid(font, buffer, fgColor);
	if (!textSurface) {
		fprintf(stderr, "Text rendering error: %s\n", TTF_GetError());
		return;
	}

	SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
	if (!textTexture) {
		fprintf(stderr, "Texture creation error: %s\n", SDL_GetError());
		SDL_FreeSurface(textSurface);
		return;
	}

	SDL_Rect dstRect = {sdl_current_x*FONT_X_SIZE, (sdl_current_y + 1)*FONT_Y_SIZE, textSurface->w, textSurface->h};
	SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderFillRect(renderer, &dstRect);
	SDL_RenderCopy(renderer, textTexture, NULL, &dstRect);
	SDL_FreeSurface(textSurface);
	SDL_DestroyTexture(textTexture);

	// SDL_SetRenderTarget(renderer, NULL);

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
	char s[2] = {c, 0};
	c_print(s);
}


/* clear the screen */
void c_cls(void)
{
	SDL_SetRenderTarget(renderer, backbuff);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
}


/* clear rectangle on the screen */
/* presumtions: x2>=x1 && y2>=y1 */
void c_clear(int x1,int y1,int x2,int y2)
{
	int w=x2-x1+1;
	int h=y2-y1+1;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_Rect rect = { x1*FONT_X_SIZE, (y1+1)*FONT_Y_SIZE, w*FONT_X_SIZE, h*FONT_Y_SIZE };

	SDL_RenderFillRect(renderer, &rect);
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
	type=type;
}


/* ring the bell */
void c_bell(void)
{
	// 	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
	// 		printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
	// 		return;
	// 	}
	// 
	// 	// Load a bell sound (you can replace with your own bell sound file)
	// 	Mix_Chunk* bellSound = Mix_LoadWAV("bell_sound.wav");
	// 	if (bellSound == NULL) {
	// 		printf("Failed to load bell sound! SDL_mixer Error: %s\n", Mix_GetError());
	// 		return;
	// 	}
	// 
	// 	// Play the bell sound
	// 	Mix_PlayChannel(-1, bellSound, 0);
	// 
	// 	// Clean up
	// 	Mix_FreeChunk(bellSound);
	// 	Mix_CloseAudio();
}


/* get screen dimensions */
void c_get_size(int *x, int *y)
{
	(*x)=sdl_width;
	(*y)=sdl_height;
}
