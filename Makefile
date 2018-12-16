clean:
	@rm -rf ./build/*

make_hello: clean
	docker run -w /files --rm -it  -v `pwd`:/files leandromoreira/ffmpeg-devel \
	  gcc -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ /files/0_hello_world.c \
	  -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
	  -o /files/build/hello

run_hello: make_hello
	docker run -w /files --rm -it -v `pwd`:/files leandromoreira/ffmpeg-devel /files/build/hello /files/small_bunny_1080p_60fps.mp4

make_remuxing: clean
	docker run -w /files --rm -it  -v `pwd`:/files leandromoreira/ffmpeg-devel \
	  gcc -L/opt/ffmpeg/lib -I/opt/ffmpeg/include/ /files/2_0_remuxing.c \
	  -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil \
	  -o /files/build/remuxing

run_remuxing: make_remuxing
	docker run -w /files --rm -it -v `pwd`:/files leandromoreira/ffmpeg-devel /files/build/remuxing /files/small_bunny_1080p_60fps.mp4 /files/remuxed_small_bunny_1080p_60fps.ts

