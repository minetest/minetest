{ pkgs ? import <nixpkgs> {}, }:

pkgs.mkShell {
  LOCALE_ARCHIVE = "${pkgs.glibcLocales}/lib/locale/locale-archive";
  env.LANG = "C.UTF-8";
  env.LC_ALL = "C.UTF-8";

  packages = [
    pkgs.SDL2
    pkgs.cmake
    pkgs.curl
    pkgs.freetype
    pkgs.gcc
    pkgs.gettext
    pkgs.libGL
    pkgs.libjpeg
    pkgs.libogg
    pkgs.libpng
    pkgs.libvorbis
    pkgs.openal
    pkgs.sqlite
    pkgs.zlib
    pkgs.zstd
  ];
}
