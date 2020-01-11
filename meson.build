project(
  'lang',
  'cpp',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
  ]
)

add_global_arguments(
  '-Wno-unused-parameter',
  '-Wno-unused-function',
  '-Wno-switch',
  language: 'cpp'
)

flex = find_program('flex')
lgen = generator(
  flex,
  output: ['@BASENAME@.cc', '@BASENAME@.hh'],
  arguments: ['--outfile=@OUTPUT0@', '--header-file=@OUTPUT1@', '@INPUT@'],
)
lfiles = lgen.process('src/lexer.ll')

bison = find_program('bison')
yflags = ['-y', '-d', '-Werror=all', '-Wno-yacc']
pgen = generator(
  bison,
  output: ['@BASENAME@.cc', '@BASENAME@.hh'],
  arguments: yflags + ['--output=@OUTPUT0@', '--defines=@OUTPUT1@', '@INPUT@'],
)

llvm_dep = dependency('llvm')
spirv_dep = declare_dependency(
  link_args: '-lSPIRV',
)

compiler = executable(
  'compiler',
  [
    'src/main.cc',
    'src/codegen_llvm.cc',
    'src/typecheck.cc',
    'src/alt-parser.cc',
    lfiles,
  ],
  include_directories: 'src',
  dependencies: [
    llvm_dep,
    spirv_dep,
  ],
  install: true,
)

type_0_tests = ['scopes']
type_1_tests = ['parse', 'codegen']
type_2_tests = ['link', 'fib', 'gcd']

foreach test_name: type_0_tests
  test(test_name, executable(
    test_name,
    [
      'src' / test_name + '.cc',
      lfiles,
    ],
    include_directories: 'src',
    dependencies: [
      llvm_dep,
      spirv_dep,
    ]
  ))
endforeach

compiler_test_wrapper = find_program('compiler_test_wrapper.sh')

foreach test_name: type_1_tests
  test(test_name,
    compiler_test_wrapper,
    depends: compiler,
    args: [
      'link',
      meson.current_build_dir() / '..' / 'tests' / test_name + '.kl',
    ],
  )
endforeach

foreach test_name: type_2_tests
  test(test_name,
    compiler_test_wrapper,
    depends: compiler,
    args: [
      'exe',
      meson.current_build_dir() / '..' / 'tests' / test_name + '.kl',
    ],
  )
endforeach
