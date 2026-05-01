#!/usr/bin/env python3
# Serve the emscripten build with the cross-origin-isolation headers
# required by -pthread / -sPROXY_POSIX_SOCKETS (SharedArrayBuffer).

import http.server
import os
import socketserver
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8000


class Handler(http.server.SimpleHTTPRequestHandler):
	def end_headers(self):
		self.send_header("Cross-Origin-Opener-Policy", "same-origin")
		self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
		self.send_header("Cache-Control", "no-store")
		super().end_headers()


os.chdir(os.path.dirname(os.path.abspath(__file__)))
with socketserver.TCPServer(("", PORT), Handler) as httpd:
	print(f"Serving http://127.0.0.1:{PORT}/ with COOP/COEP")
	httpd.serve_forever()
