with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "lang";
  src = ./.;

  buildInputs = [ meson ninja llvm_10 glslang graphviz ];
}
