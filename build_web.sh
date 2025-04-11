#!/bin/bash

# Set source files
SRC_FILES="blit.c client.c crc32.c data.c error.c hash.c help.c md5.c md5hl.c net.c sdlkbd.c sdlinterface.c sprite.c time.c"

# Set output file
OUTPUT="web/index.html"

# Compilation command
emcc $SRC_FILES \
    -sUSE_SDL=2 \
    -sUSE_SDL_TTF=2 \
    -sASYNCIFY \
    -O3 \
    -pthread \
    -o $OUTPUT \
    --preload-file data \
    --preload-file grx \
    --preload-file SpaceMono-Regular.ttf \
    -sWEBSOCKET_URL="ws://127.0.0.1:8002" \
    -sWEBSOCKET_SUBPROTOCOL="binary" \
    -sERROR_ON_UNDEFINED_SYMBOLS=0 \
    -lwebsocket.js \
    -sPROXY_POSIX_SOCKETS \
#   -sPROXY_TO_PTHREAD \
#   -sOFFSCREEN_FRAMEBUFFER \
