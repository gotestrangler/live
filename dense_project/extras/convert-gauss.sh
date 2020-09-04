
INPUT='long.mp4'


# the `split=3` means split to three streams
ffmpeg -err_detect ignore_err -i $INPUT -filter_complex '[0:v]split=2[out1][out2]' \
        -map '[out1]' -c:a aac -b:a 128k -c:v libx264 -s 720x720 -b:v 1000k -seg_duration 2 -use_template 1 -use_timeline 1 -hls_playlist 1 -index_correction 1 -dash_segment_type mp4  chunks/manifest.mpd \
        -map '[out2]' -c:a aac -b:a 128k -c:v libx264 -s 720x720 -b:v 1000k chunks/mix.ts 

