

eyelink_test_sources = files(
    'eyelink-test.c',
)

testdeps = geye_deps
testenv = [
    'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
    'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir())
]

test_include_dir = include_directories('../src')

eyelink_test = executable(
    'eyelink_test',
    eyelink_test_sources,
    dependencies : testdeps,
    include_directories : test_include_dir,
    link_with : libgeye
)

test (
    'eyelink_test',
    eyelink_test,
    env : testenv
)

