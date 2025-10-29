#!/usr/bin/env python
# -*- coding: UTF-8 -*-
## Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

"""Basic tests for libuuu wrapper."""

import pytest

from libuuu import LibUUU
from libuuu.libuuu import UUUNotifyType, UUUState, get_dll_path, get_platform_info


def test_check_dlopen_works() -> None:
    """Test that the library can be loaded."""
    lib = LibUUU()
    assert lib is not None


def test_get_last_error_string() -> None:
    """Test getting last error string."""
    lib = LibUUU()
    error_str = lib.get_last_error_string()
    assert isinstance(error_str, str)


def test_get_last_error_code() -> None:
    """Test getting last error code."""
    lib = LibUUU()
    error_code = lib.get_last_error()
    assert isinstance(error_code, int)


def test_set_wait_timeout() -> None:
    """Test setting wait timeout."""
    lib = LibUUU()
    result = lib.set_wait_timeout(30)
    assert isinstance(result, int)


def test_set_wait_next_timeout() -> None:
    """Test setting wait next timeout."""
    lib = LibUUU()
    result = lib.set_wait_next_timeout(5)
    assert isinstance(result, int)


def test_set_poll_period() -> None:
    """Test setting poll period."""
    lib = LibUUU()
    result = lib.set_poll_period(1000)
    assert isinstance(result, int)


def test_set_debug_level() -> None:
    """Test setting debug level."""
    lib = LibUUU()
    result = lib.set_debug_level(0)
    assert isinstance(result, int)


def test_set_small_mem() -> None:
    """Test setting small memory mode."""
    lib = LibUUU()
    result = lib.set_small_mem(1024 * 1024)  # 1MB
    assert isinstance(result, int)


def test_run_cmd_dry_run() -> None:
    """Test running a command in dry run mode."""
    lib = LibUUU()
    # Use a simple help command for dry run
    result = lib.run_cmd("help", dry=True)
    assert isinstance(result, int)


def test_response_property() -> None:
    """Test that response property returns bytes."""
    lib = LibUUU()
    response = lib.response
    assert isinstance(response, bytes)


def test_get_platform_info() -> None:
    """Test getting platform information."""
    system, arch = get_platform_info()
    assert isinstance(system, str)
    assert isinstance(arch, str)
    assert len(system) > 0
    assert len(arch) > 0
    # Check that we get expected values
    assert system in ["windows", "darwin", "linux"]
    assert arch in ["x86_64", "aarch64", "armv7l", "arm64"]


def test_get_dll_path() -> None:
    """Test getting DLL path."""
    dll_path = get_dll_path()
    assert isinstance(dll_path, str)
    assert len(dll_path) > 0
    # Check that path contains expected library extension
    assert any(ext in dll_path for ext in [".dll", ".so", ".dylib"])


def test_uuu_notify_type_enum() -> None:
    """Test UUUNotifyType enum values."""
    # Test that enum has expected values
    assert UUUNotifyType.NOTIFY_CMD_TOTAL.value == 0
    assert UUUNotifyType.NOTIFY_CMD_START.value == 1
    assert UUUNotifyType.NOTIFY_CMD_END.value == 2
    assert UUUNotifyType.NOTIFY_DONE.value == 17


def test_uuu_state_initialization() -> None:
    """Test UUUState class initialization."""
    state = UUUState()
    assert state.cmd == ""
    assert state.dev == ""
    assert state.waiting is False
    assert state.done is False
    assert state.error is False
    assert state.cmd_status is False
    assert state.cmd_done is False
    assert state.cmd_total == 0
    assert state.cmd_index == 0
    assert state.trans_pos == 0
    assert state.trans_size == 0


def test_library_constants() -> None:
    """Test library constants."""
    lib = LibUUU()
    assert hasattr(lib, "DLL")
    assert hasattr(lib, "NULL")
    assert isinstance(lib.DLL, str)


@pytest.mark.parametrize("timeout_value", [1, 10, 30, 60])
def test_set_wait_timeout_various_values(timeout_value: int) -> None:
    """Test setting various timeout values."""
    lib = LibUUU()
    result = lib.set_wait_timeout(timeout_value)
    assert isinstance(result, int)


@pytest.mark.parametrize("debug_level", [0, 1, 15, 65535])
def test_set_debug_level_various_values(debug_level: int) -> None:
    """Test setting various debug levels."""
    lib = LibUUU()
    result = lib.set_debug_level(debug_level)
    assert isinstance(result, int)


def test_error_handling_invalid_command() -> None:
    """Test error handling with invalid command."""
    lib = LibUUU()
    # Try to run an invalid command in dry run mode
    result = lib.run_cmd("invalid_command_xyz", dry=True)
    # Should return an error code (non-zero) but not crash
    assert isinstance(result, int)


def test_get_version() -> None:
    """Test getting library version as an integer."""
    lib = LibUUU()
    ver = lib.get_version()
    assert isinstance(ver, int)


def test_get_version_string() -> None:
    """Test getting library version as a string."""
    lib = LibUUU()
    ver = lib.get_version_string()
    assert isinstance(ver, str)
