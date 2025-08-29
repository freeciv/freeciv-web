#!/usr/bin/env bash

/docker/scripts/start-freeciv-web.sh

# Keep services running - create a service monitor loop
echo "Starting service monitor to keep all services running..."

while true; do
    # Check if Tomcat is running
    if ! curl --output /dev/null --silent --head --fail "http://localhost:8080/"; then
        echo "Tomcat appears to be down, restarting..."
        sudo $CATALINA_HOME/bin/catalina.sh start || sudo service tomcat10 start
        sleep 10
    fi
    
    # Check if nginx is running
    if ! pidof nginx > /dev/null; then
        echo "Nginx appears to be down, restarting..."
        sudo service nginx start
        sleep 5
    fi
    
    # Check if publite2 is running (game server manager)
    if ! pgrep -f "publite2" > /dev/null; then
        echo "Publite2 appears to be down, restarting..."
        cd /docker/publite2 && sh run.sh &
        sleep 5
    fi
    
    # Wait before next check
    sleep 30
done
