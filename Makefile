# 0verkill — simple Makefile.
#
# Targets:
#   make             default: SDL client + server (recommended, cross-platform)
#   make server      multiplayer server
#   make editor      level editor (terminal)
#   make bot         AI bot
#   make avi         animation player (terminal)
#   make test_server test server tool
#
#   make terminal    terminal version of the game (0verkill-tty)
#   make x11         X11 version of the game (0verkill-x11)
#   make xeditor     X11 level editor
#   make xavi        X11 animation player
#   make sdlavi      SDL animation player
#
#   make web         build for the browser via emscripten
#   make serve-web   run a local server for web/ with COOP/COEP headers
#   make web-proxy   build emscripten's WebSocket->UDP bridge so the
#                    browser build can reach a native ./server. Auto-
#                    detects $EMSDK or distro-packaged emscripten under
#                    /usr/share/emscripten. Override with WS_PROXY_DIR=...
#                    Run as: ./websocket_to_posix_proxy 8002
#   make clean       remove all build artifacts

CC      ?= cc
CFLAGS  ?= -O3 -Wall -Wno-parentheses -Wno-unused-result
LDFLAGS ?=
LIBS     = -lm

SDL_CFLAGS ?= $(shell pkg-config --cflags sdl2 SDL2_ttf 2>/dev/null)
SDL_LIBS   ?= $(shell pkg-config --libs sdl2 SDL2_ttf 2>/dev/null)
ifeq ($(strip $(SDL_LIBS)),)
SDL_LIBS = -lSDL2 -lSDL2_ttf
endif

X_LIBS ?= -lX11

EMCC ?= emcc

# Source files per binary.

GAME_COMMON  = data.c sprite.c blit.c hash.c time.c net.c crc32.c \
               md5.c md5hl.c help.c error.c

CLIENT_SRC   = client.c $(GAME_COMMON)
SERVER_SRC   = server.c data.c sprite.c blit.c console.c kbd.c hash.c time.c \
               net.c crc32.c md5.c md5hl.c error.c
EDITOR_SRC   = editor.c data.c sprite.c blit.c hash.c time.c \
               md5.c md5hl.c error.c
BOT_SRC      = bot.c data.c sprite.c hash.c time.c net.c crc32.c \
               md5.c md5hl.c error.c
AVI_SRC      = avi.c blit.c time.c avihelp.c error.c
TEST_SRV_SRC = test_server.c net.c data.c sprite.c blit.c console.c kbd.c \
               crc32.c time.c hash.c md5.c md5hl.c error.c

CONSOLE_SRC = console.c kbd.c
X_SRC       = xinterface.c xkbd.c
SDL_SRC     = sdlinterface.c sdlkbd.c

WEB_OUT = web/index.html

.PHONY: all default sdl terminal x11 server editor bot avi test_server \
        xeditor xavi sdlavi web serve-web web-proxy clean

default: 0verkill server

all: 0verkill 0verkill-tty 0verkill-x11 server editor xeditor bot avi xavi sdlavi test_server

# --- game client ---

# Default: SDL build, cross-platform.
0verkill: $(CLIENT_SRC) $(SDL_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) -DSDL -DHAVE_LIBSDL2 \
		-o $@ $(CLIENT_SRC) $(SDL_SRC) $(LIBS) $(SDL_LIBS)

sdl: 0verkill

terminal: 0verkill-tty
0verkill-tty: $(CLIENT_SRC) $(CONSOLE_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CLIENT_SRC) $(CONSOLE_SRC) $(LIBS)

x11: 0verkill-x11
0verkill-x11: $(CLIENT_SRC) $(X_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -DXWINDOW -DHAVE_LIBX11 \
		-o $@ $(CLIENT_SRC) $(X_SRC) $(LIBS) $(X_LIBS)

# --- helpers ---

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SERVER_SRC) $(LIBS)

