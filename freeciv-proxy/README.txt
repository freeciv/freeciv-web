FREECIV-PROXY

Freeciv-proxy is a server for HTML5 WebSocket communication,
so that users with HTML5 capable web browsers are able to connect
to the Freeciv C server using WebSockets.

See status on this URL when proxy is running:
http://localhost:7002/status

Freeciv-proxy is started by Publite2.

Software requirements:

- Python 3.5 or later.
- Tornado 6.1 or later - http://www.tornadoweb.org/

Freeciv-proxy can communicate with the Freeciv C server. However, it doesn't support the delta-protocol, so the Freeciv C server must be compiled (maybe even patched) to disable the delta protocol.

Todo:
-add support for delta protocol
