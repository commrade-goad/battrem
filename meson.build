project('battrem', 'c',
  version : '0.1',
  default_options : [
    'warning_level=3',
    'c_std=c23',
    'optimization=3',
    'debug=false'
  ]
)

pname = 'battrem'
libnotify_dep = dependency('libnotify', method : 'pkg-config')

executable(
  pname,
  'src/main.c',
  dependencies : [libnotify_dep],
  install : true
)
