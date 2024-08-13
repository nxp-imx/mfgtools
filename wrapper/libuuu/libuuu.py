#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
"""Main module for libuuu wrapper."""

import logging
import pathlib
import platform
from ctypes import (
    CDLL,
    CFUNCTYPE,
    POINTER,
    Structure,
    Union,
    c_char_p,
    c_int,
    c_size_t,
    c_ulonglong,
    c_ushort,
    c_void_p,
)
from dataclasses import dataclass
from enum import Enum, auto


class UUUNotifyType(Enum):
    """An enumeration to represent the different types of notifications from uuu."""

    NOTIFY_CMD_TOTAL = 0
    NOTIFY_CMD_START = auto()
    NOTIFY_CMD_END = auto()

    NOTIFY_CMD_INDEX = auto()

    NOTIFY_CMD_INFO = auto()

    NOTIFY_PHASE_TOTAL = auto()
    NOTIFY_PHASE_INDEX = auto()

    NOTIFY_TRANS_SIZE = auto()
    NOTIFY_TRANS_POS = auto()

    NOTIFY_WAIT_FOR = auto()
    NOTIFY_DEV_ATTACH = auto()

    NOTIFY_DECOMPRESS_START = auto()
    NOTIFY_DECOMPRESS_SIZE = auto()
    NOTIFY_DECOMPRESS_POS = auto()

    NOTIFY_DOWNLOAD_START = auto()
    NOTIFY_DOWNLOAD_END = auto()
    NOTIFY_THREAD_EXIT = auto()

    NOTIFY_DONE = auto()


@dataclass
class UUUNotifyDataUnion(Union):
    """A union to store data from a uuu notification."""

    _fields_ = [
        ("status", c_int),
        ("index", c_size_t),
        ("total", c_size_t),
        ("str", c_char_p),
    ]


@dataclass
class UUUNotifyStruct(Structure):
    """A class to store a notification from uuu."""

    _fields_ = [
        ("_type", c_int),
        ("id", c_ulonglong),
        ("timestamp", c_ulonglong),
        ("response", UUUNotifyDataUnion),
    ]

    @property
    def type(self) -> UUUNotifyType:
        """Return type attribute as an enum.

        :return UUUNotifyType enum
        """
        return UUUNotifyType(self._type)


@dataclass
class UUUCommandResponse(Structure):
    """A class to store the response from a uuu command."""

    _fields_ = [
        ("value", c_char_p),
    ]


UUUNotifyCallback = CFUNCTYPE(c_int, UUUNotifyStruct, POINTER(c_void_p))

UUUShowConfig = CFUNCTYPE(
    c_int,
    c_char_p,
    c_char_p,
    c_char_p,
    c_ushort,
    c_ushort,
    c_ushort,
    c_ushort,
    c_void_p,
)


@dataclass
class UUUState:
    """A class to store the state of a uuu library command call."""

    # pylint: disable=too-many-instance-attributes
    def __init__(self) -> None:
        """Constructor of UUUState class."""
        self.logger = logging.getLogger("UUUState")
        self.cmd: str = ""
        self.dev: str = ""

        self.waiting: bool = False
        self.done: bool = False
        self.error: bool = False

        self.cmd_status: bool = False
        self.cmd_done: bool = False

        self.cmd_end: int = 0
        self.cmd_pos: int = 0
        self.cmd_start: int = 0

        self.cmd_total: int = 0
        self.cmd_index: int = 0

        self.trans_pos: int = 0
        self.trans_size: int = 0

    def update(self, struct: UUUNotifyStruct) -> None:
        """Update the state with a notification from uuu.

        :param struct: A UUUNotifyStruct object
        """
        self.waiting = struct.type == UUUNotifyType.NOTIFY_WAIT_FOR
        self.done = struct.type == UUUNotifyType.NOTIFY_DONE

        if struct.type == UUUNotifyType.NOTIFY_CMD_TOTAL:
            self.cmd_total = struct.response.total
        elif struct.type == UUUNotifyType.NOTIFY_CMD_START:
            self.cmd = struct.response.str
            self.cmd_pos = 0
            self.cmd_start = struct.timestamp
        elif struct.type == UUUNotifyType.NOTIFY_CMD_END:
            status = struct.response.status
            self.done = True
            self.error = status != 0
            self.cmd_end = struct.timestamp
            if status != 0:
                self.logger.warning(f"Command {self.cmd} failed")
        elif struct.type == UUUNotifyType.NOTIFY_DEV_ATTACH:
            self.dev = struct.response.str
            self.done = False
            self.error = False
        elif struct.type == UUUNotifyType.NOTIFY_TRANS_SIZE:
            self.trans_size = struct.response.total
        elif struct.type == UUUNotifyType.NOTIFY_TRANS_POS:
            self.trans_pos = struct.response.total

        self.logger.debug(f"{self.cmd=},{self.dev=},{self.waiting=}")


