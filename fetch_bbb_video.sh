#!/bin/bash
read -p "WARNING: You're about to download 340MB, are you sure? (yN) " -n 1 -r
echo    # (optional) move to a new line
if [[ ! $REPLY =~ ^[Yy]$ ]]
then
  echo "n"
  [[ "$0" = "$BASH_SOURCE" ]] && exit 1 || return 1 # handle exits from shell or function but don't exit interactive shell
fi

check_cmd()
{
  if ! which $1 &>/dev/null; then
    error "$1 command not found, you must install it before."
  fi
}

check_cmd wget
check_cmd ffmpeg

wget -O bunny_1080p_60fps.mp4 https://github.com/leandromoreira/big-buck-bunny/raw/main/Big-Buck-Bunny-1080p-h264-4mb-60fps-multi-ch-ac3.mp4
ffmpeg -y -i bunny_1080p_60fps.mp4 -ss 00:01:24 -t 00:00:10 small_bunny_1080p_60fps.mp4
