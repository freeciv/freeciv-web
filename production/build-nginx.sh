#!/bin/bash
echo "==== Setting up nginx ===="
  sudo rm -f /etc/nginx/sites-enabled/default
  sudo ln -f /etc/nginx/sites-available/freeciv-web /etc/nginx/sites-enabled/freeciv-web