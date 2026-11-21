{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "simple-todo";
  version = "1.0.0";
  src = ./.;

  buildCommand = ''
    cd /tmp
    cp -r --no-preserve=mode $src/* .
    rm -rf build
    rm -rf lib/cutils/sb
    rm -rf lib/cutils/build
    cc build.c -g -o sb
    ./sb
    mkdir -p $out/bin
    cp build/stls $out/bin
    cp build/stedit $out/bin
  '';

}
