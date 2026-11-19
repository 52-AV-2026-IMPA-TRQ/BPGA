#!/bin/bash

URL="http://127.0.0.1:8080"

xset s off
xset s noblank
xset -dpms

unclutter -idle 0.5 -root &

until curl -fsS "$URL/api/status" >/dev/null 2>&1; do
    sleep 1
done

while true; do
    chromium-browser \
        --kiosk \
        --noerrdialogs \
        --disable-infobars \
        --disable-session-crashed-bubble \
        --disable-features=TranslateUI \
        --no-first-run \
        --disable-pinch \
        --overscroll-history-navigation=0 \
        "$URL"

    sleep 2
done
