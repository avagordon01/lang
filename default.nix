with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "lang";
  src = ./.;

  buildInputs = [ meson ninja flex bison llvm_8 glslang ];
}
