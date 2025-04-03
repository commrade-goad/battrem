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
                devShells.default = pkgs.mkShell {
                    buildInputs = with pkgs; [
                        # Build tools
                        meson
                        ninja
                        pkg-config
                        gcc

                        # Dependencies
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
