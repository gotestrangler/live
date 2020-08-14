#!/usr/bin/env bash

INPUT='ekstratest.264'

ffmpeg                                                      \
    -fflags                   nobuffer                                         \
    -err_detect               ignore_err                                       \
    -i                        $INPUT                                           \
                                                                               \
    -max_muxing_queue_size    1024                                             \
                                                                               \
    -map                      v:0                                              \
                                                                               \
    -s                        1080x1080                                        \
    -c                        libx264                                       \
      -preset                 fast                                               \
      -flags                  +cgop                                            \
      -g                      30                                               \
                                                                               \
    -b:v:0                    1000k                                            \
    -maxrate:v:0              1200k                                            \
    -bufsize:v:0              1000k                                            \
                                                                               \
    -seg_duration             2                                                \
    -window_size              5                                                \
    -extra_window_size        5                                                \
    -use_template             1                                                \
    -use_timeline             0                                                \
    -hls_playlist             1                                                \
    -streaming                1                                                \
    -index_correction         1                                                \
    -dash_segment_type        mp4                                              \
    -remove_at_exit           0                                                \
                                                                               \
    /var/www/data/mpdtest.mpd

#cp out2.mpd /var/www/data/mpdtest.mpd

cat init-stream0.m4s *.m4s > ekstraout.mp4

#./../testProgs/testH264VideoStreamer ekstratest.264


