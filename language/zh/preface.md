[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)
# 介绍

我正在找一个教程/书，教会我如何开始使用[FFmpeg](https://www.ffmpeg.org/)作为一个库（又名 libav），然后我发现[“如何写一个视频播放器不到1K行“](http://dranger.com/ffmpeg/)教程，但它被弃用，我决定写这个。

在这里的大部分代码将在c**但不用担心**您可以轻松理解并将其应用于您的首选语言。FFmpeg libav 对许多语言具有很多绑定，如：[python](https://mikeboers.github.io/PyAV/)、[go](https://github.com/imkira/go-libav)，甚至你的语言没有它，你仍然可以通过`ffi`来支持它，下面是一个例子[Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)。

我们将从一个关于什么是视频，音频，编解码器和容器的快速课程开始，然后我们将进入关于如何使用`ffmpeg`命令行的速成课程，最后我们将编写代码，随意直接跳过[学习FFmpeg libav的艰辛之路](https://github.com/leandromoreira/ffmpeg-libav-tutorial#learn-ffmpeg-libav-the-hard-way)

有人曾经说过，互联网视频流是传统电视的未来，无论如何FFmpeg是值得研究的东西。

##  视频 - 你看到了什么！

如果你有一系列的图像序列，并以给定的频率改变它们，那么假设[每秒钟有24个图像](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)，那么就会产生一个[运动错觉](https://en.wikipedia.org/wiki/Persistence_of_vision)，总之这是视频背后的一个非常基本的想法，一系列运行的图像/帧以给定的速度。

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)