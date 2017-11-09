download:
	wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

cut_smaller_version:
	ffmpeg -y -i bunny_1080p_60fps.mp4 -c copy -ss 00:01:24 -t 00:00:10 small_bunny_1080p_60fps.mp4

hello_world: clean
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world small_bunny_1080p_60fps.mp4

clean:
	rm -f ./build/hello_world

