
INPUT='long.mp4'

 ffmpeg -err_detect ignore_err -i $INPUT -max_interleave_delta 0 -max_muxing_queue_size 1024 \
  -c:v libx264 -s:v 720x720 -b:v 1000k\
  -c:a aac -b:a 128k \
  -f tee -map 0:v -map 0:a "[select=\'v:0,a\':f=hls\:hls_flags=split_by_time\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]chunks/manifes.m3u8"

