sleep 10
cp /vagrant/music/*.* /var/lib/tomcat10/webapps/freeciv-web/music/ || echo "could not install music"
