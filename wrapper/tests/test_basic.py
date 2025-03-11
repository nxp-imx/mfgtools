
from libuuu import LibUUU

def test_check_dlopen_works():
    lib  = LibUUU()

def test_try_get_version():
    lib = LibUUU()
    assert isinstance(lib.get_version_string(), str)
    
