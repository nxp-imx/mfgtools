#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

"""Wrapper for libuuu."""

from .libuuu import LibUUU, UUUNotifyCallback, UUUShowConfig, UUUState
from .misc import get_libuuu_version

version = get_libuuu_version()

__author__ = """NXP"""
__version__ = str(version)
__all__ = ["LibUUU", "UUUNotifyCallback", "UUUShowConfig", "UUUState"]
