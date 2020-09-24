
INPUT='long.mp4'

 ffmpeg -err_detect ignore_err -i $INPUT -max_interleave_delta 0 -max_muxing_queue_size 1024 \
  -c:v libx264 -s:v 720x720 -b:v 1000k\
  -c:a aac -b:a 128k \
  -f tee -map 0:v -map 0:a "[select=\'v:0,a\':f=dash\:seg_duration=2\:use_template=1\:use_timeline=1\:hls_playlist=1\:index_correction=1\:dash_segment_type=mp4]manifes.mpd|mix.ts"

