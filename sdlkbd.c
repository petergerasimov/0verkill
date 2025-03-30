/* portable X keyboard patch by Peter Berg Larsen <pebl@diku.dk> */

#include <signal.h>
#include <string.h>

#include "x.h"
#include "kbd.h"
#include "console.h"
#include "cfg.h"
#include "time.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_keycode.h>


static unsigned char keyboard[128]; /* 0=not pressed, !0=pressed */
static unsigned char old_keyboard[128]; /* 0=not pressed, !0=pressed */

extern int sdl_width, sdl_height;


void kbd_init(void)
{
	/* nothing to initialize */
}


void kbd_close(void)
{
	/* nothing to close */
}


/* convert key from scancode to key constant */
int remap_out(int k)
{
	int remap_table[128]={
	0,K_ESCAPE,'1','2','3','4','5','6','7','8','9','0','-','=',K_BACKSPACE,K_TAB,
	'q','w','e','r','t','y','u','i','o','p','[',']',K_ENTER,K_LEFT_CTRL,'a','s',
	'd','f','g','h','j','k','l',';','\'','`',K_LEFT_SHIFT,'\\','z','x','c','v',
	'b','n','m',',','.','/',K_RIGHT_SHIFT,K_NUM_ASTERISK,K_LEFT_ALT,' ',K_CAPS_LOCK,K_F1,K_F2,K_F3,K_F4,K_F5,
	K_F6,K_F7,K_F8,K_F9,K_F10,K_NUM_LOCK,K_SCROLL_LOCK,K_NUM7,K_NUM8,K_NUM9,K_NUM_MINUS,K_NUM4,K_NUM5,K_NUM6,K_NUM_PLUS,K_NUM1,
	K_NUM2,K_NUM3,K_NUM0,K_NUM_DOT,0,0,0,K_F11,K_F12,K_HOME,K_UP,K_PGUP,K_LEFT,0,K_RIGHT,K_END,
	K_DOWN,K_PGDOWN,K_INSERT,K_DELETE,K_NUM_ENTER,K_RIGHT_CTRL,K_PAUSE,K_SYSRQ,K_NUM_SLASH,K_RIGHT_ALT,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	};
	
	return remap_table[k&127];
}

int remap_in(int k)
{
	switch (k)
	{
		case K_ESCAPE: return 1;
		case '1': return 2;
		case '2': return 3;
		case '3': return 4;
		case '4': return 5;
		case '5': return 6;
		case '6': return 7;
		case '7': return 8;
		case '8': return 9;
		case '9': return 10;
		case '0': return 11;
		case '-': return 12;
		case '=': return 13;
		case K_BACKSPACE: return 14;
		case K_TAB: return 15;
		case 'q': return 16;
		case 'w': return 17;
		case 'e': return 18;
		case 'r': return 19;
		case 't': return 20;
		case 'y': return 21;
		case 'u': return 22;
		case 'i': return 23;
		case 'o': return 24;
		case 'p': return 25;
		case '[': return 26;
		case ']': return 27;
		case K_ENTER: return 28;
		case K_LEFT_CTRL: return 29;
		case 'a': return 30;
		case 's': return 31;
		case 'd': return 32;
		case 'f': return 33;
		case 'g': return 34;
		case 'h': return 35;
		case 'j': return 36;
		case 'k': return 37;
		case 'l': return 38;
		case ';': return 39;
		case '\'': return 40;
		case '`': return 41;
		case K_LEFT_SHIFT: return 42;
		case '\\': return 43;
		case 'z': return 44;
		case 'x': return 45;
		case 'c': return 46;
		case 'v': return 47;
		case 'b': return 48;
		case 'n': return 49;
		case 'm': return 50;
		case ',': return 51;
		case '.': return 52;
		case '/': return 53;
		case K_RIGHT_SHIFT: return 54;
		case K_NUM_ASTERISK: return 55;
		case K_LEFT_ALT: return 56;
		case ' ': return 57;
		case K_CAPS_LOCK: return 58;
		case K_F1: return 59;
		case K_F2: return 60;
		case K_F3: return 61;
		case K_F4: return 62;
		case K_F5: return 63;
		case K_F6: return 64;
		case K_F7: return 65;
		case K_F8: return 66;
		case K_F9: return 67;
		case K_F10: return 68;
		case K_NUM_LOCK: return 69;
		case K_SCROLL_LOCK: return 70;
		case K_NUM7: return 71;
		case K_NUM8: return 72;
		case K_NUM9: return 73;
		case K_NUM_MINUS: return 74;
		case K_NUM4: return 75;
		case K_NUM5: return 76;
		case K_NUM6: return 77;
		case K_NUM_PLUS: return 78;
		case K_NUM1: return 79;
		case K_NUM2: return 80;
		case K_NUM3: return 81;
		case K_NUM0: return 82;
		case K_NUM_DOT: return 83;
		case K_F11: return 87;
		case K_F12: return 88;
		case K_NUM_ENTER: return 100;
		case K_RIGHT_CTRL: return 101;
		case K_NUM_SLASH: return 104;
		case K_SYSRQ: return 103;
		case K_RIGHT_ALT: return 105;
		case K_HOME: return 89;
		case K_UP: return 90;
		case K_PGUP: return 91;
		case K_LEFT: return 92;
		case K_RIGHT: return 94;
		case K_END: return 95;
		case K_DOWN: return 96;
		case K_PGDOWN: return 97;
		case K_INSERT: return 98;
		case K_DELETE: return 99;
		case K_PAUSE: return 102;
		default: return 0;
	}
	
}

