#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

"""Miscellaneous functions for libuuu."""

from setuptools_scm import get_version


def get_libuuu_version() -> str:
    """Get libuuu version."""
    try:
        # pylint: disable=import-outside-toplevel
        from libuuu.__version__ import __version__ as libuuu_version
    except ImportError:
        libuuu_version = get_version()
    return libuuu_version
