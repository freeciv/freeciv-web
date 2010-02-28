/usr/local/sbin/varnishd -a localhost:80 \
-f /etc/varnish/varnish.vcl \
-s file,/tmp/varnish.cache,1G \
-p thread_pools=6 \
-p thread_pool_max=1500 \
-p listen_depth=2048 \
-p connect_timeout=600 
