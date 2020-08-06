#!/usr/bin/env bash

#INPUT='rtsp://root:toor@192.168.0.20/live1s1.sdp'
INPUT='test.264'

ffmpeg                                                                         \
    -fflags                   nobuffer                                         \
    -err_detect               ignore_err                                       \
    -rtsp_transport           tcp                                              \
    -i                        $INPUT                                           \
                                                                               \
    -max_muxing_queue_size    1024                                             \
                                                                               \
    -map                      v:0                                              \
    -map                      v:0                                              \
    -map                      v:0                                              \
                                                                               \
    -s                        1080x1080                                        \
    -c                        h264_nvenc                                       \
      -preset                 ll                                               \
      -flags                  +cgop                                            \
      -g                      30                                               \
                                                                               \
    -b:v:0                    1000k                                            \
    -maxrate:v:0              1200k                                            \
    -bufsize:v:0              1000k                                            \
                                                                               \
    -b:v:1                    4000k                                            \
    -maxrate:v:1              4800k                                            \
    -bufsize:v:1              4000k                                            \
                                                                               \
    -b:v:2                    6000k                                            \
    -maxrate:v:2              7200k                                            \
    -bufsize:v:2              6000k                                            \
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
    ./out.mpd