/* convert key constant to scancode */
int sdltocode(int ks)
{
	switch (ks)
	{
		case SDLK_ESCAPE:      return 1;
		case SDLK_1:           return 2;
		case SDLK_EXCLAIM:     return 2;
		case SDLK_2:           return 3;
		case SDLK_AT:          return 3;
		case SDLK_3:           return 4;
		case SDLK_HASH:	       return 4;
		case SDLK_4:           return 5;
		case SDLK_DOLLAR:      return 5;
		case SDLK_5:           return 6;
		case SDLK_PERCENT:     return 6;
		case SDLK_6:           return 7;
		case SDLK_CARET:       return 7;
		case SDLK_7:           return 8;
		case SDLK_AMPERSAND:   return 8;
		case SDLK_8:           return 9;
		case SDLK_ASTERISK:    return 9;
		case SDLK_9:           return 10;
		case SDLK_LEFTPAREN:   return 10;
		case SDLK_0:           return 11;
		case SDLK_RIGHTPAREN:  return 11;
		case SDLK_MINUS:       return 12;
		case SDLK_UNDERSCORE: return 12;
		case SDLK_EQUALS:      return 13;
		case SDLK_PLUS:        return 13;
		case SDLK_BACKSPACE:   return 14;
		case SDLK_TAB:         return 15;
		case SDLK_q:           return 16;
		case SDLK_w:           return 17;
		case SDLK_e:           return 18;
		case SDLK_r:           return 19;
		case SDLK_t:           return 20;
		case SDLK_y:           return 21;
		case SDLK_u:           return 22;
		case SDLK_i:           return 23;
		case SDLK_o:           return 24;
		case SDLK_p:           return 25;
		case SDLK_LEFTBRACKET: return 26;
		case SDLK_RIGHTBRACKET:return 27;
		case SDLK_RETURN:      return 28;
		case SDLK_LCTRL:       return 29;
		case SDLK_a:           return 30;
		case SDLK_s:           return 31;
		case SDLK_d:           return 32;
		case SDLK_f:           return 33;
		case SDLK_g:           return 34;
		case SDLK_h:           return 35;
		case SDLK_j:           return 36;
		case SDLK_k:           return 37;
		case SDLK_l:           return 38;
		case SDLK_SEMICOLON:   return 39;
		case SDLK_COLON:       return 39;
		case SDLK_QUOTE:       return 40;
		case SDLK_QUOTEDBL:    return 40;
		case SDLK_LSHIFT:      return 42;
		case SDLK_BACKSLASH:   return 43;
		case SDLK_z:           return 44;
		case SDLK_x:           return 45;
		case SDLK_c:           return 46;
		case SDLK_v:           return 47;
		case SDLK_b:           return 48;
		case SDLK_n:           return 49;
		case SDLK_m:           return 50;
		case SDLK_COMMA:       return 51;
		case SDLK_LESS:        return 51;
		case SDLK_PERIOD:      return 52;
		case SDLK_GREATER:     return 52;
		case SDLK_SLASH:       return 53;
		case SDLK_QUESTION:    return 53;
		case SDLK_RSHIFT:      return 54;
		case SDLK_KP_MULTIPLY: return 55;
		case SDLK_LALT:        return 56;
		case SDLK_SPACE:       return 57;
		case SDLK_CAPSLOCK:    return 58;
		case SDLK_F1:          return 59;
		case SDLK_F2:          return 60;
		case SDLK_F3:          return 61;
		case SDLK_F4:          return 62;
		case SDLK_F5:          return 63;
		case SDLK_F6:          return 64;
		case SDLK_F7:          return 65;
		case SDLK_F8:          return 66;
		case SDLK_F9:          return 67;
		case SDLK_F10:         return 68;
		case SDLK_SCROLLLOCK:  return 70;
		case SDLK_KP_7:        return 71;
		case SDLK_KP_8:        return 72;
		case SDLK_KP_9:        return 73;
		case SDLK_KP_MINUS:    return 74;
		case SDLK_KP_4:        return 75;
		case SDLK_KP_5:        return 76;
		case SDLK_KP_6:        return 77;
		case SDLK_KP_PLUS:     return 78;
		case SDLK_KP_1:        return 79;
		case SDLK_KP_2:        return 80;
		case SDLK_KP_3:        return 81;
		case SDLK_KP_0:        return 82;
		case SDLK_F11:         return 87;
		case SDLK_F12:         return 88;
		case SDLK_HOME:        return 89;
		case SDLK_UP:          return 90;
		case SDLK_PAGEUP:      return 91;
		case SDLK_LEFT:        return 92;
		case SDLK_RIGHT:       return 94;
		case SDLK_END:         return 95;
		case SDLK_DOWN:        return 96;
		case SDLK_PAGEDOWN:    return 97;
		case SDLK_INSERT:      return 98;
		case SDLK_DELETE:      return 99;
		case SDLK_KP_ENTER:    return 100;
		case SDLK_RCTRL:       return 101;
		case SDLK_PAUSE:       return 102;
		case SDLK_SYSREQ:      return 103;
		case SDLK_KP_DIVIDE:   return 104;
		case SDLK_RALT:        return 105;
		default:               return 0;
	}
}

