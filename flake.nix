{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    cutils.url = "github:hant-hub/Cutils";

  };

  outputs =
    { self, nixpkgs, ... }:
    let
      forAllSys = nixpkgs.lib.genAttrs nixpkgs.lib.platforms.all;
    in
    {
      packages = forAllSys (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          simpletodo = pkgs.callPackage ./. { };
        in
        {
          default = simpletodo;
        }
      );
    };
}

