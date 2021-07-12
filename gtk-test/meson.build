
gtk_test_sources = files(
    'gtk-eyelink-test.c',
)

gtk_test_deps = geye_deps + [libgtk3_dep]

gtk_test_include_dir = include_directories('../src')

gtk_test = executable(
    'gtk-eyelink-test',
    gtk_test_sources,
    dependencies : gtk_test_deps,
    include_directories : gtk_test_include_dir,
    link_with : libgeye
)
