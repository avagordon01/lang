with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "lang";
  src = ./.;

  buildInputs = [ flex bison llvm_8 glslang ];

  installPhase = ''
    mkdir -p $out/bin
    cp out/compiler $out/bin
  '';
}
