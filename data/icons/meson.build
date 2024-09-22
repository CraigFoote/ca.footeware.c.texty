application_id = 'ca.footeware.c.texty'

scalable_dir = 'hicolor' / 'scalable' / 'apps'
install_data(
  scalable_dir / ('@0@.svg').format(application_id),
  install_dir: get_option('datadir') / 'icons' / scalable_dir
)

scalable_dir = 'hicolor' / 'scalable' / 'actions'
install_data(
  scalable_dir / ('@0@.svg').format('ca.footeware.c.texty.line-wrapping-symbolic'),
  install_dir: get_option('datadir') / 'icons' / scalable_dir
)

symbolic_dir = 'hicolor' / 'symbolic' / 'apps'
install_data(
  symbolic_dir / ('@0@-symbolic.svg').format(application_id),
  install_dir: get_option('datadir') / 'icons' / symbolic_dir
)