def get_dll() -> str:
    """Return name of shared library based on platform."""
    if platform.system() == "Windows":
        return "libuuu.dll"
    if platform.system() == "Darwin":
        return "libuuu.dylib"
    return "libuuu.so"


@UUUNotifyCallback
def _default_notify_callback(struct: UUUNotifyStruct, data) -> int:  # type: ignore
    """A default callback function that stores the response in a class variable.

    :param struct: A UUUNotifyStruct object
    :param data: A pointer to data, here it is not used
    """
    # pylint: disable=unused-argument
    LibUUU._state.update(struct)
    if struct.type == UUUNotifyType.NOTIFY_CMD_INFO:
        LibUUU._response.value += bytes(struct.response.str)
    return 1 if LibUUU._state.error else 0


class LibUUU:
    """Wrapper for the libuuu library."""

    DLL = str(pathlib.Path(__file__).parent / "lib" / get_dll())
    NULL = POINTER(c_void_p)()

    _response = UUUCommandResponse()
    _state = UUUState()

    def __init__(self) -> None:
        """Initialize the library and register the default notify callback function."""
        self._response.value = b""
        self.lib = CDLL(self.DLL, mode=1)
        self.register_notify_callback(_default_notify_callback, self.NULL)

    def set_wait_timeout(self, timeout: int) -> int:
        """Set the wait timeout for uuu in seconds."""
        return self.lib.uuu_set_wait_timeout(c_int(timeout))

    def set_wait_next_timeout(self, timeout: int) -> int:
        """Set the wait next timeout for uuu in seconds."""
        return self.lib.uuu_set_wait_next_timeout(c_int(timeout))

    def set_poll_period(self, period: int) -> int:
        """Set the poll period for uuu in milliseconds."""
        return self.lib.uuu_set_poll_period(c_int(period))

    def set_debug_level(self, level: int) -> int:
        """Set the debug level for uuu.

        :param level: The debug level [15:0] for libusb, [31:16] for uuu
        """
        return self.lib.uuu_set_debug_level(c_int(level))

    def set_small_mem(self, size: int) -> int:
        """Disable small memory mode and buffer all data.

        :param size: The size of the buffer in bytes
        """
        return self.lib.uuu_set_small_mem(c_int(size))

    def get_version_string(self) -> str:
        """Get the version of uuu."""
        self.lib.uuu_get_version_string.restype = c_char_p
        return self.lib.uuu_get_version_string().decode()

    def get_version(self) -> int:
        """Get the version of uuu.

        :return: The version of uuu bits represent version in format [31:24].[23:12].[11:0]
        """
        return self.lib.uuu_get_version()

    def get_last_error_string(self) -> str:
        """Get the last error message from uuu."""
        self.lib.uuu_get_last_err_string.restype = c_char_p
        return self.lib.uuu_get_last_err_string().decode()

    def get_last_error(self) -> int:
        """Get the last error code from uuu."""
        return self.lib.uuu_get_last_err()

    def run_cmd(self, cmd: str, dry: bool = False) -> int:
        """Run a uuu command.

        :param cmd: The command to run
        :param dry: If set to False command will be executed, otherwise its a dry run
        :return: 0 if success
        """
        # pylint: disable=attribute-defined-outside-init
        self._response.value = b""
        return self.lib.uuu_run_cmd(c_char_p(str.encode(cmd)), c_int(1 if dry else 0))

    def register_notify_callback(self, callback: UUUNotifyCallback, data: any) -> int:  # type: ignore
        """Register a callback function to receive notifications from uuu.

        :param callback: A function that will be called when uuu sends a notification
        :param data: A pointer to data that will be passed to the callback function
        :return: 0 on success, otherwise failure
        """
        return self.lib.uuu_register_notify_callback(callback, data)

    def unregister_notify_callback(self, callback: UUUNotifyCallback) -> int:  # type: ignore
        """Unregister the callback function."""
        return self.lib.uuu_unregister_notify_callback(callback)

    @property
    def response(self) -> bytes:
        """Get the response from the last uuu command."""
        return self._response.value
