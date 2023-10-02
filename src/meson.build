texty_sources = [
  'main.c',
  'texty-application.c',
  'texty-window.c',
]

texty_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.4'),
]

texty_sources += gnome.compile_resources('texty-resources',
  'texty.gresource.xml',
  c_name: 'texty'
)

executable('texty', texty_sources,
  dependencies: texty_deps,
       install: true,
)
