#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

"""Wrapper for libuuu."""

from .__version__ import __version__ as version
from .libuuu import LibUUU, UUUNotifyCallback, UUUShowConfig, UUUState

__author__ = """NXP"""
__version__ = version
__all__ = ["LibUUU", "UUUNotifyCallback", "UUUShowConfig", "UUUState"]
