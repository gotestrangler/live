
INPUT='hongkong.avi'

 ffmpeg -err_detect ignore_err -i $INPUT -max_interleave_delta 0 -max_muxing_queue_size 1024 \
                -map v:0 \
                -map v:0 \
                -map v:0 \
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
  -f hls -hls_init_time 2 -hls_time 2 -hls_list_size 0 -hls_segment_type mpegts \
<<<<<<< HEAD
  /chunks/sikky.m3u8

=======
  ../../../../../var/www/data/chunks/manifes.m3u8
>>>>>>> 35512bf6226f879281a013d36201ce2650a3b2a0


