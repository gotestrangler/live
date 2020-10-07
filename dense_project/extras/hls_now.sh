

INPUT='long.mp4'

 ffmpeg -err_detect ignore_err -i $INPUT 
  -f hls -hls_init_time 2 -hls_time 2 -hls_list_size 0 -hls_segment_type mpegts \
  chunks/manifes.m3u8


