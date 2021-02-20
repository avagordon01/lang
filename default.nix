with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "lang";
  src = ./.;

  buildInputs = [ meson ninja flex llvm_10 glslang ];
}
