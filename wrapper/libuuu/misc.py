def get_libuuu_version():
    """Get libuuu version"""
    try:
        from libuuu.__version__ import __version__ as libuuu_version
    except ImportError:
        from setuptools_scm import get_version

        libuuu_version = get_version()
    return libuuu_version
