# -*- coding: utf-8 -*-
# wasp_c_extensions/<FILENAME>.py
#
# Copyright (C) 2018 the wasp-c-extensions authors and contributors
# <see AUTHORS file>
#
# This file is part of wasp-c-extensions.
#
# Wasp-c-extensions is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Wasp-c-extensions is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with wasp-c-extensions.  If not, see <http://www.gnu.org/licenses/>.

import os
import json
import subprocess


with open(os.path.join(os.path.dirname(__file__), 'package.json'), 'r') as f:
	__package_data__ = json.load(f)

__author__ = __package_data__['author']
__email__ = __package_data__['author_email']
__credits__ = __package_data__['credits']
__license__ = __package_data__['license']
__copyright__ = __package_data__['copyright']
__status__ = __package_data__['status']
__version__ = __package_data__['version']
