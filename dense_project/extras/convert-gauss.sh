
INPUT='jen.mp4'

 ffmpeg -err_detect ignore_err -i $INPUT \
                -map v:0 \
                -map v:0 \
                -map v:0 \
                -map a:0 \
                -c:v libx264 -s:v 720x720 -b:v 1000k \
                -c:a aac -b:a 128k              \
                -b:v:0                    1000k \
                -maxrate:v:0              1200k \
                -bufsize:v:0              1000k \
                -b:v:1                    4000k \
                -maxrate:v:1              4800k \
                -bufsize:v:1              4000k \
                -b:v:2                    6000k \
                -maxrate:v:2              7200k \
                -bufsize:v:2              6000k \
                -f tee \
                "[select=\'v:0,a\':f=hls\:hls_flags=split_by_time\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]chunks/manifes.m3u8|
                [select=\'v:1,a\':f=hls\:hls_flags=split_by_time\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]chunks/manifes1.m3u8|
                [select=\'v:2,a\':f=hls\:hls_flags=split_by_time\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]chunks/manifes2.m3u8"\
  

<<<<<<< HEAD
=======
 ffmpeg -err_detect ignore_err -i $INPUT -max_interleave_delta 0 -max_muxing_queue_size 1024 \
  -c:v libx264 -s:v 720x720 -b:v 1000k\
  -c:a aac -b:a 128k \
  -f tee -map 0:v -map 0:a "[select=\'v:0,a\':f=hls\:hls_flags=split_by_time\:hls_init_time=2\:hls_time=2\:hls_list_size=0\:hls_segment_type=mpegts]/var/www/data/manifes.m3u8"
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0

