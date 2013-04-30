Freeciv web proxy

Start the Freeciv HTML5 WebSocket proxy like this:
sudo python freeciv-websocket-proxy.py

See status on this URL when proxy is running: 
http://localhost:8082/status

The proxy requires Python 2.7.

The HTML5 Websocket server requires:
- SocketTornad.IO 0.0.4 - https://github.com/MrJoes/tornadio
- Tornado 1.2.1 - http://www.tornadoweb.org/

---------------------------------------------------
Notes:
Run sync.sh to generate packhand_gen.js
