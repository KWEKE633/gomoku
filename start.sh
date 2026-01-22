#!/bin/bash
set -e

export DISPLAY=:1

Xvfb :1 -screen 0 1024x768x24 &
sleep 2

fluxbox &
x11vnc -display :1 -nopw -forever -listen localhost &
/opt/noVNC-1.3.0/utils/novnc_proxy --vnc localhost:5900 --listen 8080