void sigwinch_handler(int);
void signal_handler(int);

int kbd_update(void)
{
	int retval = 0;
	SDL_Event event;

	memcpy(old_keyboard, keyboard, 128);
	while (SDL_PollEvent(&event)) 
	{
		switch (event.type) 
		{
			case SDL_KEYDOWN: 
				{
					retval = 1;
					SDL_Keycode keycode = sdltocode(event.key.keysym.sym);
					if (keycode > 127) break;
					keyboard[keycode] = 1;
					break;
				}

			case SDL_KEYUP: 
				{
					retval = 1;
					SDL_Keycode keycode = sdltocode(event.key.keysym.sym);
					if (keycode > 127) break;
					keyboard[keycode] = 0;
					break;
				}

			case SDL_WINDOWEVENT:
				switch (event.window.event) 
				{
					case SDL_WINDOWEVENT_CLOSE:
					// Close window event (similar to DestroyNotify in X11)
#ifdef SIGQUIT
					raise(SIGQUIT);
#else
					signal_handler(3);
#endif
					break;

					case SDL_WINDOWEVENT_SIZE_CHANGED:
						// Window resized (similar to ConfigureNotify in X11)
						sdl_width = event.window.data1;  // New width
						sdl_height = event.window.data2; // New height
						break;
				}
				break;

			case SDL_WINDOWEVENT_EXPOSED: 
			// Window exposed (similar to Expose in X11)
#ifdef SIGWINCH
			raise(SIGWINCH);
#else
			sigwinch_handler(-1);
#endif
			break;

			case SDL_QUIT:
				// Handle SDL Quit (close the application)
				return -1;  // Exit the loop and end the program
				break;
		}
	}

	// Check if CTRL+C is pressed (KEYCODE for Ctrl: 29, C: 46 in X11 mapping)
	if ((keyboard[29] || keyboard[97]) && keyboard[46]) 
	{
		raise(SIGINT);
	}

	return retval;
}

/* returns 1 if given key is pressed, 0 otherwise */
int kbd_is_pressed(int key)
{
	int a=remap_in(key);
	return a?keyboard[a]:0;
}


/* same as kbd_is_pressed but tests rising edge of the key */
int kbd_was_pressed(int key)
{
	int a=remap_in(key);
	if (!a)return 0;
	return !old_keyboard[remap_in(key)]&&keyboard[remap_in(key)];
}


void kbd_wait_for_key(void)
{
	SDL_Event event;
	while (1) 
	{
		SDL_WaitEvent(&event);
		if (event.type == SDL_KEYDOWN) 
		{
			SDL_Keycode keycode = sdltocode(event.key.keysym.sym);
			if (keycode > 127) break;
			keyboard[keycode] = 1;
			break;
		}
	}
}
