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

wget -O bunny_1080p_60fps.mp4 http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4
ffmpeg -y -i bunny_1080p_60fps.mp4 -ss 00:01:24 -t 00:00:10 small_bunny_1080p_60fps.mp4
