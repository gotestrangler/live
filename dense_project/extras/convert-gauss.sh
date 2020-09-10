
INPUT='long.mp4'

<<<<<<< HEAD
 ffmpeg -err_detect ignore_err -i $INPUT -max_muxing_queue_size 1024 \
  -c:v libx264 -s:v 720x720 -b:v 1000k\
  -c:a aac -b:a 128k \
  -f tee -map 0:v -map 0:a "[select=\'v:0,a\':f=dash\:seg_duration=2\:use_template=1\:use_timeline=1\:hls_playlist=1\:index_correction=1\:dash_segment_type=mp4]manifes.mpd|mix.ts"

=======

# the `split=3` means split to three streams
ffmpeg -err_detect ignore_err -i $INPUT -filter_complex '[0:v]split=2[out1][out2]' \
        -map '[out1]' -c:a aac -b:a 128k -c:v libx264 -s 720x720 -b:v 1000k -seg_duration 2 -use_template 1 -use_timeline 1 -hls_playlist 1 -index_correction 1 -dash_segment_type mp4  chunks/manifest.mpd \
        -map '[out2]' -c:a aac -b:a 128k -c:v libx264 -s 720x720 -b:v 1000k chunks/mix.ts 
>>>>>>> 7c37e548d01a9e6d799386bed91b5b31213b35aa

