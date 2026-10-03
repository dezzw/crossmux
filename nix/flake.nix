{
  description = "CrossPoint Reader development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    # nixos-unstable dropped x86_64-darwin in 26.11; keep Intel Mac on 26.05-darwin.
    nixpkgs-x86_64-darwin.url = "github:NixOS/nixpkgs/nixpkgs-26.05-darwin";
    flake-compat.url = "github:NixOS/flake-compat";
  };

  outputs =
    { nixpkgs, nixpkgs-x86_64-darwin, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;

      # Detect the project root from wherever the user entered the shell,
      # so commands work from the repository root or any subdirectory.
      # Using `git rev-parse` to do so (assuming git is installed
      # system-wide); user can overwrite this by setting PROJECT_ROOT env.
      setEnvs = ''
        PROJECT_ROOT="''${PROJECT_ROOT:-$(git rev-parse --show-toplevel 2>/dev/null || pwd)}"
        export PROJECT_ROOT
        export PLATFORMIO_CORE_DIR="$PROJECT_ROOT/.cache/platformio"
      '';

      mkVenvBootstrap = pythonBin: ''
        if [ ! -x "$PROJECT_ROOT/.venv/bin/pio" ]; then
          echo "Creating .venv and installing pioarduino PlatformIO Core..."
          uv venv --python ${pythonBin} "$PROJECT_ROOT/.venv" &&
          uv pip install --python "$PROJECT_ROOT/.venv/bin/python" \
            -U https://github.com/pioarduino/platformio-core/archive/refs/tags/v6.1.19.zip \
            -r "$PROJECT_ROOT/requirements.txt" ||
          echo "Failed to install pioarduino PlatformIO Core" >&2
        fi
      '';

      monitorPython = pkgs: pkgs.python3.withPackages (ps: with ps; [
        matplotlib
        pyserial
        colorama
      ]);
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgsSrc = if system == "x86_64-darwin" then nixpkgs-x86_64-darwin else nixpkgs;
          pkgs = import pkgsSrc { inherit system; };
          monitorPy = monitorPython pkgs;
        in
        if pkgs.stdenv.isLinux then
          let
            fhsEnv = pkgs.buildFHSEnv {
              name = "crosspoint-reader-shell";

              targetPkgs =
                pkgs: with pkgs; [
                  python3
                  uv

                  # Runtime libraries used by PlatformIO's downloaded ESP32 toolchain binaries.
                  stdenv.cc.cc.lib
                  zlib
                  ncurses
                ];

              profile = ''
                ${setEnvs}
                export PATH="$PROJECT_ROOT/.venv/bin:$PATH"

                # Forcing python3 from fhsEnv, otherwise we get the following
                # exception while running `pip check`
                # ModuleNotFoundError: No module named 'littlefs'
                ${mkVenvBootstrap "/usr/bin/python3"}
              '';
            };
            pio = pkgs.writeShellScriptBin "pio" ''
              exec ${fhsEnv}/bin/crosspoint-reader-shell -c 'exec pio "$@"' pio "$@"
            '';
          in
          {
            default = pkgs.mkShell {
              packages = with pkgs; [
                pio
                fhsEnv
                clang-tools # for clang-format
                monitorPy
              ];

              shellHook = setEnvs;
            };
          }
        else
          let
            pythonBin = "${pkgs.python3}/bin/python3";
            pio = pkgs.writeShellScriptBin "pio" ''
              ${setEnvs}
              export PATH="$PROJECT_ROOT/.venv/bin:$PATH"
              if [ ! -x "$PROJECT_ROOT/.venv/bin/pio" ]; then
                echo "pio is not installed yet; enter \`nix develop ./nix\` first" >&2
                exit 1
              fi
              exec "$PROJECT_ROOT/.venv/bin/pio" "$@"
            '';
          in
          {
            default = pkgs.mkShell {
              packages = with pkgs; [
                python3
                uv
                pio
                clang-tools # for clang-format
                monitorPy
              ];

              shellHook = ''
                ${setEnvs}
                export PATH="$PROJECT_ROOT/.venv/bin:$PATH"
                ${mkVenvBootstrap pythonBin}
              '';
            };
          }
      );
    };
}
