#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
"""Custom build backend that wraps setuptools and forces platform wheels."""

import os
from typing import Any, Tuple

from setuptools.build_meta import build_editable as _build_editable
from setuptools.build_meta import build_sdist as _build_sdist
from setuptools.build_meta import build_wheel as _build_wheel
from setuptools.dist import Distribution
from wheel.bdist_wheel import bdist_wheel


def patch() -> None:
    """Patch the bdist wheel build."""
    # Apply the monkey patch when the module is imported
    original_has_ext_modules = getattr(Distribution, "has_ext_modules", None)
    if original_has_ext_modules is not None:
        Distribution.has_ext_modules = lambda self: True

    # Also patch the is_pure method to return False
    original_is_pure = getattr(Distribution, "is_pure", None)
    if original_is_pure is not None:
        Distribution.is_pure = lambda self: False

    # Override the wheel tag generation

    original_get_tag = getattr(bdist_wheel, "get_tag", None)
    if original_get_tag is not None:

        def tag(self: Any) -> Tuple[str, str, str]:
            _, _, platform_tag = original_get_tag(self)
            return ("py3", "none", platform_tag)

        bdist_wheel.get_tag = tag

    print("Custom build backend loaded: Platform-specific wheels enabled")


def validate_native_libraries() -> None:
    """Validate that native libraries exist in the expected locations."""
    lib_dir = os.path.join(os.path.dirname(__file__), "libuuu", "lib")

    if os.path.exists(lib_dir):
        print(f"Native libraries found in: {lib_dir}")
        # List found libraries
        for root, _, files in os.walk(lib_dir):
            for file in files:
                if file.endswith((".dll", ".so", ".dylib")):
                    print(f"  - {os.path.relpath(os.path.join(root, file), lib_dir)}")
    else:
        print(f"Warning: No native libraries found in {lib_dir}")


def build_wheel(wheel_directory, config_settings=None, metadata_directory=None) -> str:  # type: ignore
    """Build wheel."""
    patch()
    validate_native_libraries()
    return _build_wheel(wheel_directory, config_settings, metadata_directory)


def build_sdist(sdist_directory, config_settings=None) -> str:  # type: ignore
    """Build source."""
    patch()
    validate_native_libraries()
    return _build_sdist(sdist_directory, config_settings)


def build_editable(wheel_directory, config_settings=None, metadata_directory=None) -> str:  # type: ignore
    """Build editable."""
    patch()
    validate_native_libraries()
    return _build_editable(wheel_directory, config_settings, metadata_directory)
