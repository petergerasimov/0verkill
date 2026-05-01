#!/usr/bin/env python3
"""WebSocket-to-UDP relay for the 0verkill web client.

Replaces emscripten's websocket_to_posix_proxy. The wasm client (built
with -lwebsocket.js, no PROXY_POSIX_SOCKETS) opens a WebSocket here and
sends UDP packets as raw binary frames. We open a UDP socket per WS
connection and shuttle bytes between them.

    browser  --(WS, this script)-->  relay  --(UDP)-->  ./server

Run alongside `./server` and `python3 web/serve.py`:

    python3 web/ws_udp_relay.py                      # :8002 -> 127.0.0.1:6666
    python3 web/ws_udp_relay.py 8002 127.0.0.1 6666

Requires the `websockets` package:  pip install websockets
"""
import asyncio
import sys

try:
    import websockets
except ImportError:
    sys.stderr.write(
        "error: install the `websockets` package "
        "(pip install websockets, or your distro's python3-websockets)\n"
    )
    sys.exit(1)


def parse_args(argv):
    port = 8002
    host = "127.0.0.1"
    udp_port = 6666
    if len(argv) > 1:
        port = int(argv[1])
    if len(argv) > 2:
        host = argv[2]
    if len(argv) > 3:
        udp_port = int(argv[3])
    return port, host, udp_port


class UDPProto(asyncio.DatagramProtocol):
    def __init__(self, ws):
        self.ws = ws
        self.loop = asyncio.get_running_loop()

    def datagram_received(self, data, addr):
        self.loop.create_task(self._send(data))

    async def _send(self, data):
        try:
            await self.ws.send(data)
        except websockets.ConnectionClosed:
            pass


def make_handler(target_host, target_port):
    async def handler(ws):
        loop = asyncio.get_running_loop()
        target = (target_host, target_port)
        transport, _ = await loop.create_datagram_endpoint(
            lambda: UDPProto(ws),
            local_addr=("0.0.0.0", 0),
        )
        try:
            async for msg in ws:
                if isinstance(msg, (bytes, bytearray)):
                    transport.sendto(bytes(msg), target)
        except websockets.ConnectionClosed:
            pass
        finally:
            transport.close()
    return handler


async def main():
    port, host, udp_port = parse_args(sys.argv)
    print(
        f"ws://0.0.0.0:{port}/  ->  udp://{host}:{udp_port}",
        flush=True,
    )
    async with websockets.serve(
        make_handler(host, udp_port),
        "0.0.0.0",
        port,
        subprotocols=["binary"],
    ):
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
