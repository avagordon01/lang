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
  '-Wno-missing-field-initializers',
  '-Wno-pedantic',
  '-fmax-errors=1',
  language: 'cpp'
)

llvm_dep = dependency('llvm')
spirv_dep = declare_dependency(
  link_args: '-lSPIRV',
)

expected_dep = declare_dependency(
  include_directories: [
    'vendor/expected/include',
  ]
)

compiler = executable(
  'compiler',
  [
    'src/main.cc',
    'src/codegen_llvm.cc',
    'src/typecheck.cc',
    'src/parser.cc',
    'src/tokens.cc',
    'src/lexer.cc',
  ],
  dependencies: [
    llvm_dep,
    spirv_dep,
    expected_dep,
  ],
  install: true,
)

foreach test_name: ['scopes']
  test(test_name, executable(
    test_name,
    [
      'src' / test_name + '.cc',
    ],
    include_directories: 'src',
    dependencies: [
      llvm_dep,
      spirv_dep,
    ]
  ))
endforeach

compiler_test_wrapper = find_program('compiler_test_wrapper.sh')
tests_dir = meson.source_root() / 'tests'
foreach test_name: ['parse', 'codegen', 'link', 'fib', 'gcd']
  test(test_name,
    compiler_test_wrapper,
    depends: compiler,
    args: [
      tests_dir / test_name,
    ],
  )
endforeach
