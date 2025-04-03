{
    description = "A simple battery reminder written in C.";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }:
        flake-utils.lib.eachDefaultSystem (system:
            let
                pkgs = nixpkgs.legacyPackages.${system};
            in
                {
                packages.default = pkgs.stdenv.mkDerivation {
                    pname = "battrem";
                    version = "0.1";
                    src = self;

                    nativeBuildInputs = with pkgs; [
                        meson
                        ninja
                        pkg-config
                    ];

                    buildInputs = with pkgs; [
                        libnotify
                    ];

                    meta = {
                        description = "A simple battery reminder written in C";
                        homepage = "https://github.com/commrade-goad/battrem";
                    };
                };

                devShells.default = pkgs.mkShell {
                    buildInputs = with pkgs; [
                        meson
                        ninja
                        pkg-config
                        gcc

                        libnotify
                        glibc
                    ];

                    shellHook = ''
                        echo "Use 'meson setup build' to configure your project"
                        echo "Then 'ninja -C build' to build"
                    '';
                };
            });
}
