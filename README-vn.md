[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

Tôi đang tìm một bài hướng dẫn về cách sử dụng [FFmpeg](https://www.ffmpeg.org/) như một thư viện (được biết đến là libav) và sau đó tôi đã tìm thấy bài viết ["Cách viết video player ít hơn 1000 dòng"](http://dranger.com/ffmpeg/).
Thật không may, nó không còn được dùng nữa, vì vậy tôi quyết định viết bài hướng dẫn này.

Tất cả dòng code ở đây được viết bằng ngôn ngữ C, nhưng đừng lo lắng: bạn có thể dễ dàng hiểu và áp dụng nó với ngôn ngữ bạn mong muốn.
Thư viện FFmpeg libav có rất nhiều biến thể cho các ngôn ngữ khác nhau như [python](https://pyav.org/), [go](https://github.com/imkira/go-libav) và thậm chí nếu ngôn ngữ bạn sử dụng không có thư viện này, bạn vẫn được hỗ trợ qua `ffi` (đây là một ví dụ với [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)).

Chúng ta sẽ bắt đầu với một tiết học nhanh về video, audio, codec và container; tiếp đó, chúng ta đi vào khoá học sâu hơn về cách sử dụng câu lệnh `FFmpeg` và cuối cùng chúng ta sẽ viết code. Đừng ngại bỏ qua phần đầu và nhảy thẳng đến phần [Tìm hiểu thư viện FFmpeg libav sâu hơn.](#learn-ffmpeg-libav-the-hard-way).

Một vài người thường nói phát trực tuyến video trên Internet là tương lai của TV truyền thống, dù bất cứ tình huống gì, FFmpeg là một thứ đáng để học.

__Mục lục__

- [Giới thiệu](#giới-thiệu)
  - [Video - Điều bạn thấy!](#video---điều-bạn-thấy)
  - [Audio - Điều bạn nghe!](#audio---điều-bạn-nghe)
  - [Codec - Nén dữ liệu](#codec---nén-dữ-liệu)
  - [Container - Định dạng tệp lưu trữ chung video và audio](#container---định-dạng-tệp-lưu-trữ-chung-video-và-audio)
- [FFmpeg - Bộ công cụ dưới dạng câu lệnh](#ffmpeg---bộ-công-cụ-dưới-dạng-câu-lệnh)
  - [Bộ công cụ câu lệnh FFmpeg 101](#bộ-công-cụ-câu-lệnh-ffmpeg-101)
- [Những hành động xử lý video phổ biến](#những-hành-động-xử-lý-video-phổ-biến)
  - [Chuyển đổi chuẩn nén - Transcoding](#chuyển-đổi-chuẩn-nén---transcoding)
  - [Chuyển đổi định dạng tệp - Transmuxing](#chuyển-đổi-định-dạng-tệp---transmuxing)
  - [Thay đổi tốc độ bit - Transrating](#thay-đổi-tốc-độ-bit---transrating)
  - [Thay đổi độ phân giải - Transsizing](#thay-đổi-độ-phân-giải---transsizing)
  - [Mở rộng: phát trực tuyến thích ứng (Adaptive-streaming)](#mở-rộng-phát-trực-tuyến-thích-ứng-adaptive-streaming)
  - [Hơn thế nữa](#hơn-thế-nữa)
- [Tìm hiểu thư viện FFmpeg libav sâu hơn](#tìm-hiểu-thư-viện-ffmpeg-libav-sâu-hơn)
  - [Chapter 0 - Hello world nổi tiếng](#chapter-0---hello-world-nổi-tiếng)
    - [Kiến trúc thư viện FFmpeg libav](#kiến-trúc-thư-viện-ffmpeg-libav)
    - [Các yêu cầu](#các-yêu-cầu)
    - [Chương 0 - lướt qua các dòng code](#chương-0---lướt-qua-các-dòng-code)
  - [Chapter 1 - Đồng bộ audio và video](#chapter-1---đồng-bộ-audio-và-video)
  - [Chapter 2 - Remuxing](#chapter-2---remuxing)
  - [Chapter 3 - Transcoding](#chapter-3---transcoding)
    - [Transmuxing](#transmuxing)
    - [Transcoding](#transcoding)
  
# Giới thiệu

## Video - Điều bạn thấy!

Nếu bạn có một chuỗi tuần tự các hình ảnh và thay đổi chúng ở một tần số đã biết (hãy ví dụ như [24 hình trên giây](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)), bạn sẽ tạo ra [ảo giác về sự chuyển động](https://en.wikipedia.org/wiki/Persistence_of_vision).
Tóm lại, đây là nguyên lý cơ bản đằng sau video: **một chuỗi các hình ảnh chạy với tốc độ cho trước**. 

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

## Audio - Điều bạn nghe!

Mặc dù video không âm thanh có thể mang đến rất nhiều cảm xúc, nhưng việc bổ sung thêm âm thanh sẽ mang lại nhiều trải nghiệm hứng khởi hơn.

Âm thanh là sự rung động lan truyền như sóng áp suất, thông qua không khí hoặc bất cứ phương tiện truyền dẫn khác, như khí gas, chất lỏng hoặc đất.

> Trong một hệ thống âm thanh kỹ thuật số, microphone chuyển đổi âm thanh thành tín hiệu điện tương tự, sau đó qua bộ chuyển đổi tương tự - số (analog-to-digital converter ADC) - tiêu biểu sử dụng [điều chế độ rộng xung (pulse-code modulation PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation) - chuyển đổi tín hiệu tương tự sang tín hiệu số.

![chuyển đổi tín hiệu tương tự âm thành sang tín hiệu số](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")
>[Nguồn](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## Codec - Nén dữ liệu

> CODEC là một mạch điện tử hoặc phần mềm dùng để **nén hoặc giải nén dữ liệu video/audio kỹ thuật số.** Nó chuyển đổi dữ liệu video/audio số hoá nguyên thuỷ (chưa nén) sang định dạng nén hoặc ngược lại.
> https://en.wikipedia.org/wiki/Video_codec

Nhưng nếu chúng ta chọn đóng gói hàng triệu hình ảnh vào trong tệp tài liệu và gọi nó là một bộ phim, chúng ta có thể nhận được một tệp tài liệu khổng lồ. Hãy thử tính toán một chút:

Giả sử chúng ta đang tạo một video với độ phân giải `1080 x 1920` (cao x rộng) và chúng ta dành `3 bytes` cho mỗi điểm ảnh (pixel - đơn vị nhỏ nhất của một màn hình) để mã hoá màu sắc (hoặc [màu sắc 24 bit](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), nó đại diện cho 16,777,216 màu sắc khác nhau), và video này chạy ở tốc độ `24 hình trên giây`, kéo dài `30 phút`.

```c
toppf = 1080 * 1920 //tong_so_diem_anh_tren_mot_hinh
cpp = 3 //gia_tri_cho_moi_diem_anh
tis = 30 * 60 //thoi_gian_tinh_bang_giay
fps = 24 //so_hinh_tren_giay

bo_nho_yeu_cau = tis * fps * toppf * cpp
```

Video này sẽ yêu cầu xấp xỉ bộ nhớ `250.28GB` hoặc băng thông `1.19Gbps`! Đó là lý do tại sao chúng ta cần dùng [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work).

## Container - Định dạng tệp lưu trữ chung video và audio

> Một container hay định dạng tệp là một định dạng tập tin mà thông số của nó miêu tả những thành phần khác nhau của dữ liệu và thông tin cũng tồn tại như thế nào trong một tập tin máy tính.
> https://en.wikipedia.org/wiki/Digital_container_format

Một **tệp tin chứa tất cả các luồng dữ liệu** (bao gồm tất cả audio và video) và nó cũng cung cấp cơ chế đồng bộ và thông tin chung, như tựa đề, độ phân giải,...

Thông thường chúng ta có thể suy luận định dạng của tệp dữ liệu bằng cách nhìn vào phần mở rộng tên tệp: ví dụ như một tệp có tên `video.webm` là một video sử dụng định dạng container [`webm`](https://www.webmproject.org/).

![container](/img/container.png)

# FFmpeg - Bộ công cụ dưới dạng câu lệnh

> Một giải pháp hoàn thiện, đa nền tảng để ghi lại, chuyển đổi và phát trực tuyến luồng audio và video.

Để làm việc với truyền thông đa phương tiện, chúng ta có thể sử dụng công cụ/thư viện hữu ích gọi là [FFmpeg](https://www.ffmpeg.org/). Rất có thể bạn đã từng biết/sử dụng nó một cách trực tiếp hoặc gián tiếp (bạn có sử dụng [Chrome?](https://www.chromium.org/developers/design-documents/video)).

Nó có một chương trình chạy lệnh gọi là `ffmpeg`, một chương trình mã nhị phân đơn giản nhưng vô cùng mạnh mẽ
Ví dự như bạn có thể chuyển đổi từ định dạng `mp4` sang định dạng container `avi` chỉ bằng cách gỗ câu lệnh sau:

```bash
$ ffmpeg -i input.mp4 output.avi
```

Chúng ta chỉ thực hiện một bước **định dạng lại (remuxing)** ở đây, nghĩa là nó đang chuyển đổi từ định dạng container này sang một định dạng container khác.
Về mặt kỹ thuật FFmpeg cũng có thể thực hiện thêm một bước chuyển đổi chuẩn nén (transcode) nhưng chúng ta sẽ nói về nó sau.

## Bộ công cụ câu lệnh FFmpeg 101

FFmpeg có một trang chủ [tài liệu](https://www.ffmpeg.org/ffmpeg.html) đã giải thích rõ ràng đầy đủ về nguyên lý hoạt động của nó. 

Ngắn gọn mà nói, chương trình câu lệnh FFmpeg cần định dạng đối số sau để thực hiện hành động của nó `ffmpeg {1} {2} -i {3} {4} {5}` trong đó:

1. tuỳ chọn toàn cục
2. tuỳ chọn đầu vào
3. đường dẫn đầu vào
4. tuỳ chọn đầu ra
5. đường dẫn đầu ra

Các phần 2, 3, 4 và 5 có thể là một hoặc nhiều theo như yêu cầu của bạn.
Thật dễ dạng để hiểu những định dạng đối số này trong câu lệnh dưới đây:

``` bash
# WARNING: kích thước file xấp xỉ 300MB
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # lựa chọn toàn cục
-c:a libfdk_aac \ # tuỳ chọn đầu vào
-i bunny_1080p_60fps.mp4 \ # đường dẫn đầu vào
-c:v libvpx-vp9 -c:a libvorbis \ # tuỳ chọn đầu ra
bunny_1080p_60fps_vp9.webm # đường dẫn đầu ra
```
Câu lệnh này nhận tệp đầu vào định dạng `mp4` chứa 2 luồng dữ liệu (một luồng audio nén với chuẩn nén `aac` và một luồng video sử dụng chuẩn nén `h264`) và chuyển đổi nó sang định dạng tệp `webm`, cũng thay đổi chuẩn nén audio và video của nó.

Chúng ta có thể đơn giản hoá các câu lệnh trên nhưng hãy lưu ý rằng FFmpeg sẽ nhận hoặc dự đoán các giá trị mặc định cho bạn.
Ví dụ, khi bạn gõ `ffmpeg -i input.avi output.mp4`, chuẩn nén audio/video sẽ được sử dụng để xuất `output.mp4` là gì?

Werner Robitza đã viết một bài hướng dẫn nên đọc về [nén và chính sửa với FFmpeg](http://slhck.info/ffmpeg-encoding-course/#/).

# Những hành động xử lý video phổ biến

Trong khi làm việc với audio/video, chúng ta thường thực hiện một bộ các tác vụ cụ thể với nội dung đa phương tiện.

## Chuyển đổi chuẩn nén - Transcoding

![transcoding](/img/transcoding.png)

**Là gì?** là hành động chuyển đổi một luồng dữ liệu (có thể là audio hoặc video) từ chuẩn nén này sang chuẩn nén khác.

**Tại sao?** thỉnh thoảng chúng ta bắt gặp trường hợp một vài thiết bị (Tivi, điện thoại thông minh, bẳng điều khiển,...) không hỗ trợ loại X nhưng lại hỗ trợ loại Y và những chuẩn nén mới cung cấp tỉ lệ nén tốt hơn.

**Như thế nào?** chuyển đổi một video từ chuẩn nén `H264` (AVC) sang chuẩn nén `H265` (HEVC) bằng cách sau:
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## Chuyển đổi định dạng tệp - Transmuxing

![transmuxing](/img/transmuxing.png)

**Là gì?** là hành động chuyển đổi từ một định dạng tệp (container) này sang một định dạng tệp khác.

**Tại sao?** thỉng thoảng một vài thiết bị (Tivi, điện thoại thông minh, bẳng điều khiển,...) không hỗ trợ loại X nhưng lại hỗ trợ loại Y và thỉnh thoảng những định dạng mới cung cấp những tính năng hiện đại được yêu cầu.

**Như thế nào?** thực hiện chuyển đổi từ định dạng `mp4` sang định dạng `webm`.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # dieu_khien_ffmpeg_bo_qua_buoc_giai_ma_va_ma_hoa
bunny_1080p_60fps.webm
```

## Thay đổi tốc độ bit - Transrating

![transrating](/img/transrating.png)

**Là gì?** là hành động thay đổi tốc độ bit của video/audio, hoặc xuất ra những biến thể (renditions) khác.

**Tại sao?** mọi người có thể thử xem video của bạn với kết nối mạng `2G`(edge) bằng cách sử dụng các thiết bị điện thoại thông minh hiệu năng thấp hoặc bằng kết nối Internet `cáp quang` (fiber) trên thiết bị Tivi 4K của họ. Do đó, bạn nên đề xuất nhiều hơn một biến thể của cùng một video với tốc độ bit khác nhau.

**Như thế nào?** tiến hành xuất một biến thể với tốc độ bit giữa 3856K và 2000K.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

Thông thường chúng ta sẽ cùng sử dụng 2 tác vụ thay đổi tốc độ và thay đổi kích thước. Werner Robitza đã viết một chuỗi các bài viết nên đọc về [điều khiển tỉ lệ trong FFmpeg](http://slhck.info/posts/).

## Thay đổi độ phân giải - Transsizing

![transsizing](/img/transsizing.png)

**Là gì?** là hành động thay đổi chất lượng video từ độ phân giải này sang một độ phân giải khác. Như đã nói trước đó, tác vụ thay đổi kích thước thường đi kèm với tác vụ thay đổi tốc độ.

**Tại sao?** lý do tương tư như với tác vụ thay đổi tốc độ.

**Như thế nào?** thay đổi từ độ phân giải từ `1080p` thành `480p`.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## Mở rộng: phát trực tuyến thích ứng (Adaptive-streaming)

![adaptive streaming](/img/adaptive-streaming.png)

**Là gì?** là hành động xuất nhiều độ phân giải (hoặc tốc độ bit) và chia nội dung đa phương tiện thành các đoạn và truyền tải chúng thông qua giao thức http.

**Tại sao?** để cung cấp nội dung đa phương tiện linh hoạt để có thể xem trên điện thoại thông minh hiệu năng thấp hoặc tivi 4K, nó cũng dễ dàng mở rộng và triển khai nhưng có thể tăng thêm độ trễ.

**Như thế nào?** tạo ra một nội dụng định dạng WebM thích ứng (adaptive) bằng cách sử dụng giao thức DASH.
```bash
# luồng video
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# luồng audio
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# tệp kê khai DASH
$ ffmpeg \
 -f webm_dash_manifest -i video_160x90_250k.webm \
 -f webm_dash_manifest -i video_320x180_500k.webm \
 -f webm_dash_manifest -i video_640x360_750k.webm \
 -f webm_dash_manifest -i video_640x360_1000k.webm \
 -f webm_dash_manifest -i video_1280x720_500k.webm \
 -f webm_dash_manifest -i audio_128k.webm \
 -c copy -map 0 -map 1 -map 2 -map 3 -map 4 -map 5 \
 -f webm_dash_manifest \
 -adaptation_sets "id=0,streams=0,1,2,3,4 id=1,streams=5" \
 manifest.mpd
```

PS: Tôi đã lấy ví dụ này từ bài [Giới thiệu cách thức xem lại WebM thích ứng bằng giao thức DASH](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## Hơn thế nữa

Còn [rất nhiều cách sử dụng khác nữa của FFmpeg](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly).
Tôi sử dụng nó khi kết hợp với *iMovie* để xuất ra/chỉnh sửa một vài video cho nền tảng Youtube và bạn chắc chắn có thể sử dụng nó một cách chuyên nghiệp hơn.

# Tìm hiểu thư viện FFmpeg libav sâu hơn

> Bạn không nên lo lắng quá nhiều về âm thanh và hình ảnh?
> **David Robert Jones**

Bởi vì [FFmpeg](#ffmpeg---command-line) là một câu lệnh rất hữu dụng để làm những tác vụ thiết yếu trên các tệp tin đa phương tiện, bằng cách nào chúng ta có thể sử dụng nó trong chương trình của chúng ta?

FFmpeg được [kết hợp bởi một vài thư viện](https://www.ffmpeg.org/doxygen/trunk/index.html) mà có thể tích hợp vào trong chương trình của chúng ta.
Thông thường, khi bạn cài đặt FFmpeg, nó sẽ tự động cài tất cá các thư viện đó. Tôi sẽ tham chiếu đến tập các thư viện gọi là **FFmpeg libav**.

>> Tựa đề này là trang chủ của chuỗi các bài viết của Zed Shaw ["Học X chuyên sâu"](https://learncodethehardway.org/), đặc biệt là cuốn sách của anh ấy "Học ngôn ngữ C chuyên sâu" (Learn C the Hard Way).

## Chapter 0 - Hello world nổi tiếng

Chương trình Hello world này thực chất sẽ không hiển thị tin nhắn `"hello world"` trên màn hình terminal :tongue: Thay vào đó, chúng ta sẽ in ra thông tin của video, ví dụ như là định dạng tệp (container) của nó, thời lượng, độ phân giải, các kênh audio và cuối cùng, chúng ta sẽ **giải nén một số khung hình (frames) và lưu chúng lại như tệp tin hình ảnh.**

### Kiến trúc thư viện FFmpeg libav

Trước khi chúng ta bắt đầu viết chương trình, hãy học cách **kiến trúc thư viện FFmpeg libav** hoạt động và các thành phần của nó giao tiếp với nhau như thế nào.

Đây là sơ đồ tiến trình giải nén một video:

![kiến trúc thư viện ffmpeg libav - tiến trình giải nén](/img/decoding.png)

Đầu tiên bạn sẽ cần tải lên một tệp tin đa phương tiện của bạn vào thành phần gọi là [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) (Containter của video còn được gọi là định dạng).
Nó thực chất không tải toàn bộ tệp tin: nó thường chỉ đọc phần đầu header của tệp tin.

Một khi chúng đọc được ít nhất **phần đầu (header) của container**, chúng ta có thể truy cập vào các luồng dữ liệu của nó (nghĩ chúng như là phần thông tin chung của dữ liệu audio và video).
Mỗi luồng (stream) sẽ được lưu trong thành phần gọi là [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html).

> Luồng là một cái tên ưa thích đại điện cho một dòng dữ liệu liên tục.

Giả sử chúng ta có một video chứa hai luồng dữ liệu: một luồng là audio được nén với [chuẩn nén AAC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) và luồng còn lại là video được nén với [chuẩn nén H264 (AVC)](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC). Từ mỗi luồng, chúng ta có thể giải nén **từng mảnh (slices) của dữ liệu** gọi là gói (packet) mà chúng sẽ được tải vào những phần tử [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html).

Phần **dữ liệu trong các gói vẫn được nén** và để giải nén các gói, chúng ta cần đưa chúng vào [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html) cụ thể.

Thành phần `AVCodec` sẽ giải mã chúng thành phần tử [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) và cuối cùng, những phần tử này sẽ cho chúng ta những khung hình gốc không nén. Có thể nhận ra rằng thuật ngữ/tiến trình đều được sử dụng bởi cả luồng audio và video.

### Các yêu cầu

Bởi có một số người đã [gặp vấn để trong khi biên dịch hoặc chạy các ví dụ mẫu](https://github.com/leandromoreira/ffmpeg-libav-tutorial/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+compiling) **chúng ta sẽ sử dụng [`Docker`](https://docs.docker.com/install/) như là một trường phát triển hay chạy thử,** chúng ta cũng sẽ sử dụng video "The big buck bunny" vì thế nếu bạn không có nó ở trên máy tính thì hãy chạy lệnh `make fetch_small_bunny_video`.

### Chương 0 - lướt qua các dòng code

> #### TLDR; hãy mở [code](/0_hello_world.c) và thực thi nó.
> ```bash
> $ make run_hello
> ```

Chúng ta sẽ bỏ qua một số chi tiết, nhưng đừng lo lắng: [source code có sẵn trên github](/0_hello_world.c).

Chúng ta sẽ khởi tạo vùng nhớ cho thành phần [`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) để giữ các thông tin về định dạng tệp (container).

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

Bây giở chúng ta sẽ mở tệp tin và đọc phần đầu (header) của nó và điền vào `AVFormatContext` với thông tin tối thiểu về định dạng (lưu ý rằng các chuẩn nén vẫn chưa được xác định).
Hàm được sử dụng để làm điều đó là [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49). Nó cần đầu vào là một `AVFormatContext`, một `tên file (filename)` và đối số tuỳ chọn: [`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html) (nếu bạn đưa vào `NULL`, FFmpeg sẽ dự đoán định dạng) và [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html) (là các tuỳ chọn cho bộ demuxer).  

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

Chúng ta có thể in tên định dạng và thời lượng đa phương tiện:

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

Để truy cập vào `các luồng`, chúng ta cần đọc dữ liệu từ đa phương tiện. Hàm [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb) thực hiên điều đó.
Bây giờ, thành phần `pFormatContext->nb_streams` sẽ giữ số lượng các luồng và `pFormatContext->streams[i]` sẽ cho chúng ta các thông tin về luồng `i` (tương ứng với một [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html)).

```c
avformat_find_stream_info(pFormatContext,  NULL);
```

Bây giờ chúng ta sẽ chạy vòng lặp qua tất cả các luồng.

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
  //
}
```

Với mỗi luồng, chúng ta sẽ cần [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html), nó miêu tả các thuộc tính của chuẩn nén được sử dụng với luồng `i`.

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```

với thuộc tính của chuẩn nén, chúng ta có thể tìm chuẩn nén thích hợp thông qua hàm [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca) và tìm bộ giải nén sẵn có với mã định danh của chuẩn nén đó (code id) và trả về một [`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html), thành phần biết cách thức thực hiện nén (en**CO**de) và giải nén (**DEC**ode) luồng dữ liệu.
```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

Đến giờ chúng ta có thể in thông tin về chuẩn nén

```c
// specific for video and audio
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
  printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
  printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// general
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
```

Với thông tin chuẩn nén, chúng ta khởi tạo vùng nhớ cho [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html), nó sẽ giữ nội dung của tiến trình giải mã/mã hoá, nhưng sau đó chúng ta cần điền nội dung chuẩn nén với các thông số đã xác định; chúng ta làm thực hiện với hàm [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16).

Một khi chúng ta đã điền vào nội dung bộ mã hoá, chúng ta có thể mở bộ mã hoá. Chúng ta gọi hàm [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) và sau đó chúng ta có thể sử dụng nó.

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

Bây giờ, chúng ta sẽ đọc các gói dữ liệu từ luồng stream và giải mã chúng thành các khung hình nhưng trước tiên, chúng ta cần khởi tạo bộ nhớ cho cả hai thành phần, [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) và [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html).

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

Hãy lấy các gói dữ liệu từ luồng stream với hàm [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61) trong khi nó có dữ liệu.

```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
  //...
}
```

**Đưa gói dữ liệu thô** (hình đã nén) vào bộ giải mã, đi qua bộ mã hoá, bằng hàm [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3).

```c
avcodec_send_packet(pCodecContext, pPacket);
```

Và **Nhận hình ảnh thô** (hình đã giải nén) từ bộ giải mã, thông qua bộ mã hoá tương tự, bằng hàm [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c).

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

Chúng ta có thể in ra số lượng khung hình, thông số [PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [frame type](https://en.wikipedia.org/wiki/Video_compression_picture_types) và nhiều hơn thế.

```c
printf(
    "Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]",
    av_get_picture_type_char(pFrame->pict_type),
    pCodecContext->frame_number,
    pFrame->pts,
    pFrame->pkt_dts,
    pFrame->key_frame,
    pFrame->coded_picture_number,
    pFrame->display_picture_number
);
```

Cuối cùng chúng ta có thể lưu lại những khung hình đã được giải nén thành một ảnh xám đơn giản [simple gray image](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example). Quá trình này rất đơn giản, chúng ta sẽ dùng `pFrame->data` nơi mà index liên quan đến không gian màu [planes Y, Cb and Cr](https://en.wikipedia.org/wiki/YCbCr), chúng ta chỉ cần lấy phần tử đầu tiên với chỉ mục `0` (Y) để lưu thành hình ảnh.

```c
save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
```

Vậy là cuối cùng chúng ta có một ảnh xám với kích thước 2MB:

![saved frame](/img/generated_frame.png)

## Chapter 1 - Đồng bộ audio và video

> **Player** - một nhà phát triển JS hoàn thành một trình phát video mới.

Trước khi chúng ta đến với [ví dụ về  transcoding](#chapter-2---transcoding) hãy nói về **đồng bộ thời gian** , hoặc cách thức một trình phát video biết khi nào cần hiển thị hình ảnh.

Trong ví dụ cuối cùng, chúng ta đã lưu một số khung hình có thể xem được ở đây:

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

Khi chúng ta thiết kế trình phát video, chúng ta cần hiển thị từng khung hình theo một tốc độ nhất định, nếu không, sẽ rất khó để xem video một cách thoải mái bởi vì nó phát rất nhanh hoặc rất chậm.

Do đó, chúng ta cần xác định một số logic để phát mỗi khung hình một cách mượt mà. Để xử lý vấn đề này, mỗi khung hình có một **mốc thời gian hiển thị** (PTS) tăng dần theo hệ số **timebase**, là một số hữu tỉ (trong đó mẫu số được biết đến như **timescale**), chia cho **tốc độ khung hình (fps)** 

Sẽ dễ dàng để hiểu khi chúng ta nhìn vào một số ví dụ, hãy thực hiện một số kịch bản.

Với `fps=60/1` và `timebase=1/60000`, mỗi PTS sẽ tăng lên `timescale / pts = 1000`, do đó **PTS thời gian thực** cho mỗi khung hình sẽ là (giả định bắt đầu từ 0):

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

Với kịch bản tương tự nhưng timebase bằng `1/60`. 

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

Với `fps=25/1` và `timebase=1/75`, mỗi PTS sẽ tăng một khoảng `timescale / pts = 3` và mốc thời gian PTS sẽ là:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

Bây giờ với `pts_time` chúng ta có thể tìm được cách kết xuất video đồng bộ với `pts_time` của audio hoặc với nhịp xung hệ thống. Thư viện FFmpeg libav cung cấp những thông tin này thông qua API:  

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

Xem xét sâu hơn, những khung hình chúng ta lưu được gửi theo thứ tự DTS (frames: 1,6,4,2,3,5) nhưng phát theo thứ tự PTS (frames: 1,2,3,4,5). Cũng để ý xem hiệu quả của khung hình loại B so với khung hình loại P hoặc I.

```
LOG: AVStream->r_frame_rate 60/1
LOG: AVStream->time_base 1/60000
...
LOG: Frame 1 (type=I, size=153797 bytes) pts 6000 key_frame 1 [DTS 0]
LOG: Frame 2 (type=B, size=8117 bytes) pts 7000 key_frame 0 [DTS 3]
LOG: Frame 3 (type=B, size=8226 bytes) pts 8000 key_frame 0 [DTS 4]
LOG: Frame 4 (type=B, size=17699 bytes) pts 9000 key_frame 0 [DTS 2]
LOG: Frame 5 (type=B, size=6253 bytes) pts 10000 key_frame 0 [DTS 5]
LOG: Frame 6 (type=P, size=34992 bytes) pts 11000 key_frame 0 [DTS 1]
```

## Chapter 2 - Remuxing

Remuxing là hành động thay đổi từ định dang tệp (container) này sang định dạng tệp khác, ví dụ, chúng ta thay đổi một video định dạng [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) sang định dạng [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) mà không gặp nhiều khó khăn khi sử dụng FFmpeg: 

```bash
ffmpeg input.mp4 -c copy output.ts
```

Nó sẽ bóc tách định dạng mp4 nhưng nó sẽ không giải mã hay mã hoá lại (`-c copy`) và cuối cùng, nó sẽ sắp xếp lại theo định dạng `mpegts`. Nếu bạn không cung cấp định dạng `-f`, ffmpeg sẽ cố gắng đoán nó bằng tên mở rộng của tập tin đầu ra.

Cách sử dụng thông thường của FFmpeg hoặc thư viện libav theo kiến trúc/ mẫu hoặc theo trình tự như sau:
* **[lớp giao thức](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - nó nhận `đầu vào`  (có thể là một `tập tin` hoặc là giao thức `rtmp` hay `HTTP`)
* **[lớp định dạng](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - nó `bóc tách` nội dung trong đó, lấy được hầu hết siêu dữ liệu và các luồng của nó
* **[lớp mã hoá](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - nó `giải mã` dữ liệu được nén trong các luồng <sup>*tuỳ chọn*</sup>
* **[lớp điểm ảnh](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** - nó cũng cung cấp  `bộ lọc` tương tác với từng khung hình gốc (như thay đổi kích thước)<sup>*tuỳ chọn*</sup>
* và sau đó thực hiện ngược lại các bước
* **[lớp mã hoá](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - nó `mã hoá` (hoặc `mã hoá lại` hoặc `transcode`) những frame gốc<sup>*tuỳ chọn*</sup>
* **[lớp định dạng](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - nó `sắp xếp` (hoặc `tái sắp xếp`) các luồng dữ liệu (dữ liệu nén)
* **[lớp giao thức](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - và cuối cũng những dữ liệu được sắp xếp sẽ được gửi đến `đầu ra` (tập tin khác hoặc có thể là một máy chủ mạng)

![ffmpeg libav workflow](/img/ffmpeg_libav_workflow.jpeg)
> Sơ đồ này được truyền cảm hứng từ sự nỗ lực của [Leixiaohua's](http://leixiaohua1020.github.io/#ffmpeg-development-examples) vả [Slhck's](https://slhck.info/ffmpeg-encoding-course/#/9).

Bây giờ hãy xem ví dụ sử dụng libav để đưa ra hiệu ứng tương tự như trong `ffmpeg input.mp4 -c copy output.ts`.

Chúng ta đọc từ đầu vào (`input_format_context`) và thay đổi nó thành đầu ra khác (`output_format_context`).

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

Chúng ta bắt đầu thực hiện khởi tạo vùng nhớ và mở định dạng đầu vào. Cho trường hợp này, chúng ta cần mở tập tin đầu vào và khởi tạo vùng nhớ cho tập tin đầu ra.

```c
if ((ret = avformat_open_input(&input_format_context, in_filename, NULL, NULL)) < 0) {
  fprintf(stderr, "Could not open input file '%s'", in_filename);
  goto end;
}
if ((ret = avformat_find_stream_info(input_format_context, NULL)) < 0) {
  fprintf(stderr, "Failed to retrieve input stream information");
  goto end;
}

avformat_alloc_output_context2(&output_format_context, NULL, NULL, out_filename);
if (!output_format_context) {
  fprintf(stderr, "Could not create output context\n");
  ret = AVERROR_UNKNOWN;
  goto end;
}
```

Chúng ta cũng cần sắp xếp lại các luồng stream video, audio và subtitle, vì để giữ chúng, chúng ta sẽ lưu thông tin của chúng trong mảng.

```c
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
```

Sau đó, chúng ta khởi tạo vùng nhớ yêu cầu, chúng ta thực hiện vòng lặp tất cả các luồng stream và với mỗi luồng stream chúng ta cần tạo một luồng stream đầu ra cho định dạng đầu ra, bằng cách dùng hàm [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827). Chú ý chúng ta sẽ đánh đấu tất cả các luồng stream kể cả không phải video, audio hoặc subtitle, vì vậy chúng ta có thể bỏ qua chúng.

```c
for (i = 0; i < input_format_context->nb_streams; i++) {
  AVStream *out_stream;
  AVStream *in_stream = input_format_context->streams[i];
  AVCodecParameters *in_codecpar = in_stream->codecpar;
  if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
      in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
      in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
    streams_list[i] = -1;
    continue;
  }
  streams_list[i] = stream_index++;
  out_stream = avformat_new_stream(output_format_context, NULL);
  if (!out_stream) {
    fprintf(stderr, "Failed allocating output stream\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }
  ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
  if (ret < 0) {
    fprintf(stderr, "Failed to copy codec parameters\n");
    goto end;
  }
}
```

Bây giờ chúng ta có thể tạo tập tin đầu ra.

```c
if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
  ret = avio_open(&output_format_context->pb, out_filename, AVIO_FLAG_WRITE);
  if (ret < 0) {
    fprintf(stderr, "Could not open output file '%s'", out_filename);
    goto end;
  }
}

ret = avformat_write_header(output_format_context, NULL);
if (ret < 0) {
  fprintf(stderr, "Error occurred when opening output file\n");
  goto end;
}
```

Sau đó, chúng ta có thể sao chép các luồng stream, từng gói dữ liệu packet, từ luồng đầu vào đến luồng đầu ra. Thực hiện vòng lặp khi có gói dự liệu (`av_read_frame`), với mỗi gói dữ liệu, chúng ta cần tính lại PTS và DTS để kết thúc ghi nó lại (`av_interleaved_write_frame`) tới bộ định dạng đầu ra.

```c
while (1) {
  AVStream *in_stream, *out_stream;
  ret = av_read_frame(input_format_context, &packet);
  if (ret < 0)
    break;
  in_stream  = input_format_context->streams[packet.stream_index];
  if (packet.stream_index >= number_of_streams || streams_list[packet.stream_index] < 0) {
    av_packet_unref(&packet);
    continue;
  }
  packet.stream_index = streams_list[packet.stream_index];
  out_stream = output_format_context->streams[packet.stream_index];
  /* copy packet */
  packet.pts = av_rescale_q_rnd(packet.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.dts = av_rescale_q_rnd(packet.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
  packet.duration = av_rescale_q(packet.duration, in_stream->time_base, out_stream->time_base);
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html#ab5793d8195cf4789dfb3913b7a693903
  packet.pos = -1;

  //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
  ret = av_interleaved_write_frame(output_format_context, &packet);
  if (ret < 0) {
    fprintf(stderr, "Error muxing packet\n");
    break;
  }
  av_packet_unref(&packet);
}
```

Để kết thúc chúng ta cần viết phần kết thúc luồng stream tới tập tin đầu ra với hàm [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13)

```c
av_write_trailer(output_format_context);
```

Từ giờ chúng ta đã sẵn sàng để kiểm tra nó và bài kiểm tra đầu tiên sẽ là chuyển đổi định dạng video từ MP4 sang MPEG-TS. Chúng ta cơ bản thực hiện lệnh  `ffmpeg input.mp4 -c copy output.ts` với thư viện libav.

```bash
make run_remuxing_ts
```

Nó đã làm việc!!! Bạn không tin tôi ư?! Để chắc chắn, chúng ta có thể kiểm tra nó với `ffprobe`

```bash
ffprobe -i remuxed_small_bunny_1080p_60fps.ts

Input #0, mpegts, from 'remuxed_small_bunny_1080p_60fps.ts':
  Duration: 00:00:10.03, start: 0.000000, bitrate: 2751 kb/s
  Program 1
    Metadata:
      service_name    : Service01
      service_provider: FFmpeg
    Stream #0:0[0x100]: Video: h264 (High) ([27][0][0][0] / 0x001B), yuv420p(progressive), 1920x1080 [SAR 1:1 DAR 16:9], 60 fps, 60 tbr, 90k tbn, 120 tbc
    Stream #0:1[0x101]: Audio: ac3 ([129][0][0][0] / 0x0081), 48000 Hz, 5.1(side), fltp, 320 kb/s
```

Tổng hợp lại những gì chúng ta đã làm theo sơ đồ, chúng ta xem lại bài mở đầu [ý tưởng libav hoạt động](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-libav-architecture) và như đã thấy chúng ta bỏ qua phần mã hoá.

![remuxing libav components](/img/remuxing_libav_components.png)

Trước khi kết thúc chương này, tôi muốn chỉ ra phần quan trọng nhất của tiến trình remuxing, **bạn có thể đưa các tuỳ chọn vào bộ muxer**. Hãy nói chúng ta muốn chuyển định dạng [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding), để giải quyết vấn đề này, chúng ta cần sử dụng định dạng [fragmented mp4](https://stackoverflow.com/a/35180327) (thỉnh thoảng được giới thiệu như `fmp4`) thay vì MPEF-TS hoặc thuần MPEG-4.

Với [việc thực hiện dễ dàng bằng câu lệnh](https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE#Fragmenting).

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

Dùng libav cũng dễ dàng như câu lệnh, chúng ta cần đưa các tuỳ chọn khi ghi header đầu ra, trước khi sao chép các gói dữ liệu packet.

```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
```

Chúng ta có thể tạo tập tin fragmented mp4:

```bash
make run_remuxing_fragmented_mp4
```

Nhưng để chắc chắn rằng tôi không nói dối, bạn có thể sử dụng một công cụ tiện ích [gpac/mp4box.js](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js/filereader.html) hoặc [http://mp4parser.com/](http://mp4parser.com/) để nhìn sự khác biệt, đầu tiên tải lên tập tin mp4 "thông thường"

![mp4 boxes](/img/boxes_normal_mp4.png)

Như bạn thấy, nó chỉ có duy nhất một box `mdat`, **nơi chứa khung hình video và audio**. Giờ hãy tải lên tệp tin định dạng fragmented mp4 để nhìn chúng được chia thành nhiều box `mdat`

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## Chapter 3 - Transcoding

> #### TLDR; chỉ ra bộ mã hoá [code](/3_transcoding.c) và thực thi.
> ```bash
> $ make run_transcoding
> ```
> Chúng ta sẽ bỏ qua chi tiết, nhưng đùng lo lắng: [source code có sẵn trên github](/3_transcoding.c).


Ở chương này, chúng ta sẽ tạo một bộ chuyển đổi chuẩn nén transcoder tối giản nhất, viết bằng ngôn ngữ C, có thể chuyển đổi video từ chuẩn nén H264 thành H265 bằng thư viện **FFmpeg/libav**, cụ thể là  [libavcodec](https://ffmpeg.org/libavcodec.html), libavformat, và libavutil.

![media transcoding flow](/img/transcoding_flow.png)

> _Tóm tắt nhanh:_ [**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html) là sự trừu tượng cho các định dạng tệp tin đa phương tiện, hay còn gọi là container (ví dự: MKV, MP4, Webm, TS). [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html) đại diện mỗi loại dữ liệu của định dạng đã cho (ví dụ: audio, video, subtitle, metadata). [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html) là một phần của dữ liệu đã nén chứa trong `AVStream`, nó có thể được giải mã bới [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html) (ví dụ: av1, h264, vp9, hevc), tạo ra dữ liệu gốc gọi là [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html).

### Transmuxing

Hãy bắt đầu với sự hoạt động transmuxing đơn giản và sau đó chúng ta có thể xây dựng dựa trên code đó, bước đầu tiên là **tải tập tin đầu vào**. 

```c
// Allocate an AVFormatContext
avfc = avformat_alloc_context();
// Open an input stream and read the header.
avformat_open_input(avfc, in_filename, NULL, NULL);
// Read packets of a media file to get stream information.
avformat_find_stream_info(avfc, NULL);
```

Chúng ta sẽ cài đặt một bộ giải mã, `AVFormatContext` sẽ cho phép chúng ta truy cập tất cả thành phần `AVStream` và mỗi thành phần trong số chúng, chúng ta có thể nhận `AVCodec` và tạo `AVCodecContext` chi tiết và cuối cùng chúng ta có thể mở codec nhận được, do đó chúng ta có thể thực hiện quá trình giải mã.  

>  Thành phần [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html) giữ những dữ liệu về cấu hình đa phương tiện như tốc độ bit, tốc độ khung hình, tốc độ mẫu, các kênh, chiều cao và rất nhiều thứ khác nữa.

```c
for (int i = 0; i < avfc->nb_streams; i++)
{
  AVStream *avs = avfc->streams[i];
  AVCodec *avc = avcodec_find_decoder(avs->codecpar->codec_id);
  AVCodecContext *avcc = avcodec_alloc_context3(*avc);
  avcodec_parameters_to_context(*avcc, avs->codecpar);
  avcodec_open2(*avcc, *avc, NULL);
}
```

Chúng ta cần chuẩn bị tập tin đầu ra cho việc transmuxing, đầu tiên chúng ta **khởi tạo vùng nhớ** cho `AVFormatContext` đầu ra. Chúng ta tạo **từng luồng stream** cho định dạng đầu ra. Để đóng gói luồng thích hợp, chúng ta **sao chép các thông số codec** từ bộ giải mã.

Chúng ta **bật cờ** `AV_CODEC_FLAG_GLOBAL_HEADER` để nói cho bộ mã hoá rằng nó có thể sử dụng global header và cuối cùng chúng ta mở **tập tin để ghi** đầu ra và giữ header.

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
  encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);

```

Chúng ta nhận `AVPacket` từ bộ giải mã, điều chỉnh timestamp, và ghi gói dữ liệu packet thích hợp với tập tin đầu ra. Mặc dù hàm `av_interleaved_write_frame` nói "ghi khung hình", nhưng chúng ta đang lưu các gói packet. Chúng ta kết thúc quá trình transmuxing bằng cách ghi phần đuôi (trailer) vào tập tin.

```c
AVFrame *input_frame = av_frame_alloc();
AVPacket *input_packet = av_packet_alloc();

while (av_read_frame(decoder_avfc, input_packet) >= 0)
{
  av_packet_rescale_ts(input_packet, decoder_video_avs->time_base, encoder_video_avs->time_base);
  av_interleaved_write_frame(*avfc, input_packet) < 0));
}

av_write_trailer(encoder_avfc);
```

### Transcoding

Phần trước đã đưa ra chương trình transmuxer đơn giản, bây giờ chúng ta sẽ thêm vào khả năng cho tập tin mã hoá, đặc biệt chúng ta sẽ thực hiện transcode video từ `h264` sang `h265`

Sau khi chúng ta chuẩn bị bộ giải mã, trước khi chúng ta sắp xếp các tập tin đầu ra, chúng ta sẽ cài đặt bộ mã hoá.

* Tạo video `AVStream` trong bộ mã hoá, [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827)
* Sử dụng `AVCodec` là `libx265`, [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37)
* Tạo `AVCodecContext` dựa vào codec được tạo, [`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315)
* Cài đặt thuộc tính cơ sở cho phiên transcoding, và
* Mở codec và sao chép thông số từ context tới luồng stream. [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) và [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe)

```c
AVRational input_framerate = av_guess_frame_rate(decoder_avfc, decoder_video_avs, NULL);
AVStream *video_avs = avformat_new_stream(encoder_avfc, NULL);

char *codec_name = "libx265";
char *codec_priv_key = "x265-params";
// we're going to use internal options for the x265
// it disables the scene change detection and fix then
// GOP on 60 frames.
char *codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

AVCodec *video_avc = avcodec_find_encoder_by_name(codec_name);
AVCodecContext *video_avcc = avcodec_alloc_context3(video_avc);
// encoder codec params
av_opt_set(sc->video_avcc->priv_data, codec_priv_key, codec_priv_value, 0);
video_avcc->height = decoder_ctx->height;
video_avcc->width = decoder_ctx->width;
video_avcc->pix_fmt = video_avc->pix_fmts[0];
// control rate
video_avcc->bit_rate = 2 * 1000 * 1000;
video_avcc->rc_buffer_size = 4 * 1000 * 1000;
video_avcc->rc_max_rate = 2 * 1000 * 1000;
video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
// time base
video_avcc->time_base = av_inv_q(input_framerate);
video_avs->time_base = sc->video_avcc->time_base;

avcodec_open2(sc->video_avcc, sc->video_avc, NULL);
avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
```

Chúng ta cần mở rộng vòng lặp giải mã cho việc trancoding luồng video:

* Gửi `AVPacket` rỗng tới bộ giải mã, [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
* Nhận `AVFrame` đã giải nén, [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
* Bắt đầu transcode khung hình gốc này,
* Gửi khung hình gốc, [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169)
* Nhận dữ liệu nén lại dựa trên codec, `AVPacket`, [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5b8eff59cf259747cf0b31563e38ded6)
* Cài đặt timestamp, và [`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
* Ghi nó vào tập tin đầu ra. [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

```c
AVFrame *input_frame = av_frame_alloc();
AVPacket *input_packet = av_packet_alloc();

while (av_read_frame(decoder_avfc, input_packet) >= 0)
{
  int response = avcodec_send_packet(decoder_video_avcc, input_packet);
  while (response >= 0) {
    response = avcodec_receive_frame(decoder_video_avcc, input_frame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      return response;
    }
    if (response >= 0) {
      encode(encoder_avfc, decoder_video_avs, encoder_video_avs, decoder_video_avcc, input_packet->stream_index);
    }
    av_frame_unref(input_frame);
  }
  av_packet_unref(input_packet);
}
av_write_trailer(encoder_avfc);

// used function
int encode(AVFormatContext *avfc, AVStream *dec_video_avs, AVStream *enc_video_avs, AVCodecContext video_avcc int index) {
  AVPacket *output_packet = av_packet_alloc();
  int response = avcodec_send_frame(video_avcc, input_frame);

  while (response >= 0) {
    response = avcodec_receive_packet(video_avcc, output_packet);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      return -1;
    }

    output_packet->stream_index = index;
    output_packet->duration = enc_video_avs->time_base.den / enc_video_avs->time_base.num / dec_video_avs->avg_frame_rate.num * dec_video_avs->avg_frame_rate.den;

    av_packet_rescale_ts(output_packet, dec_video_avs->time_base, enc_video_avs->time_base);
    response = av_interleaved_write_frame(avfc, output_packet);
  }
  av_packet_unref(output_packet);
  av_packet_free(&output_packet);
  return 0;
}

```

Chúng ta chuyển đổi luồng media từ `h264` tới `h265`, như phiên bản `h265` mong đợi của tập tin media sẽ có kích thước nhỏ hơn `h264` tuy nhiên [chương trình được tạo](/3_transcoding.c) có khả năng:

```c

  /*
   * H264 -> H265
   * Audio -> remuxed (untouched)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx265";
  sp.codec_priv_key = "x265-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> remuxed (untouched)
   * MP4 - MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> remuxed (untouched)
   * MP4 - fragmented MP4
   */
  StreamingParams sp = {0};
  sp.copy_audio = 1;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
  sp.muxer_opt_key = "movflags";
  sp.muxer_opt_value = "frag_keyframe+empty_moov+delay_moov+default_base_moof";

  /*
   * H264 -> H264 (fixed gop)
   * Audio -> AAC
   * MP4 - MPEG-TS
   */
  StreamingParams sp = {0};
  sp.copy_audio = 0;
  sp.copy_video = 0;
  sp.video_codec = "libx264";
  sp.codec_priv_key = "x264-params";
  sp.codec_priv_value = "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
  sp.audio_codec = "aac";
  sp.output_extension = ".ts";

  /* WIP :P  -> it's not playing on VLC, the final bit rate is huge
   * H264 -> VP9
   * Audio -> Vorbis
   * MP4 - WebM
   */
  //StreamingParams sp = {0};
  //sp.copy_audio = 0;
  //sp.copy_video = 0;
  //sp.video_codec = "libvpx-vp9";
  //sp.audio_codec = "libvorbis";
  //sp.output_extension = ".webm";

```

> Bây giờ, thânh thật mà nói, điều này [khó hơn tôi nghĩ](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54) nó là như vậy và tôi phải đào sâu hơn [source code câu lệnh FFmpeg](https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749) và kiểm tra nó rất nhiều và tôi nghĩ tôi đang bỏ quên một số thứ bởi vì tôi phải thực hiện `force-cfr` cho `h264` để làm việc và tôi vẫn xem một số tin nhắn cảnh báo như `warning messages (forced frame type (5) at 80 was changed to frame type (3))`.
