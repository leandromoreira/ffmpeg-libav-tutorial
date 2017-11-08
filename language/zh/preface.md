[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)
# 介绍

我正在找一个教程/书，教会我如何开始使用[FFmpeg]（https://www.ffmpeg.org/）作为一个库（又名libav），然后我发现[“如何写一个视频播放器不到1K行“]（http://dranger.com/ffmpeg/）教程，但它被弃用，我决定写这个。

在这里的大部分代码将在c **中，但不用担心**您可以轻松理解并将其应用于您的首选语言。 FFmpeg libav对许多语言具有很多绑定，如[python]（https://mikeboers.github.io/PyAV/），[go]（https://github.com/imkira/go-libav），甚至你的语言没有它，你仍然可以通过`ffi`来支持它，下面是一个例子[Lua]（https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua ）。

我们将从一个关于什么是视频，音频，编解码器和容器的快速课程开始，然后我们将进入关于如何使用`ffmpeg`命令行的崩溃课程，最后我们将编写代码，随意直接跳过[学习FFmpeg libav的艰辛之路]（＃learn-ffmpeg-libav-the -艰辛的道路）

有人曾经说过，互联网视频流是传统电视的未来，无论如何FFmpeg是值得研究的东西。