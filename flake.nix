{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    cutils.url = "github:hant-hub/Cutils";

  };

  outputs =
    {
      self,
      nixpkgs,
      ...
    }@inputs:
    let
      forAllSys = nixpkgs.lib.genAttrs nixpkgs.lib.platforms.all;
    in
    {

      devShells = forAllSys (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = [ inputs.cutils.packages.${pkgs.system}.default ];

            shellHook = "zsh";
          };
        }
      );

      packages = forAllSys (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          cutils = inputs.cutils.packages.${pkgs.system}.default;
          simpletodo = pkgs.callPackage ./. { cutils = cutils; };
        in
        {
          default = simpletodo;
        }
      );
    };
}
