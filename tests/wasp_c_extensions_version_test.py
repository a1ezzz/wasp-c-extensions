# -*- coding: utf-8 -*-

from wasp_c_extensions.version import package_version


def test_version():
	assert(isinstance(package_version(), str) is True)
	assert(len(package_version()) > 0)
