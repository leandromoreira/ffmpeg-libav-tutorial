[![license](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)](https://img.shields.io/badge/license-BSD--3--Clause-blue.svg)

Eu estava procurando por um tutorial/livro que me ensinasse a começar a usar o [FFmpeg](https://www.ffmpeg.org/) como biblioteca (também conhecida como libav) e então encontrei o tutorial ["Como escrever um player de vídeo em menos de 1k linhas"](http://dranger.com/ffmpeg/).
Infelizmente, ele foi descontinuado, então decidi escrever este.

A maior parte do código aqui será em C **mas não se preocupe**: você pode facilmente entender e aplicá-lo à sua linguagem preferida.
O FFmpeg libav tem muitas ligações para várias linguagens, como [python](https://pyav.org/), [go](https://github.com/imkira/go-libav) e mesmo que sua linguagem não tenha, ainda é possível suportá-la através do `ffi` (aqui está um exemplo com [Lua](https://github.com/daurnimator/ffmpeg-lua-ffi/blob/master/init.lua)).

Começaremos com uma breve lição sobre o que é vídeo, áudio, codec e contêiner e depois faremos um curso intensivo sobre como usar a linha de comando do `FFmpeg` e, finalmente, escreveremos código. Sinta-se à vontade para pular diretamente para a seção [Aprenda o FFmpeg libav do jeito difícil.](#aprenda-o-ffmpeg-libav-do-modo-difícil)

Algumas pessoas costumavam dizer que o streaming de vídeo na internet é o futuro da TV tradicional, de qualquer forma, o FFmpeg é algo que vale a pena estudar.

__Índice__

* [Introdução](#introdução)
	* [Vídeo - O que você vê!](#vídeo---o-que-você-vê)
	* [Áudio - O que você ouve!](#áudio---o-que-você-ouve)
	* [Codec - reduzindo dados](#codec---reduzindo-dados)
	* [Container - um lugar confortável para áudio e vídeo](#container---um-lugar-confortável-para-áudio-e-vídeo)
* [FFmpeg - linha de comando](#ffmpeg---linha-de-comando)
	* [Ferramenta de linha de comando do FFmpeg 101](#ferramenta-de-linha-de-comando-do-ffmpeg-101)
* [Operações comuns de vídeo](#operações-comuns-de-vídeo)
	* [Transcodificação](#transcodificação)
	* [Transmuxing](#transmuxing)
	* [Transcodificação de Taxa de Bits](#transcodificação-de-taxa-de-bits)
	* [Transdimensionamento](#transdimensionamento)
	* [Bônus: Streaming Adaptativo](#bônus-streaming-adaptativo)
	* [Indo além](#indo-além)
* [Aprenda o FFmpeg libav do modo difícil](#aprenda-o-ffmpeg-libav-do-modo-difícil)
  	* [Capítulo 0 - O famoso "hello world"](#capítulo-0---o-famoso-hello-world)
		* [Arquitetura da biblioteca FFmpeg libav](#arquitetura-da-biblioteca-ffmpeg-libav)
	* [Capítulo 1 - Sincronização de áudio e vídeo](#capítulo-1---sincronização-de-áudio-e-vídeo)
	* [Capítulo 2 - Remuxing](#capítulo-2---remuxing)
	* [Capítulo 3 - Transcoding](#capítulo-3---transcoding)

# Introdução

## Vídeo - O que você vê!

Se você tem uma sequência de imagens e as altera com uma determinada frequência (digamos [24 imagens por segundo](https://www.filmindependent.org/blog/hacking-film-24-frames-per-second/)), você criará uma [ilusão de movimento](https://en.wikipedia.org/wiki/Persistence_of_vision).
Em resumo, essa é a ideia básica por trás de um vídeo: **uma série de imagens/quadros sendo executados a uma determinada taxa**.

<img src="https://upload.wikimedia.org/wikipedia/commons/1/1f/Linnet_kineograph_1886.jpg" title="flip book" height="280"></img>

Zeitgenössische Illustration (1886)

## Áudio - O que você ouve!

Embora um vídeo sem som possa expressar uma variedade de sentimentos, adicionar som a ele traz mais prazer à experiência.

O som é a vibração que se propaga como uma onda de pressão, através do ar ou qualquer outro meio de transmissão, como um gás, líquido ou sólido.

> Em um sistema de áudio digital, um microfone converte o som em um sinal elétrico analógico, em seguida, um conversor analógico-digital (ADC) - tipicamente usando [modulação por código de pulso (PCM)](https://en.wikipedia.org/wiki/Pulse-code_modulation) - converte o sinal analógico em um sinal digital.

![audio analog to digital](https://upload.wikimedia.org/wikipedia/commons/thumb/c/c7/CPT-Sound-ADC-DAC.svg/640px-CPT-Sound-ADC-DAC.svg.png "audio analog to digital")
>[Fonte](https://commons.wikimedia.org/wiki/File:CPT-Sound-ADC-DAC.svg)

## Codec - reduzindo dados

> CODEC é um circuito eletrônico ou software que **comprime ou descomprime áudio/vídeo digital.** Ele converte áudio/vídeo digital bruto (não comprimido) para um formato comprimido ou vice-versa.
> https://en.wikipedia.org/wiki/Video_codec

Mas se escolhermos empacotar milhões de imagens em um único arquivo e chamá-lo de filme, podemos acabar com um arquivo enorme. Vamos fazer as contas:

Suponha que estamos criando um vídeo com resolução `1080 x 1920` (altura x largura) e que gastaremos `3 bytes` por pixel (o ponto mínimo em uma tela) para codificar a cor (ou [cor de 24 bits](https://en.wikipedia.org/wiki/Color_depth#True_color_.2824-bit.29), o que nos dá 16.777.216 cores diferentes) e este vídeo é executado a `24 quadros por segundo` e tem `30 minutos` de duração.

```c
toppf = 1080 * 1920 // total_de_pixels_por_quadro
cpp = 3 //custo_por_pixel
tis = 30 * 60 //tempo_em_segundos
fps = 24 //quadros_por_segundo

armazenamento_necessário = tis * fps * toppf * cpp
```

Este vídeo exigiria aproximadamente `250,28 GB` de armazenamento ou `1,19 Gbps` de largura de banda! É por isso que precisamos usar um [CODEC](https://github.com/leandromoreira/digital_video_introduction#how-does-a-video-codec-work).

## container - um lugar confortável para áudio e vídeo

> Um formato de container ou envoltório é um formato de metafile cuja especificação descreve como diferentes elementos de dados e metadados coexistem em um arquivo de computador.
> https://en.wikipedia.org/wiki/Digital_container_format

Um **único arquivo que contém todos os fluxos** (principalmente áudio e vídeo) e também fornece **sincronização e metadados gerais**, como título, resolução, entre outros.

Normalmente, podemos inferir o formato de um arquivo ao olhar para sua extensão: por exemplo, um `video.webm` provavelmente é um vídeo usando o container [`webm`](https://www.webmproject.org/).

![container](/img/container.png)

# FFmpeg - linha de comando

> Uma solução completa e multiplataforma para gravar, converter e transmitir áudio e vídeo.

Para trabalhar com multimídia, podemos usar a FERRAMENTA/BIBLIOTECA incrível chamada [FFmpeg](https://www.ffmpeg.org/). Provavelmente, você já a conhece/usa diretamente ou indiretamente (você usa o [Chrome?](https://www.chromium.org/developers/design-documents/video)).

Ele tem um programa de linha de comando chamado `ffmpeg`, um binário muito simples, porém poderoso.
Por exemplo, você pode converter de `mp4` para o contêiner `avi` apenas digitando o seguinte comando:

```bash
$ ffmpeg -i input.mp4 output.avi
```

Acabamos de fazer um **remuxing** aqui, que é converter de um contêiner para outro.
Tecnicamente, o FFmpeg também poderia estar fazendo uma transcodificação, mas falaremos sobre isso mais tarde.

## Ferramenta de linha de comando do FFmpeg 101

O FFmpeg possui uma [documentação](https://www.ffmpeg.org/ffmpeg.html) que faz um ótimo trabalho explicando como ele funciona.

```bash
# você também pode procurar a documentação usando a linha de comando

ffmpeg -h full | grep -A 10 -B 10 avoid_negative_ts
```

Resumidamente, o programa de linha de comando do FFmpeg espera o seguinte formato de argumento para executar suas ações: `ffmpeg {1} {2} -i {3} {4} {5}`, onde:

1. opções globais
2. opções do arquivo de entrada
3. URL de entrada
4. opções do arquivo de saída
5. URL de saída

As partes 2, 3, 4 e 5 podem ser quantas você precisar.
É mais fácil entender esse formato de argumento na prática:

```bash
# ATENÇÃO: este arquivo tem cerca de 300MB
$ wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4

$ ffmpeg \
-y \ # opções globais
-c:a libfdk_aac \ # opções de entrada
-i bunny_1080p_60fps.mp4 \ # URL de entrada
-c:v libvpx-vp9 -c:a libvorbis \ # opções de saída
bunny_1080p_60fps_vp9.webm # URL de saída
```
Este comando leva um arquivo de entrada `mp4` contendo dois fluxos (um áudio codificado com `aac` CODEC e um vídeo codificado usando `h264` CODEC) e o converte para `webm`, mudando seus CODECs de áudio e vídeo também.

Podemos simplificar o comando acima, mas esteja ciente de que o FFmpeg adotará ou adivinhará os valores padrão para você.
Por exemplo, quando você apenas digita `ffmpeg -i input.avi output.mp4`, que CODEC de áudio/vídeo ele usa para produzir o `output.mp4`?

Werner Robitza escreveu um [tutorial obrigatório para ler/executar sobre codificação e edição com FFmpeg](http://slhck.info/ffmpeg-encoding-course/#/).

# Operações comuns de vídeo

Ao trabalhar com áudio/vídeo, geralmente realizamos um conjunto de tarefas com a mídia.

## Transcodificação

![transcodificação](/img/transcoding.png)

**O que é?** É o ato de converter um dos fluxos (áudio ou vídeo) de um CODEC para outro.

**Por que?** Às vezes, alguns dispositivos (TVs, smartphones, consoles etc.) não suportam X, mas sim Y, e os novos CODECs fornecem melhor taxa de compressão.

**Como?** Convertendo um vídeo `H264` (AVC) para `H265` (HEVC).
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c:v libx265 \
bunny_1080p_60fps_h265.mp4
```

## Transmuxing

![transmuxing](/img/transmuxing.png)

**O que é?** É o ato de converter de um formato (container) para outro.

**Por que?** Às vezes, alguns dispositivos (TVs, smartphones, consoles, etc.) não suportam o formato X, mas suportam o Y e, às vezes, os novos formatos (containers) fornecem recursos modernos necessários.

**Como?** Converter um arquivo `mp4` para `ts`.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-c copy \ # just saying to ffmpeg to skip encoding
bunny_1080p_60fps.ts
```

## Transcodificação de Taxa de Bits

![transrating](/img/transrating.png)

**O que é?** É a alteração da taxa de bits de um vídeo, ou a produção de outras versões do mesmo vídeo.

**Por que fazer?** As pessoas podem tentar assistir ao seu vídeo em uma conexão de rede `2G` (edge) usando um smartphone menos potente ou em uma conexão de fibra óptica em suas TVs 4K. Portanto, você deve oferecer mais de uma versão do mesmo vídeo com diferentes taxas de bits.

**Como fazer?** Produzindo uma versão com taxa de bits entre 3856K e 2000K.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-minrate 964K -maxrate 3856K -bufsize 2000K \
bunny_1080p_60fps_transrating_964_3856.mp4
```

Geralmente, a transcodificação de taxa de bits é usada em conjunto com a transcodificação de tamanho de vídeo. Werner Robitza escreveu outra série de posts que deve ser lida/executada sobre o controle de taxa do FFmpeg (http://slhck.info/posts/).

## Transdimensionamento

![transsizing](/img/transsizing.png)

**O que é?** a ação de converter de uma resolução para outra. Como mencionado antes, o transdimensionamento é frequentemente usado junto com o transrating.

**Por quê?** as razões são as mesmas que para o transrating.

**Como?** convertendo uma resolução `1080p` para `480p`.
```bash
$ ffmpeg \
-i bunny_1080p_60fps.mp4 \
-vf scale=480:-1 \
bunny_1080p_60fps_transsizing_480.mp4
```

## Bônus: Streaming Adaptativo

![Streaming adaptativo](/img/adaptive-streaming.png)

**O que é?** A produção de várias resoluções (taxas de bits) e a divisão da mídia em pedaços para serem servidos por HTTP.

**Por que?** Para fornecer uma mídia flexível que possa ser assistida em um smartphone de baixo desempenho ou em uma TV 4K, além de ser fácil de dimensionar e implantar, mas pode adicionar latência.

**Como?** Criando um WebM adaptativo usando o DASH.
```bash
# video streams
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 160x90 -b:v 250k -keyint_min 150 -g 150 -an -f webm -dash 1 video_160x90_250k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 320x180 -b:v 500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_320x180_500k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 750k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_750k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 640x360 -b:v 1000k -keyint_min 150 -g 150 -an -f webm -dash 1 video_640x360_1000k.webm

$ ffmpeg -i bunny_1080p_60fps.mp4 -c:v libvpx-vp9 -s 1280x720 -b:v 1500k -keyint_min 150 -g 150 -an -f webm -dash 1 video_1280x720_1500k.webm

# audio streams
$ ffmpeg -i bunny_1080p_60fps.mp4 -c:a libvorbis -b:a 128k -vn -f webm -dash 1 audio_128k.webm

# the DASH manifest
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

PS: Eu roubei esse exemplo das [Instruções para reproduzir Adaptive WebM usando DASH](http://wiki.webmproject.org/adaptive-streaming/instructions-to-playback-adaptive-webm-using-dash)

## Indo além

Existem [muitos outros usos para o FFmpeg](https://github.com/leandromoreira/digital_video_introduction/blob/master/encoding_pratical_examples.md#split-and-merge-smoothly).
Eu uso em conjunto com o *iMovie* para produzir/editar alguns vídeos para o YouTube e certamente você pode usá-lo profissionalmente.

# Aprenda o FFmpeg libav do modo difícil

> Você nunca se perguntou sobre som e visão?
> **David Robert Jones**

Já que o [FFmpeg](#ffmpeg---linha-de-comando) é tão útil como uma ferramenta de linha de comando para realizar tarefas essenciais em arquivos de mídia, como podemos usá-lo em nossos programas?

O FFmpeg é [composto por diversas bibliotecas](https://www.ffmpeg.org/doxygen/trunk/index.html) que podem ser integradas em nossos próprios programas. Geralmente, quando você instala o FFmpeg, ele instala automaticamente todas essas bibliotecas. Estarei me referindo a esse conjunto de bibliotecas como **FFmpeg libav**.

> Este título é uma homenagem à série de Zed Shaw [Aprenda X do Modo Difícil](https://learncodethehardway.org/), em particular seu livro Aprenda C do Modo Difícil.

## Capítulo 0 - O famoso "hello world"
Este "hello world" na verdade não mostrará a mensagem "hello world" no terminal :tongue:
Em vez disso, vamos **imprimir informações sobre o vídeo**, como seu formato (container), duração, resolução, canais de áudio e, no final, vamos **decodificar alguns quadros e salvá-los como arquivos de imagem**.

### Arquitetura da biblioteca FFmpeg libav

Mas antes de começarmos a programar, vamos aprender como funciona a **arquitetura da biblioteca FFmpeg libav** e como seus componentes se comunicam entre si.

Aqui está um diagrama do processo de decodificação de um vídeo:

![ffmpeg libav architecture - processo de decodificação](/img/decoding.png)

Você primeiro precisará carregar seu arquivo de mídia em um componente chamado [`AVFormatContext`](https://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) (o contêiner de vídeo também é conhecido como formato).
Na verdade, ele não carrega todo o arquivo: muitas vezes ele lê apenas o cabeçalho.

Depois de carregar o **cabeçalho mínimo do nosso contêiner**, podemos acessar suas streams (pense nelas como dados de áudio e vídeo rudimentares).
Cada stream estará disponível em um componente chamado [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html).

> Stream é um nome elegante para um fluxo contínuo de dados.

Suponha que nosso vídeo tenha duas streams: um áudio codificado com [AAC CODEC](https://en.wikipedia.org/wiki/Advanced_Audio_Coding) e um vídeo codificado com [H264 (AVC) CODEC](https://en.wikipedia.org/wiki/H.264/MPEG-4_AVC). De cada stream podemos extrair **pedaços (slices) de dados** chamados pacotes que serão carregados em componentes chamados [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html).

Os **dados dentro dos pacotes ainda estão codificados** (comprimidos) e, para decodificar os pacotes, precisamos passá-los para um [`AVCodec`](https://ffmpeg.org/doxygen/trunk/structAVCodec.html) específico.

O `AVCodec` os decodificará em [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html) e, finalmente, este componente nos fornecerá **o quadro não comprimido**. Observe que a mesma terminologia/processo é usada tanto para fluxo de áudio quanto de vídeo.

### Requisitos

Como algumas pessoas estavam [enfrentando problemas ao compilar ou executar os exemplos](https://github.com/leandromoreira/ffmpeg-libav-tutorial/issues?utf8=%E2%9C%93&q=is%3Aissue+is%3Aopen+compiling), **vamos usar o [`Docker`](https://docs.docker.com/install/) como nosso ambiente de desenvolvimento/execução**, também usaremos o vídeo Big Buck Bunny, então se você não o tiver localmente, basta executar o comando `make fetch_small_bunny_video`.

### Capítulo 0 - apresentação do código

> #### TLDR; mostre-me o [código](/0_hello_world.c) e a execução.
> ```bash
> $ make run_hello
> ```

Vamos pular alguns detalhes, mas não se preocupe: o [código-fonte está disponível no GitHub](/0_hello_world.c).

Vamos alocar memória para o componente [`AVFormatContext`](http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html) que conterá informações sobre o formato (container).

```c
AVFormatContext *pFormatContext = avformat_alloc_context();
```

Agora vamos abrir o arquivo e ler seu cabeçalho e preencher o `AVFormatContext` com informações mínimas sobre o formato (observe que geralmente os codecs não são abertos).
A função usada para isso é [`avformat_open_input`](http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49). Ele espera um `AVFormatContext`, um `filename` e dois argumentos opcionais: o [`AVInputFormat`](https://ffmpeg.org/doxygen/trunk/structAVInputFormat.html) (se você passar `NULL`, o FFmpeg adivinhará o formato) e o [`AVDictionary`](https://ffmpeg.org/doxygen/trunk/structAVDictionary.html) (que são as opções para o demuxer).

```c
avformat_open_input(&pFormatContext, filename, NULL, NULL);
```

Podemos imprimir o nome do formato e a duração da mídia:

```c
printf("Format %s, duration %lld us", pFormatContext->iformat->long_name, pFormatContext->duration);
```

Para acessar as `streams`, precisamos ler os dados da mídia. A função [`avformat_find_stream_info`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb) faz isso. Agora, o `pFormatContext->nb_streams` irá armazenar a quantidade de streams e o `pFormatContext->streams[i]` nos fornecerá a `i`-ésima stream (um [`AVStream`](https://ffmpeg.org/doxygen/trunk/structAVStream.html)).

```c
avformat_find_stream_info(pFormatContext,  NULL);
```

Agora vamos iterar por todos os fluxos (streams).

```c
for (int i = 0; i < pFormatContext->nb_streams; i++)
{
	//
}
```

Para cada fluxo, vamos manter os [`AVCodecParameters`](https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html), que descreve as propriedades de um codec usado pelo fluxo `i`.

```c
AVCodecParameters *pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
```

Com as propriedades do codec, podemos procurar o CODEC apropriado consultando a função [`avcodec_find_decoder`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca) e encontrar o decodificador registrado para o id do codec e retornar um [`AVCodec`](http://ffmpeg.org/doxygen/trunk/structAVCodec.html), o componente que sabe como en**CO**der e de**CO**der o fluxo.
```c
AVCodec *pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
```

Agora podemos imprimir informações sobre os codecs.

```c
// especifico para video e audio
if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
	printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
	printf("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
}
// geral
printf("\tCodec %s ID %d bit_rate %lld", pLocalCodec->long_name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
```

Com o codec, podemos alocar memória para o [`AVCodecContext`](https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html), que conterá o contexto para nosso processo de decodificação/ codificação, mas precisamos preencher este contexto do codec com os parâmetros do CODEC; fazemos isso com [`avcodec_parameters_to_context`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16).

Uma vez preenchido o contexto do codec, precisamos abrir o codec. Chamamos a função [`avcodec_open2`](https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) e, em seguida, podemos usá-lo.

```c
AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
avcodec_parameters_to_context(pCodecContext, pCodecParameters);
avcodec_open2(pCodecContext, pCodec, NULL);
```

Agora vamos ler os pacotes do fluxo e decodificá-los em quadros, mas antes disso, precisamos alocar memória para ambos os componentes, o [`AVPacket`](https://ffmpeg.org/doxygen/trunk/structAVPacket.html) e [`AVFrame`](https://ffmpeg.org/doxygen/trunk/structAVFrame.html).

```c
AVPacket *pPacket = av_packet_alloc();
AVFrame *pFrame = av_frame_alloc();
```

Vamos alimentar nossos pacotes das streams com a função [`av_read_frame`](https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61) enquanto houver pacotes.


```c
while (av_read_frame(pFormatContext, pPacket) >= 0) {
	//...
}
```

Vamos **enviar o pacote de dados bruto** (quadro comprimido) para o decodificador, por meio do contexto do codec, usando a função [`avcodec_send_packet`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3).

```c
avcodec_send_packet(pCodecContext, pPacket);
```

E vamos **receber o quadro de dados bruto** (quadro descomprimido) do decodificador, através do mesmo contexto do codec, usando a função [`avcodec_receive_frame`](https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c).

```c
avcodec_receive_frame(pCodecContext, pFrame);
```

Podemos imprimir o número do quadro, o [PTS](https://en.wikipedia.org/wiki/Presentation_timestamp), DTS, [tipo de quadro](https://en.wikipedia.org/wiki/Video_compression_picture_types) e etc.

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

Finalmente, podemos salvar nosso quadro decodificado em uma [imagem simples em tons de cinza](https://en.wikipedia.org/wiki/Netpbm_format#PGM_example). O processo é muito simples, usaremos `pFrame->data`, onde o índice está relacionado aos [planos Y, Cb e Cr](https://en.wikipedia.org/wiki/YCbCr), escolhemos apenas `0` (Y) para salvar nossa imagem em tons de cinza.

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

E voilà! Agora temos uma imagem em escala de cinza com 2MB:

![saved frame](/img/generated_frame.png)

## Capítulo 1 - sincronização de áudio e vídeo

> **Seja o player** - um jovem desenvolvedor de JS criando um novo player de vídeo MSE.

Antes de avançarmos para [codificar um exemplo de transcodificação](#capítulo-2---remuxing), vamos falar sobre **tempo**, ou como um player de vídeo sabe a hora certa de exibir um quadro.

No último exemplo, salvamos alguns quadros que podem ser vistos aqui:

![frame 0](/img/hello_world_frames/frame0.png)
![frame 1](/img/hello_world_frames/frame1.png)
![frame 2](/img/hello_world_frames/frame2.png)
![frame 3](/img/hello_world_frames/frame3.png)
![frame 4](/img/hello_world_frames/frame4.png)
![frame 5](/img/hello_world_frames/frame5.png)

Quando estamos projetando um player de vídeo, precisamos **reproduzir cada quadro em um ritmo definido**, caso contrário, seria difícil visualizar o vídeo de forma agradável, seja porque está reproduzindo muito rápido ou muito devagar.

Portanto, precisamos introduzir alguma lógica para reproduzir cada quadro suavemente. Para esse fim, cada quadro tem um **carimbo de tempo de apresentação** (PTS), que é um número crescente multiplicado por uma **base de tempo** que é um número racional (onde o denominador é conhecido como **timescale**) divisível pela **taxa de quadros (fps)**.

É mais fácil entender quando olhamos alguns exemplos, vamos simular alguns cenários.

Para um `fps=60/1` e `timebase=1/60000` cada PTS aumentará `timescale / fps = 1000` portanto, o **tempo real do PTS** para cada quadro poderia ser (supondo que começou em 0):

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

Para um cenário quase idêntico, mas com uma base de tempo igual a `1/60`.

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
* `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
* `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

Para um `fps=25/1` e `timebase=1/75` cada PTS aumentará `timescale/fps = 3` e o tempo de PTS pode ser:

* `frame=0, PTS = 0, PTS_TIME = 0`
* `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
* `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
* `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
* ...
* `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
* ...
* `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

Agora com o `pts_time` podemos encontrar uma maneira de renderizar isso sincronizado com o `pts_time` de áudio ou com o relógio do sistema. O libav do FFmpeg fornece essas informações por meio de sua API:

- fps = [`AVStream->avg_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a946e1e9b89eeeae4cab8a833b482c1ad)
- tbr = [`AVStream->r_frame_rate`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#ad63fb11cc1415e278e09ddc676e8a1ad)
- tbn = [`AVStream->time_base`](https://ffmpeg.org/doxygen/trunk/structAVStream.html#a9db755451f14e2bf590d4b85d82b32e6)

Apenas por curiosidade, os quadros que salvamos foram enviados em uma ordem DTS (quadros: 1,6,4,2,3,5), mas tocados em uma ordem PTS (quadros: 1,2,3,4,5). Além disso, observe como os quadros B são baratos em comparação com os quadros P ou I.

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

## Capítulo 2 - Remuxing

Remuxar é o ato de mudar de um formato (container) para outro, por exemplo, podemos mudar um vídeo [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4_Part_14) para um [MPEG-TS](https://en.wikipedia.org/wiki/MPEG_transport_stream) sem muito esforço usando o FFmpeg:

```bash
ffmpeg input.mp4 -c copy output.ts
```

Ele irá demultiplexar o mp4, mas não o decodificará ou codificará (`-c copy`) e, no final, o multiplexará em um arquivo `mpegts`. Se você não fornecer o formato `-f`, o ffmpeg tentará adivinhá-lo com base na extensão do arquivo.

O uso geral do FFmpeg ou do libav segue um padrão/arquitetura ou fluxo de trabalho:

* **[camada de protocolo](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - aceita uma entrada (`input`) (um arquivo, por exemplo, mas também pode ser uma entrada `rtmp` ou `HTTP`)
* **[camada de formato](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - faz a desmultiplexação de seu conteúdo, revelando principalmente metadados e seus fluxos
* **[camada de codec](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - decodifica os dados de fluxos comprimidos <sup>*opcional*</sup>
* **[camada de pixel](https://ffmpeg.org/doxygen/trunk/group__lavfi.html)** - também pode aplicar alguns `filtros` aos quadros brutos (como redimensionamento)<sup>*opcional*</sup>
* e então ele segue o caminho inverso
* **[camada de codec](https://ffmpeg.org/doxygen/trunk/group__libavc.html)** - codifica (ou re-codifica ou mesmo transcodifica) os quadros brutos<sup>*opcional*</sup>
* **[camada de formato](https://ffmpeg.org/doxygen/trunk/group__libavf.html)** - multiplexa (ou remultiplexa) os fluxos brutos (os dados comprimidos)
* **[camada de protocolo](https://ffmpeg.org/doxygen/trunk/protocols_8c.html)** - e finalmente, os dados multiplexados são enviados para uma saída (outro arquivo ou talvez um servidor remoto de rede)

![fluxo de trabalho do ffmpeg libav](/img/ffmpeg_libav_workflow.jpeg)
> Este gráfico é fortemente inspirado nos trabalhos de [Leixiaohua](http://leixiaohua1020.github.io/#ffmpeg-development-examples) e [Slhck](https://slhck.info/ffmpeg-encoding-course/#/9).

Agora vamos codificar um exemplo usando o libav para fornecer o mesmo efeito de `ffmpeg input.mp4 -c copy output.ts`.

Vamos ler de uma entrada (`input_format_context`) e convertê-la para outra saída (`output_format_context`).

```c
AVFormatContext *input_format_context = NULL;
AVFormatContext *output_format_context = NULL;
```

Começamos alocando a memória necessária e abrindo o formato de entrada. Para este caso específico, vamos abrir um arquivo de entrada e alocar memória para um arquivo de saída.

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

Vamos remuxar apenas os tipos de fluxos de vídeo, áudio e legenda, portanto, estamos armazenando em um array de índices quais fluxos serão usados.

```c
number_of_streams = input_format_context->nb_streams;
streams_list = av_mallocz_array(number_of_streams, sizeof(*streams_list));
```

Logo após alocarmos a memória necessária, vamos fazer um loop em todos os fluxos e, para cada um, precisamos criar um novo fluxo de saída em nosso contexto de formato de saída, usando a função [avformat_new_stream](https://ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827). Observe que estamos marcando todos os fluxos que não são de vídeo, áudio ou legenda para que possamos ignorá-los posteriormente.

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

Agora podemos criar o arquivo de saída.

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

Depois disso, podemos copiar os fluxos, pacote por pacote, dos nossos fluxos de entrada para os nossos fluxos de saída. Vamos fazer um loop enquanto tiver pacotes (`av_read_frame`), para cada pacote, precisamos recalcular o PTS e DTS para finalmente escrevê-lo (`av_interleaved_write_frame`) no nosso contexto de formato de saída.

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

Para finalizar, precisamos escrever o trailer do fluxo em um arquivo de mídia de saída com a função [av_write_trailer](https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga7f14007e7dc8f481f054b21614dfec13).

```c
av_write_trailer(output_format_context);
```

Agora estamos prontos para testar e o primeiro teste será a conversão de formato (container de vídeo) de um arquivo MP4 para um arquivo de vídeo MPEG-TS. Basicamente, estamos executando a linha de comando `ffmpeg input.mp4 -c copy output.ts` com o libav.

```bash
make run_remuxing_ts
```

Está funcionando!!! Você não confia em mim?! Você não deveria, podemos verificar com `ffprobe`:

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

Para resumir o que fizemos aqui em um gráfico, podemos revisitar nossa [ideia inicial sobre como o libav funciona](https://github.com/leandromoreira/ffmpeg-libav-tutorial#ffmpeg-libav-architecture) mostrando que pulamos a parte do codec.

![remuxing libav components](/img/remuxing_libav_components.png)

Antes de encerrarmos este capítulo, gostaria de mostrar uma parte importante do processo de remuxing, você pode passar opções para o muxer. Digamos que queremos entregar o formato [MPEG-DASH](https://developer.mozilla.org/en-US/docs/Web/Apps/Fundamentals/Audio_and_video_delivery/Setting_up_adaptive_streaming_media_sources#MPEG-DASH_Encoding) para isso precisamos usar [fragmented mp4](https://stackoverflow.com/a/35180327) (às vezes referido como `fmp4`) em vez de MPEG-TS ou MPEG-4 simples.

Com a [linha de comando, podemos fazer isso facilmente](https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE#Fragmenting).

```
ffmpeg -i non_fragmented.mp4 -movflags frag_keyframe+empty_moov+default_base_moof fragmented.mp4
```

Quase tão fácil quanto a linha de comando é a versão da biblioteca libav, só precisamos passar as opções ao escrever o cabeçalho de saída, logo antes da cópia dos pacotes.

```c
AVDictionary* opts = NULL;
av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
ret = avformat_write_header(output_format_context, &opts);
```

Agora podemos gerar este arquivo mp4 fragmentado:

```bash
make run_remuxing_fragmented_mp4
```

Mas para ter certeza de que não estou mentindo para você, você pode usar o incrível site/ferramenta [gpac/mp4box.js](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js/filereader.html) ou o site [http://mp4parser.com/](http://mp4parser.com/) para ver as diferenças. Primeiro carregue o mp4 "comum".

![mp4 boxes](/img/boxes_normal_mp4.png)

Como você pode ver, ele tem apenas um átomo/box `mdat`, **este é o local onde estão os quadros de vídeo e áudio**. Agora carregue o mp4 fragmentado para ver como ele espalha as caixas `mdat`.

![fragmented mp4 boxes](/img/boxes_fragmente_mp4.png)

## Capítulo 3 - Transcoding

> #### TLDR; mostre-me o [código](/3_transcoding.c) e a execução.
> ```bash
> $ make run_transcoding
> ```
> Vamos pular alguns detalhes, mas não se preocupe: o [código-fonte está disponível no github](/3_transcoding.c).


Neste capítulo, vamos criar um transcodificador minimalista, escrito em C, que pode converter vídeos codificados em H264 para H265 usando a biblioteca **FFmpeg/libav**, especificamente [libavcodec](https://ffmpeg.org/libavcodec.html), libavformat e libavutil.

![media transcoding flow](/img/transcoding_flow.png)

> _Apenas um rápido resumo:_ O [**AVFormatContext**](https://www.ffmpeg.org/doxygen/trunk/structAVFormatContext.html) é a abstração para o formato do arquivo de mídia, também conhecido como contêiner (ex: MKV, MP4, Webm, TS). O [**AVStream**](https://www.ffmpeg.org/doxygen/trunk/structAVStream.html) representa cada tipo de dados para um determinado formato (ex: áudio, vídeo, legenda, metadados). O [**AVPacket**](https://www.ffmpeg.org/doxygen/trunk/structAVPacket.html) é uma fatia de dados comprimidos obtidos do `AVStream` que pode ser decodificado por um [**AVCodec**](https://www.ffmpeg.org/doxygen/trunk/structAVCodec.html) (ex: av1, h264, vp9, hevc) gerando um dado bruto chamado [**AVFrame**](https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html).

### Transmuxing

Vamos começar com a operação simples de transmuxing e depois podemos desenvolver este código. O primeiro passo é **carregar o arquivo de entrada**.

```c
// Allocate an AVFormatContext
avfc = avformat_alloc_context();
// Open an input stream and read the header.
avformat_open_input(avfc, in_filename, NULL, NULL);
// Read packets of a media file to get stream information.
avformat_find_stream_info(avfc, NULL);
```

Agora vamos configurar o decodificador, o `AVFormatContext` nos dará acesso a todos os componentes do `AVStream` e, para cada um deles, podemos obter seu `AVCodec` e criar o `AVCodecContext` correspondente e, finalmente, podemos abrir o codec fornecido para que possamos prosseguir com o processo de decodificação.

> O [**AVCodecContext**](https://www.ffmpeg.org/doxygen/trunk/structAVCodecContext.html) contém dados sobre a configuração de mídia, como taxa de bits, taxa de quadros, taxa de amostragem, canais, altura e muitos outros.

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

Precisamos preparar o arquivo de mídia de saída para a transmuxação também, primeiro **alocamos memória** para o `AVFormatContext` de saída. Criamos **cada fluxo** no formato de saída. Para empacotar o fluxo adequadamente, **copiamos os parâmetros do codec** do decodificador.

**Definimos a flag** `AV_CODEC_FLAG_GLOBAL_HEADER`, que informa ao codificador que ele pode usar os cabeçalhos globais e, finalmente, abrimos o arquivo de saída para escrever e persistimos os cabeçalhos.

```c
avformat_alloc_output_context2(&encoder_avfc, NULL, NULL, out_filename);

AVStream *avs = avformat_new_stream(encoder_avfc, NULL);
avcodec_parameters_copy(avs->codecpar, decoder_avs->codecpar);

if (encoder_avfc->oformat->flags & AVFMT_GLOBALHEADER)
	encoder_avfc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

avio_open(&encoder_avfc->pb, encoder->filename, AVIO_FLAG_WRITE);
avformat_write_header(encoder->avfc, &muxer_opts);

```

Estamos recebendo os `AVPacket` do decodificador, ajustando os timestamps e escrevendo o pacote corretamente no arquivo de saída. Embora a função `av_interleaved_write_frame` diga "escrever quadro", estamos armazenando o pacote. Finalizamos o processo de transmuxing escrevendo o trailer do fluxo no arquivo.

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

### Transcodificação

A seção anterior mostrou um programa de transmuxer simples, agora vamos adicionar a capacidade de codificar arquivos, especificamente, vamos habilitá-lo para transcoded vídeos de `h264` para `h265`.

Após prepararmos o decodificador, mas antes de organizarmos o arquivo de mídia de saída, vamos configurar o codificador.

* Criar o `AVStream` de vídeo no codificador, [`avformat_new_stream`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__core.html#gadcb0fd3e507d9b58fe78f61f8ad39827)
* Usar o `AVCodec` chamado `libx265`, [`avcodec_find_encoder_by_name`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__encoding.html#gaa614ffc38511c104bdff4a3afa086d37)
* Criar o `AVCodecContext` com base no codec criado, [`avcodec_alloc_context3`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#gae80afec6f26df6607eaacf39b561c315)
* Configurar atributos básicos para a sessão de transcodificação, e
* Abrir o codec e copiar parâmetros do contexto para o stream. [`avcodec_open2`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d) e [`avcodec_parameters_from_context`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga0c7058f764778615e7978a1821ab3cfe)

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

Precisamos expandir nosso loop de decodificação para a transcodificação do fluxo de vídeo:

* Enviar o `AVPacket` vazio para o decodificador, [`avcodec_send_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3)
* Receber o `AVFrame` não comprimido, [`avcodec_receive_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c)
* Começar a transcodificar este frame bruto,
* Enviar o frame bruto, [`avcodec_send_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga9395cb802a5febf1f00df31497779169)
* Receber o `AVPacket` comprimido, com base no nosso codec, [`avcodec_receive_packet`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga5b8eff59cf259747cf0b31563e38ded6)
* Configurar o timestamp e [`av_packet_rescale_ts`](https://www.ffmpeg.org/doxygen/trunk/group__lavc__packet.html#gae5c86e4d93f6e7aa62ef2c60763ea67e)
* Escrever no arquivo de saída. [`av_interleaved_write_frame`](https://www.ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1)

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

Nós convertemos o fluxo de mídia de `h264` para `h265`, como esperado a versão `h265` do arquivo de mídia é menor que a versão `h264`, no entanto o [programa criado](/3_transcoding.c) é capaz de:

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

> Para ser honesto, isso foi mais difícil do que eu pensava que seria (https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54) e tive que mergulhar no código-fonte da linha de comando do FFmpeg (https://github.com/leandromoreira/ffmpeg-libav-tutorial/pull/54#issuecomment-570746749) e testar muito, e acho que ainda estou perdendo alguma coisa, pois tive que forçar `force-cfr` para o `h264` funcionar e ainda estou vendo algumas mensagens de aviso, como `warning messages (forced frame type (5) at 80 was changed to frame type (3))`.