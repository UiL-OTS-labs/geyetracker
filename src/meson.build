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

geye_public_header = 'geye.h'
geye_public_headers = files (
    geye_public_header,
    'eyelink-et.h',
    'eyetracker-error.h',
    'eyetracker.h'
)

install_headers(geye_public_headers, subdir : 'geye')

extra_c_args = ['-DG_LOG_DOMAIN="GEye"', '-DG_LOG_USE_STRUCTURED']
buildtype = get_option('buildtype')
if buildtype != 'debug' and buildtype != 'plain'
    extra_c_args += ['-DG_DISABLE_ASSERT']
else
    extra_c_args += []
endif



geye_sources = files(
    'eyelink-et-private.c',
    'eyelink-et.c',
    'eyetracker-error.c',
    'eyetracker.c'
)

libgeye = library(
    'geye',
    geye_sources,
    dependencies : geye_deps + [lib_eyelink_core],
    gnu_symbol_visibility : 'hidden',
    install : true,
    c_args : extra_c_args
)

libgeye_dep = declare_dependency(
    link_with : libgeye,
    dependencies : geye_deps + [lib_eyelink_core]
)