editor: $(EDITOR_SRC) $(CONSOLE_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(EDITOR_SRC) $(CONSOLE_SRC) $(LIBS)

xeditor: $(EDITOR_SRC) $(X_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -DXWINDOW -DHAVE_LIBX11 \
		-o $@ $(EDITOR_SRC) $(X_SRC) $(LIBS) $(X_LIBS)

bot: $(BOT_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(BOT_SRC) $(LIBS)

avi: $(AVI_SRC) $(CONSOLE_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(AVI_SRC) $(CONSOLE_SRC) $(LIBS)

xavi: $(AVI_SRC) $(X_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -DXWINDOW -DHAVE_LIBX11 \
		-o $@ $(AVI_SRC) $(X_SRC) $(LIBS) $(X_LIBS)

sdlavi: $(AVI_SRC) $(SDL_SRC)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(LDFLAGS) -DSDL -DHAVE_LIBSDL2 \
		-o $@ $(AVI_SRC) $(SDL_SRC) $(LIBS) $(SDL_LIBS)

test_server: $(TEST_SRV_SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TEST_SRV_SRC) $(LIBS)

# --- web (emscripten) ---

web: $(WEB_OUT)
$(WEB_OUT): $(CLIENT_SRC) $(SDL_SRC)
	@mkdir -p web
	$(EMCC) $(CLIENT_SRC) $(SDL_SRC) \
		-DSDL -DHAVE_LIBSDL2 \
		-sUSE_SDL=2 -sUSE_SDL_TTF=2 \
		-sASYNCIFY \
		-sASYNCIFY_STACK_SIZE=65536 \
		-sALLOW_MEMORY_GROWTH=1 \
		-O3 \
		-o $@ \
		--preload-file data \
		--preload-file grx \
		--preload-file SpaceMono-Regular.ttf \
		-sWEBSOCKET_URL="ws://127.0.0.1:8002" \
		-sWEBSOCKET_SUBPROTOCOL="binary" \
		-sERROR_ON_UNDEFINED_SYMBOLS=0 \
		-lwebsocket.js

WEB_PORT ?= 8000
serve-web:
	@python3 web/serve.py $(WEB_PORT)

# WebSocket -> UDP bridge from emscripten's source tree. Lets the
# emscripten build (which speaks WS on port 8002 by default) talk to a
# native ./server listening on UDP.
#
# Auto-detect: emsdk layout ($EMSDK/upstream/emscripten/...) first,
# then distro layout (/usr/share/emscripten/...). Override either piece
# with WS_PROXY_DIR=... on the command line.
ifeq ($(strip $(WS_PROXY_DIR)),)
ifneq ($(strip $(EMSDK)),)
WS_PROXY_DIR := $(EMSDK)/upstream/emscripten/tools/websocket_to_posix_proxy
else ifneq ($(wildcard /usr/share/emscripten/tools/websocket_to_posix_proxy),)
WS_PROXY_DIR := /usr/share/emscripten/tools/websocket_to_posix_proxy
endif
endif

# Sources may sit at the top of WS_PROXY_DIR (older layouts) or in src/
# (current upstream and Debian/Ubuntu packaging).
WS_PROXY_CPP := $(wildcard $(WS_PROXY_DIR)/src/*.cpp $(WS_PROXY_DIR)/*.cpp)
WS_PROXY_C   := $(wildcard $(WS_PROXY_DIR)/src/*.c   $(WS_PROXY_DIR)/*.c)

web-proxy: websocket_to_posix_proxy
websocket_to_posix_proxy:
	@if [ -z "$(WS_PROXY_DIR)" ]; then \
		echo >&2 "Could not locate emscripten's websocket_to_posix_proxy."; \
		echo >&2 "Set EMSDK (source emsdk_env.sh) or pass WS_PROXY_DIR=..."; \
		exit 1; \
	fi
	@if [ -z "$(strip $(WS_PROXY_CPP))$(strip $(WS_PROXY_C))" ]; then \
		echo >&2 "No .cpp/.c sources under $(WS_PROXY_DIR) (or its src/)."; \
		echo >&2 "Override WS_PROXY_DIR=... to point at the right tree."; \
		exit 1; \
	fi
	$(CXX) -O2 -pthread -x c++ $(WS_PROXY_CPP) -x c $(WS_PROXY_C) -o $@

clean:
	rm -f 0verkill 0verkill-tty 0verkill-x11 \
	      server editor xeditor bot \
	      avi xavi sdlavi test_server \
	      websocket_to_posix_proxy \
	      *.o core
	rm -f web/index.html web/index.js web/index.wasm web/index.data
