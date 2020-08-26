#!/usr/bin/env bash

INPUT='demux.h264'

ffmpeg                                                      \
    -fflags                   nobuffer                                         \
    -err_detect               ignore_err                                       \
    -i                        $INPUT                                           \
                                                                               \
    -max_muxing_queue_size    1024                                             \
                                                                               \
    -map                      v:0                                              \
                                                                               \
    -s                        720x720                                        \
    -c                        libx264                                       \
    -preset                   fast                                               \
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
    chunks/flip.mpd
    #/var/www/data/flip.mpd

#cp out2.mpd /var/www/data/mpdtest.mpd

#cat /var/www/data/init-stream0.m4s /var/www/data/*.m4s > /var/www/data/flip.mp4
#cat chunks/init-stream0.m4s chunks/*.m4s > flip.mp4

#wget 172.23.190.178/flip.mp4 --header "Host: denseserver.com"

#./../testProgs/testH264VideoStreamer ekstratest.264

    #-window_size              5                                                \
    #-extra_window_size        5                                                \

