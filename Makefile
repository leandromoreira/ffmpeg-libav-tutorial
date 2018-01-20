VIDEO_URL := http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

hello_world: clean small_bunny_1080p_60fps.mp4
	gcc -g -Wall -o build/hello_world -lavformat -lavcodec -lswscale -lz 0_hello_world.c \
	  && ./build/hello_world $(lastword $?)

bunny_1080p_60fps.mp4:
	wget -O $@ $(VIDEO_URL)

small_bunny_1080p_60fps.mp4: bunny_1080p_60fps.mp4
	ffmpeg -y -i $? -c copy -ss 00:01:24 -t 00:00:10 $@

clean:
	@rm -f ./build/hello_world
