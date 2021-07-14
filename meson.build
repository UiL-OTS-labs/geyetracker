# A gobject style eyetracker library
# Copyright (C) 2021  Maarten Duijndam
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

project(
    'geyetracker',
    ['c'],
    version : '0.0.0',
    default_options : [
        'warning_level=3'
    ],
    license : 'LGPL-2.1-or-later'
)

c_compiler = meson.get_compiler('c')

geye_version = meson.project_version()
version_array = geye_version.split('.')
version_major = version_array[0]
version_minor = version_array[1]
version_micro = version_array[1]
version_string = '' + version_major + '.' + version_minor + '.' +  version_micro
geye_version = 'geye-' + version_major + '.' + version_minor
geye_api_version = version_major + '.0'

# dependencies
libglib_dep = dependency('glib-2.0', fallback : ['glib', 'libglib_dep'] )
libgobject_dep = dependency('gobject-2.0', fallback : ['glib', 'libgobject_dep'])
libgio_dep= dependency('gio-2.0', fallback : ['glib', 'libgio_dep'])
libgmodule_dep = dependency('gmodule-2.0', fallback : ['glib', 'libgmodule_dep'])
#libcairogobject_dep = dependency('cairo-gobject')
libgtk3_dep= dependency('gtk+-3.0')

lib_eyelink_core = c_compiler.find_library('eyelink_core', static : false)

geye_deps = [libglib_dep, libgobject_dep, libgio_dep, libgmodule_dep]

subdir ('src')
subdir ('test')
subdir ('gtk-test')

pkg = import ('pkgconfig')
pkg.generate (
    libgeye,
    name : 'geye',
    filebase : 'geye',
    subdirs : 'geye',
    requires : 'glib-2.0',
    description : 'A gobject style eyetracker library',
    version : geye_version
)

