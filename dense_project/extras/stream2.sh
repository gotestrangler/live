#!/usr/bin/env bash

sudo /etc/init.d/nginx restart

INPUT='long.mp4'

ffmpeg                                                                          \
    -fflags                   nobuffer                                          \
    -err_detect               ignore_err                                        \
    -i                        $INPUT                                            \
                                                                                \
    -max_muxing_queue_size    1024                                              \
                                                                                \
    -map                      v:0                                               \
    -map                      0:a\?:0                                           \
    -c:a                      aac                                               \
    -b:a                      128k                                              \
                                                                                \
    -s                        720x720                                           \
    -c:v                      libx264                                           \
    -preset                   fast                                              \
                                                                               \
    -b:v:0                    1000k                                            \
    -maxrate:v:0              1200k                                            \
    -bufsize:v:0              1000k                                            \
                                                                               \
    -seg_duration             2                                                \
    -use_template             1                                                \
    -use_timeline             1                                                \
    -hls_playlist             1                                                \
    -streaming                0                                                \
    -index_correction         1                                                \
    -dash_segment_type        mp4                                              \
    -remove_at_exit           0                                                \
                                                                               \
    /var/www/data/manifest.mpd

cat /var/www/data/init-stream0.m4s /var/www/data/chunk-stream0* > /var/www/data/video_only.mp4
cat /var/www/data/init-stream1.m4s /var/www/data/chunk-stream1* > /var/www/data/audio_only.mp4

ffmpeg -i /var/www/data/video_only.mp4 -i /var/www/data/audio_only.mp4 -c copy /var/www/data/mix.ts 

wget 172.23.89.214/20/mix.ts --header "Host: denseserver.com"

#./../testProgs/transportStreamer mix.ts

    #-window_size              5                                                \
    #-extra_window_size        5                                                \

