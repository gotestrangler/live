#!/usr/bin/env bash

#INPUT='rtsp://root:toor@192.168.0.20/live1s1.sdp'
INPUT='test.264'


ffmpeg -i $INPUT \
  -map 0:v:0 -map 0:a\?:0 -map 0:v:0 -map 0:a\?:0 -map 0:v:0 -map 0:a\?:0  \
  -b:v:0 1000k -c:v:0 libx264 -filter:v:0 "scale=320:-1"  \
  -b:v:1 4000k -c:v:1 libx264 -filter:v:1 "scale=640:-1"  \
  -b:v:2 6000k -c:v:2 libx264 -filter:v:2 "scale=1280:-1" \
  -use_timeline 1 -use_template 1 -window_size 6 -adaptation_sets "id=0,streams=v  id=1,streams=a" \
  -hls_playlist true -f dash output.mpd
