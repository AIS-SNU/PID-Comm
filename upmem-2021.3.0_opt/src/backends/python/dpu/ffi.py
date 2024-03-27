# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

r"""Wrapper for dpu.h

Do not modify this file.
"""

import glob
import re
import os.path
import ctypes.util
import ctypes
import platform
__docformat__ = "restructuredtext"

# Begin preamble for Python v(3, 2)

import ctypes
import os
import sys
from ctypes import *

_int_types = (c_int16, c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (c_int64,)
for t in _int_types:
    if sizeof(t) == sizeof(c_size_t):
        c_ptrdiff_t = t
del t
del _int_types


class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __cmp__(self, string):
        if isinstance(string, UserString):
            return cmp(self.data, string.data)
        else:
            return cmp(self.data, string)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1:]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1:]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, Union):

    _fields_ = [("raw", POINTER(c_char)), ("data", c_char_p)]

    def __init__(self, obj=""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(POINTER(c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, c_char_p):
            return obj

        # Convert from POINTER(c_char)
        elif isinstance(obj, POINTER(c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(cast(obj, POINTER(c_char)))

        # Convert from c_char array
        elif isinstance(obj, c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to c_void_p.
def UNCHECKED(type):
    if hasattr(
            type,
            "_type_") and isinstance(
            type._type_,
            str) and type._type_ != "P":
        return type
    else:
        return c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (
        isinstance(
            value,
            bytes) or isinstance(
            value,
            str)) else value

# End preamble


_libs = {}
_libdirs = []

# Begin loader

# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------


def _environ_path(name):
    if name in os.environ:
        return os.environ[name].split(":")
    else:
        return []


class LibraryLoader(object):
    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup(object):
        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            try:
                return self.Lookup(path)
            except BaseException:
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # then we search the directory where the generated python interface
            # is stored
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib
            # paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, libname):
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [format % libname for format in self.name_formats]

        for dir in self.getdirs(libname):
            for name in names:
                yield os.path.join(dir, name)

    def getdirs(self, libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path(
            "DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"), "/usr/local/lib", "/usr/lib"]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))

        if hasattr(sys, "frozen") and sys.frozen == "macosx_app":
            dirs.append(
                os.path.join(
                    os.environ["RESOURCEPATH"],
                    "..",
                    "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    class _Directories(dict):
        def __init__(self):
            self.order = 0

        def add(self, directory):
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            o = self.setdefault(directory, self.order)
            if o == self.order:
                self.order += 1

        def extend(self, directories):
            for d in directories:
                self.add(d)

        def ordered(self):
            return (i[0] for i in sorted(self.items(), key=lambda D: D[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive funtion to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as f:
                for D in f:
                    D = D.strip()
                    if not D:
                        continue

                    m = self._include.match(D)
                    if not m:
                        dirs.add(D)
                    else:
                        for D2 in glob.glob(m.group("pattern")):
                            self._get_ld_so_conf_dirs(D2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HPUX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu",
                                       "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/x86_64-linux-gnu",
                                       "/usr/lib/x86_64-linux-gnu"]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        ext_re = re.compile(r"\.s[ol]$")
        for dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, [])
                    cache_i.append(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, [])
                        cache_i.append(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, [])
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for F in other_dirs:
        if not os.path.isabs(F):
            F = os.path.abspath(F)
        load_library.other_dirs.append(F)


del loaderclass

# End loader

add_library_search_dirs([os.path.dirname(os.path.dirname(
    os.path.dirname((os.path.dirname(__file__)))))])

# Begin libraries
_libs["libdpu.so"] = load_library("libdpu.so")

# 1 libraries
# End libraries

# No modules

__off_t = c_long  # /usr/include/x86_64-linux-gnu/bits/types.h: 150

__off64_t = c_long  # /usr/include/x86_64-linux-gnu/bits/types.h: 151

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 49


class struct__IO_FILE(Structure):
    pass


FILE = struct__IO_FILE  # /usr/include/x86_64-linux-gnu/bits/types/FILE.h: 7

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 36


class struct__IO_marker(Structure):
    pass

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 37


class struct__IO_codecvt(Structure):
    pass

# /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 38


class struct__IO_wide_data(Structure):
    pass


_IO_lock_t = None  # /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h: 43

struct__IO_FILE.__slots__ = [
    '_flags',
    '_IO_read_ptr',
    '_IO_read_end',
    '_IO_read_base',
    '_IO_write_base',
    '_IO_write_ptr',
    '_IO_write_end',
    '_IO_buf_base',
    '_IO_buf_end',
    '_IO_save_base',
    '_IO_backup_base',
    '_IO_save_end',
    '_markers',
    '_chain',
    '_fileno',
    '_flags2',
    '_old_offset',
    '_cur_column',
    '_vtable_offset',
    '_shortbuf',
    '_lock',
    '_offset',
    '_codecvt',
    '_wide_data',
    '_freeres_list',
    '_freeres_buf',
    '__pad5',
    '_mode',
    '_unused2',
]
struct__IO_FILE._fields_ = [
    ('_flags', c_int),
    ('_IO_read_ptr', String),
    ('_IO_read_end', String),
    ('_IO_read_base', String),
    ('_IO_write_base', String),
    ('_IO_write_ptr', String),
    ('_IO_write_end', String),
    ('_IO_buf_base', String),
    ('_IO_buf_end', String),
    ('_IO_save_base', String),
    ('_IO_backup_base', String),
    ('_IO_save_end', String),
    ('_markers', POINTER(struct__IO_marker)),
    ('_chain', POINTER(struct__IO_FILE)),
    ('_fileno', c_int),
    ('_flags2', c_int),
    ('_old_offset', __off_t),
    ('_cur_column', c_ushort),
    ('_vtable_offset', c_char),
    ('_shortbuf', c_char * int(1)),
    ('_lock', POINTER(_IO_lock_t)),
    ('_offset', __off64_t),
    ('_codecvt', POINTER(struct__IO_codecvt)),
    ('_wide_data', POINTER(struct__IO_wide_data)),
    ('_freeres_list', POINTER(struct__IO_FILE)),
    ('_freeres_buf', POINTER(None)),
    ('__pad5', c_size_t),
    ('_mode', c_int),
    ('_unused2', c_char * int((((15 * sizeof(c_int)) - (4 * sizeof(POINTER(None)))) - sizeof(c_size_t)))),
]

enum_dpu_error_t = c_int  # dpu_error.h: 92

DPU_OK = 0  # dpu_error.h: 92

DPU_ERR_INTERNAL = (DPU_OK + 1)  # dpu_error.h: 92

DPU_ERR_SYSTEM = (DPU_ERR_INTERNAL + 1)  # dpu_error.h: 92

DPU_ERR_DRIVER = (DPU_ERR_SYSTEM + 1)  # dpu_error.h: 92

DPU_ERR_ALLOCATION = (DPU_ERR_DRIVER + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_DPU_SET = (DPU_ERR_ALLOCATION + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_THREAD_ID = (DPU_ERR_INVALID_DPU_SET + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_NOTIFY_ID = (DPU_ERR_INVALID_THREAD_ID + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_WRAM_ACCESS = (
    DPU_ERR_INVALID_NOTIFY_ID +
    1)  # dpu_error.h: 92

DPU_ERR_INVALID_IRAM_ACCESS = (
    DPU_ERR_INVALID_WRAM_ACCESS +
    1)  # dpu_error.h: 92

DPU_ERR_INVALID_MRAM_ACCESS = (
    DPU_ERR_INVALID_IRAM_ACCESS +
    1)  # dpu_error.h: 92

DPU_ERR_INVALID_SYMBOL_ACCESS = (
    DPU_ERR_INVALID_MRAM_ACCESS +
    1)  # dpu_error.h: 92

DPU_ERR_MRAM_BUSY = (DPU_ERR_INVALID_SYMBOL_ACCESS + 1)  # dpu_error.h: 92

DPU_ERR_TRANSFER_ALREADY_SET = (DPU_ERR_MRAM_BUSY + 1)  # dpu_error.h: 92

DPU_ERR_NO_PROGRAM_LOADED = (
    DPU_ERR_TRANSFER_ALREADY_SET +
    1)  # dpu_error.h: 92

DPU_ERR_DIFFERENT_DPU_PROGRAMS = (
    DPU_ERR_NO_PROGRAM_LOADED +
    1)  # dpu_error.h: 92

DPU_ERR_CORRUPTED_MEMORY = (
    DPU_ERR_DIFFERENT_DPU_PROGRAMS +
    1)  # dpu_error.h: 92

DPU_ERR_DPU_DISABLED = (DPU_ERR_CORRUPTED_MEMORY + 1)  # dpu_error.h: 92

DPU_ERR_DPU_ALREADY_RUNNING = (DPU_ERR_DPU_DISABLED + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_MEMORY_TRANSFER = (
    DPU_ERR_DPU_ALREADY_RUNNING +
    1)  # dpu_error.h: 92

DPU_ERR_INVALID_LAUNCH_POLICY = (
    DPU_ERR_INVALID_MEMORY_TRANSFER +
    1)  # dpu_error.h: 92

DPU_ERR_DPU_FAULT = (DPU_ERR_INVALID_LAUNCH_POLICY + 1)  # dpu_error.h: 92

DPU_ERR_ELF_INVALID_FILE = (DPU_ERR_DPU_FAULT + 1)  # dpu_error.h: 92

DPU_ERR_ELF_NO_SUCH_FILE = (DPU_ERR_ELF_INVALID_FILE + 1)  # dpu_error.h: 92

DPU_ERR_ELF_NO_SUCH_SECTION = (DPU_ERR_ELF_NO_SUCH_FILE + 1)  # dpu_error.h: 92

DPU_ERR_TIMEOUT = (DPU_ERR_ELF_NO_SUCH_SECTION + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_PROFILE = (DPU_ERR_TIMEOUT + 1)  # dpu_error.h: 92

DPU_ERR_UNKNOWN_SYMBOL = (DPU_ERR_INVALID_PROFILE + 1)  # dpu_error.h: 92

DPU_ERR_LOG_FORMAT = (DPU_ERR_UNKNOWN_SYMBOL + 1)  # dpu_error.h: 92

DPU_ERR_LOG_CONTEXT_MISSING = (DPU_ERR_LOG_FORMAT + 1)  # dpu_error.h: 92

DPU_ERR_LOG_BUFFER_TOO_SMALL = (
    DPU_ERR_LOG_CONTEXT_MISSING +
    1)  # dpu_error.h: 92

DPU_ERR_VPD_INVALID_FILE = (
    DPU_ERR_LOG_BUFFER_TOO_SMALL +
    1)  # dpu_error.h: 92

DPU_ERR_VPD_NO_REPAIR = (DPU_ERR_VPD_INVALID_FILE + 1)  # dpu_error.h: 92

DPU_ERR_NO_THREAD_PER_RANK = (DPU_ERR_VPD_NO_REPAIR + 1)  # dpu_error.h: 92

DPU_ERR_INVALID_BUFFER_SIZE = (
    DPU_ERR_NO_THREAD_PER_RANK +
    1)  # dpu_error.h: 92

DPU_ERR_NONBLOCKING_SYNC_CALLBACK = (
    DPU_ERR_INVALID_BUFFER_SIZE +
    1)  # dpu_error.h: 92

DPU_ERR_ASYNC_JOBS = (1 << 31)  # dpu_error.h: 92

dpu_error_t = enum_dpu_error_t  # dpu_error.h: 92

# dpu_error.h: 100
if _libs["libdpu.so"].has("dpu_error_to_string", "cdecl"):
    dpu_error_to_string = _libs["libdpu.so"].get(
        "dpu_error_to_string", "cdecl")
    dpu_error_to_string.argtypes = [dpu_error_t]
    dpu_error_to_string.restype = c_char_p

dpu_rank_id_t = c_uint16  # dpu_types.h: 41

dpu_id_t = c_uint32  # dpu_types.h: 46

dpu_slice_id_t = c_uint8  # dpu_types.h: 51

dpu_member_id_t = c_uint8  # dpu_types.h: 56

dpu_group_id_t = c_uint8  # dpu_types.h: 61

dpu_thread_t = c_uint8  # dpu_types.h: 66

dpu_notify_bit_id_t = c_uint8  # dpu_types.h: 71

iram_addr_t = c_uint16  # dpu_types.h: 76

wram_addr_t = c_uint32  # dpu_types.h: 81

mram_addr_t = c_uint32  # dpu_types.h: 86

dpu_mem_max_addr_t = mram_addr_t  # dpu_types.h: 91

iram_size_t = c_uint16  # dpu_types.h: 96

wram_size_t = c_uint32  # dpu_types.h: 101

mram_size_t = c_uint32  # dpu_types.h: 106

dpu_mem_max_size_t = mram_size_t  # dpu_types.h: 111

dpuinstruction_t = c_uint64  # dpu_types.h: 116

dpuword_t = c_uint32  # dpu_types.h: 121

dpu_bitfield_t = c_uint8  # dpu_types.h: 126

dpu_ci_bitfield_t = c_uint8  # dpu_types.h: 137

enum__dpu_clock_division_t = c_int  # dpu_types.h: 147

DPU_CLOCK_DIV8 = 0  # dpu_types.h: 147

DPU_CLOCK_DIV4 = 4  # dpu_types.h: 147

DPU_CLOCK_DIV3 = 3  # dpu_types.h: 147

DPU_CLOCK_DIV2 = 8  # dpu_types.h: 147

dpu_clock_division_t = enum__dpu_clock_division_t  # dpu_types.h: 147

enum_dpu_slice_target_type = c_int  # dpu_types.h: 152

DPU_SLICE_TARGET_NONE = 0  # dpu_types.h: 152

DPU_SLICE_TARGET_DPU = (DPU_SLICE_TARGET_NONE + 1)  # dpu_types.h: 152

DPU_SLICE_TARGET_ALL = (DPU_SLICE_TARGET_DPU + 1)  # dpu_types.h: 152

DPU_SLICE_TARGET_GROUP = (DPU_SLICE_TARGET_ALL + 1)  # dpu_types.h: 152

NR_OF_DPU_SLICE_TARGETS = (DPU_SLICE_TARGET_GROUP + 1)  # dpu_types.h: 152

# dpu_types.h: 167


class union_anon_23(Union):
    pass


union_anon_23.__slots__ = [
    'dpu_id',
    'group_id',
]
union_anon_23._fields_ = [
    ('dpu_id', dpu_member_id_t),
    ('group_id', dpu_group_id_t),
]

# dpu_types.h: 163


class struct_dpu_slice_target(Structure):
    pass


struct_dpu_slice_target.__slots__ = [
    'type',
    'unnamed_1',
]
struct_dpu_slice_target._anonymous_ = [
    'unnamed_1',
]
struct_dpu_slice_target._fields_ = [
    ('type', enum_dpu_slice_target_type),
    ('unnamed_1', union_anon_23),
]

# dpu_types.h: 187
for _lib in _libs.values():
    try:
        dpu_slice_target_names = (
            POINTER(c_char) *
            int(NR_OF_DPU_SLICE_TARGETS)).in_dll(
            _lib,
            "dpu_slice_target_names")
        break
    except BaseException:
        pass

enum__dpu_event_kind_t = c_int  # dpu_types.h: 199

DPU_EVENT_RESET = 0  # dpu_types.h: 199

DPU_EVENT_EXTRACT_CONTEXT = (DPU_EVENT_RESET + 1)  # dpu_types.h: 199

DPU_EVENT_RESTORE_CONTEXT = (DPU_EVENT_EXTRACT_CONTEXT + 1)  # dpu_types.h: 199

DPU_EVENT_MRAM_ACCESS_PROGRAM = (
    DPU_EVENT_RESTORE_CONTEXT +
    1)  # dpu_types.h: 199

DPU_EVENT_LOAD_PROGRAM = (
    DPU_EVENT_MRAM_ACCESS_PROGRAM +
    1)  # dpu_types.h: 199

DPU_EVENT_DEBUG_ACTION = (DPU_EVENT_LOAD_PROGRAM + 1)  # dpu_types.h: 199

dpu_event_kind_t = enum__dpu_event_kind_t  # dpu_types.h: 199

# dpu_types.h: 216


class struct_dpu_rank_t(Structure):
    pass

# dpu_types.h: 221


class struct_dpu_t(Structure):
    pass

# dpu_types.h: 226


class struct_dpu_transfer_matrix(Structure):
    pass


enum__dpu_set_kind_t = c_int  # dpu_types.h: 236

DPU_SET_RANKS = 0  # dpu_types.h: 236

DPU_SET_DPU = (DPU_SET_RANKS + 1)  # dpu_types.h: 236

dpu_set_kind_t = enum__dpu_set_kind_t  # dpu_types.h: 236

# dpu_types.h: 247


class struct_anon_24(Structure):
    pass


struct_anon_24.__slots__ = [
    'nr_ranks',
    'ranks',
]
struct_anon_24._fields_ = [
    ('nr_ranks', c_uint32),
    ('ranks', POINTER(POINTER(struct_dpu_rank_t))),
]

# dpu_types.h: 245


class union_anon_25(Union):
    pass


union_anon_25.__slots__ = [
    'list',
    'dpu',
]
union_anon_25._fields_ = [
    ('list', struct_anon_24),
    ('dpu', POINTER(struct_dpu_t)),
]

# dpu_types.h: 241


class struct_dpu_set_t(Structure):
    pass


struct_dpu_set_t.__slots__ = [
    'kind',
    'unnamed_1',
]
struct_dpu_set_t._anonymous_ = [
    'unnamed_1',
]
struct_dpu_set_t._fields_ = [
    ('kind', dpu_set_kind_t),
    ('unnamed_1', union_anon_25),
]

# dpu_types.h: 261


class struct_dpu_program_t(Structure):
    pass

# dpu_types.h: 266


class struct_dpu_symbol_t(Structure):
    pass


struct_dpu_symbol_t.__slots__ = [
    'address',
    'size',
]
struct_dpu_symbol_t._fields_ = [
    ('address', dpu_mem_max_addr_t),
    ('size', dpu_mem_max_size_t),
]

# dpu_types.h: 276


class struct_dpu_incbin_t(Structure):
    pass


struct_dpu_incbin_t.__slots__ = [
    'buffer',
    'size',
    'path',
]
struct_dpu_incbin_t._fields_ = [
    ('buffer', POINTER(c_uint8)),
    ('size', c_size_t),
    ('path', String),
]

# dpu_types.h: 288


class struct_dpu_bit_config(Structure):
    pass


struct_dpu_bit_config.__slots__ = [
    'cpu2dpu',
    'dpu2cpu',
    'nibble_swap',
    'stutter',
]
struct_dpu_bit_config._fields_ = [
    ('cpu2dpu', c_uint16),
    ('dpu2cpu', c_uint16),
    ('nibble_swap', c_uint8),
    ('stutter', c_uint8),
]

# dpu_types.h: 302


class struct_dpu_carousel_config(Structure):
    pass


struct_dpu_carousel_config.__slots__ = [
    'cmd_duration',
    'cmd_sampling',
    'res_duration',
    'res_sampling',
]
struct_dpu_carousel_config._fields_ = [
    ('cmd_duration', c_uint8),
    ('cmd_sampling', c_uint8),
    ('res_duration', c_uint8),
    ('res_sampling', c_uint8),
]

# dpu_types.h: 316


class struct_dpu_repair_config(Structure):
    pass


struct_dpu_repair_config.__slots__ = [
    'AB_msbs',
    'CD_msbs',
    'A_lsbs',
    'B_lsbs',
    'C_lsbs',
    'D_lsbs',
    'even_index',
    'odd_index',
]
struct_dpu_repair_config._fields_ = [
    ('AB_msbs', c_uint8),
    ('CD_msbs', c_uint8),
    ('A_lsbs', c_uint8),
    ('B_lsbs', c_uint8),
    ('C_lsbs', c_uint8),
    ('D_lsbs', c_uint8),
    ('even_index', c_uint8),
    ('odd_index', c_uint8),
]

enum_dpu_temperature = c_int  # dpu_types.h: 338

DPU_TEMPERATURE_LESS_THAN_50 = 0  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_50_AND_60 = 1  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_60_AND_70 = 2  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_70_AND_80 = 3  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_80_AND_90 = 4  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_90_AND_100 = 5  # dpu_types.h: 338

DPU_TEMPERATURE_BETWEEN_100_AND_110 = 6  # dpu_types.h: 338

DPU_TEMPERATURE_GREATER_THAN_110 = 7  # dpu_types.h: 338

enum_dpu_pc_mode = c_int  # dpu_types.h: 352

DPU_PC_MODE_12 = 0  # dpu_types.h: 352

DPU_PC_MODE_13 = 1  # dpu_types.h: 352

DPU_PC_MODE_14 = 2  # dpu_types.h: 352

DPU_PC_MODE_15 = 3  # dpu_types.h: 352

DPU_PC_MODE_16 = 4  # dpu_types.h: 352

# dpu_types.h: 370


class struct_anon_26(Structure):
    pass


struct_anon_26.__slots__ = [
    'nr_threads',
    'nr_registers',
    'nr_atomic_bits',
    'iram_size',
    'mram_size',
    'wram_size',
]
struct_anon_26._fields_ = [
    ('nr_threads', c_uint32),
    ('nr_registers', c_uint32),
    ('nr_atomic_bits', c_uint32),
    ('iram_size', iram_size_t),
    ('mram_size', mram_size_t),
    ('wram_size', wram_size_t),
]

# dpu_types.h: 368


class struct_dpu_context_t(Structure):
    pass


struct_dpu_context_t.__slots__ = [
    'info',
    'iram',
    'mram',
    'wram',
    'registers',
    'pcs',
    'atomic_register',
    'zero_flags',
    'carry_flags',
    'nr_of_running_threads',
    'scheduling',
    'bkp_fault',
    'dma_fault',
    'mem_fault',
    'bkp_fault_thread_index',
    'dma_fault_thread_index',
    'mem_fault_thread_index',
    'bkp_fault_id',
]
struct_dpu_context_t._fields_ = [
    ('info', struct_anon_26),
    ('iram', POINTER(dpuinstruction_t)),
    ('mram', POINTER(c_uint8)),
    ('wram', POINTER(dpuword_t)),
    ('registers', POINTER(c_uint32)),
    ('pcs', POINTER(iram_addr_t)),
    ('atomic_register', POINTER(c_bool)),
    ('zero_flags', POINTER(c_bool)),
    ('carry_flags', POINTER(c_bool)),
    ('nr_of_running_threads', c_uint8),
    ('scheduling', POINTER(c_uint8)),
    ('bkp_fault', c_bool),
    ('dma_fault', c_bool),
    ('mem_fault', c_bool),
    ('bkp_fault_thread_index', dpu_thread_t),
    ('dma_fault_thread_index', dpu_thread_t),
    ('mem_fault_thread_index', dpu_thread_t),
    ('bkp_fault_id', c_uint32),
]

enum__dpu_checkpoint_flags_t = c_int  # dpu_checkpoint.h: 30

DPU_CHECKPOINT_NONE = 0  # dpu_checkpoint.h: 30

DPU_CHECKPOINT_INTERNAL = 1  # dpu_checkpoint.h: 30

DPU_CHECKPOINT_IRAM = 2  # dpu_checkpoint.h: 30

DPU_CHECKPOINT_MRAM = 8  # dpu_checkpoint.h: 30

DPU_CHECKPOINT_WRAM = 16  # dpu_checkpoint.h: 30

dpu_checkpoint_flags_t = enum__dpu_checkpoint_flags_t  # dpu_checkpoint.h: 30

# dpu_checkpoint.h: 46
if _libs["libdpu.so"].has("dpu_checkpoint_save", "cdecl"):
    dpu_checkpoint_save = _libs["libdpu.so"].get(
        "dpu_checkpoint_save", "cdecl")
    dpu_checkpoint_save.argtypes = [
        struct_dpu_set_t,
        dpu_checkpoint_flags_t,
        POINTER(struct_dpu_context_t)]
    dpu_checkpoint_save.restype = dpu_error_t

# dpu_checkpoint.h: 56
if _libs["libdpu.so"].has("dpu_checkpoint_restore", "cdecl"):
    dpu_checkpoint_restore = _libs["libdpu.so"].get(
        "dpu_checkpoint_restore", "cdecl")
    dpu_checkpoint_restore.argtypes = [
        struct_dpu_set_t,
        dpu_checkpoint_flags_t,
        POINTER(struct_dpu_context_t)]
    dpu_checkpoint_restore.restype = dpu_error_t

# dpu_checkpoint.h: 64
if _libs["libdpu.so"].has(
    "dpu_checkpoint_get_serialized_context_size",
        "cdecl"):
    dpu_checkpoint_get_serialized_context_size = _libs["libdpu.so"].get(
        "dpu_checkpoint_get_serialized_context_size", "cdecl")
    dpu_checkpoint_get_serialized_context_size.argtypes = [
        POINTER(struct_dpu_context_t)]
    dpu_checkpoint_get_serialized_context_size.restype = c_uint32

# dpu_checkpoint.h: 74
if _libs["libdpu.so"].has("dpu_checkpoint_serialize", "cdecl"):
    dpu_checkpoint_serialize = _libs["libdpu.so"].get(
        "dpu_checkpoint_serialize", "cdecl")
    dpu_checkpoint_serialize.argtypes = [
        POINTER(struct_dpu_context_t), POINTER(
            POINTER(c_uint8)), POINTER(c_uint32)]
    dpu_checkpoint_serialize.restype = dpu_error_t

# dpu_checkpoint.h: 84
if _libs["libdpu.so"].has("dpu_checkpoint_deserialize", "cdecl"):
    dpu_checkpoint_deserialize = _libs["libdpu.so"].get(
        "dpu_checkpoint_deserialize", "cdecl")
    dpu_checkpoint_deserialize.argtypes = [
        POINTER(c_uint8), c_uint32, POINTER(struct_dpu_context_t)]
    dpu_checkpoint_deserialize.restype = dpu_error_t

# dpu_checkpoint.h: 92
if _libs["libdpu.so"].has("dpu_checkpoint_free", "cdecl"):
    dpu_checkpoint_free = _libs["libdpu.so"].get(
        "dpu_checkpoint_free", "cdecl")
    dpu_checkpoint_free.argtypes = [POINTER(struct_dpu_context_t)]
    dpu_checkpoint_free.restype = dpu_error_t

enum__dpu_launch_policy_t = c_int  # dpu.h: 57

DPU_ASYNCHRONOUS = 0  # dpu.h: 57

DPU_SYNCHRONOUS = (DPU_ASYNCHRONOUS + 1)  # dpu.h: 57

dpu_launch_policy_t = enum__dpu_launch_policy_t  # dpu.h: 57

enum__dpu_xfer_t = c_int  # dpu.h: 67

DPU_XFER_TO_DPU = 0  # dpu.h: 67

DPU_XFER_FROM_DPU = (DPU_XFER_TO_DPU + 1)  # dpu.h: 67

dpu_xfer_t = enum__dpu_xfer_t  # dpu.h: 67

enum__dpu_xfer_flags_t = c_int  # dpu.h: 82

DPU_XFER_DEFAULT = 0  # dpu.h: 82

DPU_XFER_NO_RESET = (1 << 0)  # dpu.h: 82

DPU_XFER_ASYNC = (1 << 1)  # dpu.h: 82

dpu_xfer_flags_t = enum__dpu_xfer_flags_t  # dpu.h: 82

enum__dpu_callback_flags_t = c_int  # dpu.h: 102

DPU_CALLBACK_DEFAULT = 0  # dpu.h: 102

DPU_CALLBACK_ASYNC = (1 << 0)  # dpu.h: 102

DPU_CALLBACK_NONBLOCKING = (1 << 1)  # dpu.h: 102

DPU_CALLBACK_SINGLE_CALL = (1 << 2)  # dpu.h: 102

dpu_callback_flags_t = enum__dpu_callback_flags_t  # dpu.h: 102

# dpu.h: 144
if _libs["libdpu.so"].has("dpu_alloc", "cdecl"):
    dpu_alloc = _libs["libdpu.so"].get("dpu_alloc", "cdecl")
    dpu_alloc.argtypes = [c_uint32, String, POINTER(struct_dpu_set_t)]
    dpu_alloc.restype = dpu_error_t

# dpu.h: 158
if _libs["libdpu.so"].has("dpu_alloc_ranks", "cdecl"):
    dpu_alloc_ranks = _libs["libdpu.so"].get("dpu_alloc_ranks", "cdecl")
    dpu_alloc_ranks.argtypes = [c_uint32, String, POINTER(struct_dpu_set_t)]
    dpu_alloc_ranks.restype = dpu_error_t

# dpu.h: 169
if _libs["libdpu.so"].has("dpu_free", "cdecl"):
    dpu_free = _libs["libdpu.so"].get("dpu_free", "cdecl")
    dpu_free.argtypes = [struct_dpu_set_t]
    dpu_free.restype = dpu_error_t

# dpu.h: 178
if _libs["libdpu.so"].has("dpu_get_nr_ranks", "cdecl"):
    dpu_get_nr_ranks = _libs["libdpu.so"].get("dpu_get_nr_ranks", "cdecl")
    dpu_get_nr_ranks.argtypes = [struct_dpu_set_t, POINTER(c_uint32)]
    dpu_get_nr_ranks.restype = dpu_error_t

# dpu.h: 187
if _libs["libdpu.so"].has("dpu_get_nr_dpus", "cdecl"):
    dpu_get_nr_dpus = _libs["libdpu.so"].get("dpu_get_nr_dpus", "cdecl")
    dpu_get_nr_dpus.argtypes = [struct_dpu_set_t, POINTER(c_uint32)]
    dpu_get_nr_dpus.restype = dpu_error_t

# dpu.h: 250


class struct_dpu_set_rank_iterator_t(Structure):
    pass


struct_dpu_set_rank_iterator_t.__slots__ = [
    'set',
    'count',
    'next_idx',
    'has_next',
    'next',
]
struct_dpu_set_rank_iterator_t._fields_ = [
    ('set', POINTER(struct_dpu_set_t)),
    ('count', c_uint32),
    ('next_idx', c_uint32),
    ('has_next', c_bool),
    ('next', struct_dpu_set_t),
]

# dpu.h: 264


class struct_dpu_set_dpu_iterator_t(Structure):
    pass


struct_dpu_set_dpu_iterator_t.__slots__ = [
    'rank_iterator',
    'count',
    'next_idx',
    'has_next',
    'next',
]
struct_dpu_set_dpu_iterator_t._fields_ = [
    ('rank_iterator', struct_dpu_set_rank_iterator_t),
    ('count', c_uint32),
    ('next_idx', c_uint32),
    ('has_next', c_bool),
    ('next', struct_dpu_set_t),
]

# dpu.h: 282
if _libs["libdpu.so"].has("dpu_set_rank_iterator_from", "cdecl"):
    dpu_set_rank_iterator_from = _libs["libdpu.so"].get(
        "dpu_set_rank_iterator_from", "cdecl")
    dpu_set_rank_iterator_from.argtypes = [POINTER(struct_dpu_set_t)]
    dpu_set_rank_iterator_from.restype = struct_dpu_set_rank_iterator_t

# dpu.h: 293
if _libs["libdpu.so"].has("dpu_set_rank_iterator_next", "cdecl"):
    dpu_set_rank_iterator_next = _libs["libdpu.so"].get(
        "dpu_set_rank_iterator_next", "cdecl")
    dpu_set_rank_iterator_next.argtypes = [
        POINTER(struct_dpu_set_rank_iterator_t)]
    dpu_set_rank_iterator_next.restype = None

# dpu.h: 305
if _libs["libdpu.so"].has("dpu_set_dpu_iterator_from", "cdecl"):
    dpu_set_dpu_iterator_from = _libs["libdpu.so"].get(
        "dpu_set_dpu_iterator_from", "cdecl")
    dpu_set_dpu_iterator_from.argtypes = [POINTER(struct_dpu_set_t)]
    dpu_set_dpu_iterator_from.restype = struct_dpu_set_dpu_iterator_t

# dpu.h: 316
if _libs["libdpu.so"].has("dpu_set_dpu_iterator_next", "cdecl"):
    dpu_set_dpu_iterator_next = _libs["libdpu.so"].get(
        "dpu_set_dpu_iterator_next", "cdecl")
    dpu_set_dpu_iterator_next.argtypes = [
        POINTER(struct_dpu_set_dpu_iterator_t)]
    dpu_set_dpu_iterator_next.restype = None

# dpu.h: 328
if _libs["libdpu.so"].has("dpu_load_from_memory", "cdecl"):
    dpu_load_from_memory = _libs["libdpu.so"].get(
        "dpu_load_from_memory", "cdecl")
    dpu_load_from_memory.argtypes = [
        struct_dpu_set_t, POINTER(c_uint8), c_size_t, POINTER(
            POINTER(struct_dpu_program_t))]
    dpu_load_from_memory.restype = dpu_error_t

# dpu.h: 339
if _libs["libdpu.so"].has("dpu_load_from_incbin", "cdecl"):
    dpu_load_from_incbin = _libs["libdpu.so"].get(
        "dpu_load_from_incbin", "cdecl")
    dpu_load_from_incbin.argtypes = [
        struct_dpu_set_t, POINTER(struct_dpu_incbin_t), POINTER(
            POINTER(struct_dpu_program_t))]
    dpu_load_from_incbin.restype = dpu_error_t

# dpu.h: 350
if _libs["libdpu.so"].has("dpu_load", "cdecl"):
    dpu_load = _libs["libdpu.so"].get("dpu_load", "cdecl")
    dpu_load.argtypes = [
        struct_dpu_set_t, String, POINTER(
            POINTER(struct_dpu_program_t))]
    dpu_load.restype = dpu_error_t

# dpu.h: 360
if _libs["libdpu.so"].has("dpu_get_symbol", "cdecl"):
    dpu_get_symbol = _libs["libdpu.so"].get("dpu_get_symbol", "cdecl")
    dpu_get_symbol.argtypes = [
        POINTER(struct_dpu_program_t),
        String,
        POINTER(struct_dpu_symbol_t)]
    dpu_get_symbol.restype = dpu_error_t

# dpu.h: 369
if _libs["libdpu.so"].has("dpu_launch", "cdecl"):
    dpu_launch = _libs["libdpu.so"].get("dpu_launch", "cdecl")
    dpu_launch.argtypes = [struct_dpu_set_t, dpu_launch_policy_t]
    dpu_launch.restype = dpu_error_t

# dpu.h: 379
if _libs["libdpu.so"].has("dpu_status", "cdecl"):
    dpu_status = _libs["libdpu.so"].get("dpu_status", "cdecl")
    dpu_status.argtypes = [struct_dpu_set_t, POINTER(c_bool), POINTER(c_bool)]
    dpu_status.restype = dpu_error_t

# dpu.h: 387
if _libs["libdpu.so"].has("dpu_sync", "cdecl"):
    dpu_sync = _libs["libdpu.so"].get("dpu_sync", "cdecl")
    dpu_sync.argtypes = [struct_dpu_set_t]
    dpu_sync.restype = dpu_error_t

# dpu.h: 399
if _libs["libdpu.so"].has("dpu_copy_to", "cdecl"):
    dpu_copy_to = _libs["libdpu.so"].get("dpu_copy_to", "cdecl")
    dpu_copy_to.argtypes = [
        struct_dpu_set_t,
        String,
        c_uint32,
        POINTER(None),
        c_size_t]
    dpu_copy_to.restype = dpu_error_t

# dpu.h: 411
if _libs["libdpu.so"].has("dpu_copy_from", "cdecl"):
    dpu_copy_from = _libs["libdpu.so"].get("dpu_copy_from", "cdecl")
    dpu_copy_from.argtypes = [
        struct_dpu_set_t,
        String,
        c_uint32,
        POINTER(None),
        c_size_t]
    dpu_copy_from.restype = dpu_error_t

# dpu.h: 423
if _libs["libdpu.so"].has("dpu_copy_to_symbol", "cdecl"):
    dpu_copy_to_symbol = _libs["libdpu.so"].get("dpu_copy_to_symbol", "cdecl")
    dpu_copy_to_symbol.argtypes = [
        struct_dpu_set_t,
        struct_dpu_symbol_t,
        c_uint32,
        POINTER(None),
        c_size_t]
    dpu_copy_to_symbol.restype = dpu_error_t

# dpu.h: 435
if _libs["libdpu.so"].has("dpu_copy_from_symbol", "cdecl"):
    dpu_copy_from_symbol = _libs["libdpu.so"].get(
        "dpu_copy_from_symbol", "cdecl")
    dpu_copy_from_symbol.argtypes = [
        struct_dpu_set_t,
        struct_dpu_symbol_t,
        c_uint32,
        POINTER(None),
        c_size_t]
    dpu_copy_from_symbol.restype = dpu_error_t

# dpu.h: 448
if _libs["libdpu.so"].has("dpu_prepare_xfer", "cdecl"):
    dpu_prepare_xfer = _libs["libdpu.so"].get("dpu_prepare_xfer", "cdecl")
    dpu_prepare_xfer.argtypes = [struct_dpu_set_t, POINTER(None)]
    dpu_prepare_xfer.restype = dpu_error_t

# dpu.h: 466
if _libs["libdpu.so"].has("dpu_push_xfer", "cdecl"):
    dpu_push_xfer = _libs["libdpu.so"].get("dpu_push_xfer", "cdecl")
    dpu_push_xfer.argtypes = [
        struct_dpu_set_t,
        dpu_xfer_t,
        String,
        c_uint32,
        c_size_t,
        dpu_xfer_flags_t]
    dpu_push_xfer.restype = dpu_error_t

# dpu.h: 489
if _libs["libdpu.so"].has("dpu_push_xfer_symbol", "cdecl"):
    dpu_push_xfer_symbol = _libs["libdpu.so"].get(
        "dpu_push_xfer_symbol", "cdecl")
    dpu_push_xfer_symbol.argtypes = [
        struct_dpu_set_t,
        dpu_xfer_t,
        struct_dpu_symbol_t,
        c_uint32,
        c_size_t,
        dpu_xfer_flags_t]
    dpu_push_xfer_symbol.restype = dpu_error_t

# dpu.h: 508
if _libs["libdpu.so"].has("dpu_broadcast_to", "cdecl"):
    dpu_broadcast_to = _libs["libdpu.so"].get("dpu_broadcast_to", "cdecl")
    dpu_broadcast_to.argtypes = [
        struct_dpu_set_t,
        String,
        c_uint32,
        POINTER(None),
        c_size_t,
        dpu_xfer_flags_t]
    dpu_broadcast_to.restype = dpu_error_t

# dpu.h: 527
if _libs["libdpu.so"].has("dpu_broadcast_to_symbol", "cdecl"):
    dpu_broadcast_to_symbol = _libs["libdpu.so"].get(
        "dpu_broadcast_to_symbol", "cdecl")
    dpu_broadcast_to_symbol.argtypes = [
        struct_dpu_set_t,
        struct_dpu_symbol_t,
        c_uint32,
        POINTER(None),
        c_size_t,
        dpu_xfer_flags_t]
    dpu_broadcast_to_symbol.restype = dpu_error_t

# dpu.h: 544
if _libs["libdpu.so"].has("dpu_callback", "cdecl"):
    dpu_callback = _libs["libdpu.so"].get("dpu_callback", "cdecl")
    dpu_callback.argtypes = [
        struct_dpu_set_t,
        CFUNCTYPE(
            UNCHECKED(dpu_error_t),
            struct_dpu_set_t,
            c_uint32,
            POINTER(None)),
        POINTER(None),
        dpu_callback_flags_t]
    dpu_callback.restype = dpu_error_t

# dpu.h: 582
if _libs["libdpu.so"].has("dpu_log_read", "cdecl"):
    dpu_log_read = _libs["libdpu.so"].get("dpu_log_read", "cdecl")
    dpu_log_read.argtypes = [struct_dpu_set_t, POINTER(FILE)]
    dpu_log_read.restype = dpu_error_t

# /usr/include/limits.h: 84
try:
    UINT_MAX = 4294967295
except BaseException:
    pass

# dpu_error.h: 107
try:
    DPU_ERROR_ASYNC_JOB_TYPE_SHIFT = 16
except BaseException:
    pass

# dpu_types.h: 21
try:
    DPU_MAX_NR_CIS = 8
except BaseException:
    pass

# dpu_types.h: 26
try:
    DPU_MAX_NR_DPUS_PER_CI = 8
except BaseException:
    pass

# dpu_types.h: 31
try:
    DPU_BOOT_THREAD = 0
except BaseException:
    pass

# dpu_types.h: 36
try:
    DPU_MAX_NR_GROUPS = 8
except BaseException:
    pass

# dpu_types.h: 132
try:
    DPU_BITFIELD_ALL = (dpu_bitfield_t(ord_if_char((-1)))).value
except BaseException:
    pass

# dpu_types.h: 180


def DPU_SLICE_TARGET_TYPE_NAME(target_type):
    return ((c_uint32(ord_if_char(target_type))).value < NR_OF_DPU_SLICE_TARGETS) and (
        dpu_slice_target_names[target_type]) or 'DPU_SLICE_TARGET_TYPE_UNKNOWN'


# dpu_types.h: 211
try:
    DPU_MRAM_HEAP_POINTER_NAME = '__sys_used_mram_end'
except BaseException:
    pass

# dpu_checkpoint.h: 36
try:
    DPU_CHECKPOINT_ALL = (((DPU_CHECKPOINT_INTERNAL | DPU_CHECKPOINT_IRAM)
                          | DPU_CHECKPOINT_MRAM) | DPU_CHECKPOINT_WRAM)
except BaseException:
    pass

# dpu.h: 130
try:
    DPU_ALLOCATE_ALL = UINT_MAX
except BaseException:
    pass

dpu_slice_target = struct_dpu_slice_target  # dpu_types.h: 163

dpu_rank_t = struct_dpu_rank_t  # dpu_types.h: 216

dpu_t = struct_dpu_t  # dpu_types.h: 221

dpu_transfer_matrix = struct_dpu_transfer_matrix  # dpu_types.h: 226

dpu_set_t = struct_dpu_set_t  # dpu_types.h: 241

dpu_program_t = struct_dpu_program_t  # dpu_types.h: 261

dpu_symbol_t = struct_dpu_symbol_t  # dpu_types.h: 266

dpu_incbin_t = struct_dpu_incbin_t  # dpu_types.h: 276

dpu_bit_config = struct_dpu_bit_config  # dpu_types.h: 288

dpu_carousel_config = struct_dpu_carousel_config  # dpu_types.h: 302

dpu_repair_config = struct_dpu_repair_config  # dpu_types.h: 316

dpu_context_t = struct_dpu_context_t  # dpu_types.h: 368

dpu_set_rank_iterator_t = struct_dpu_set_rank_iterator_t  # dpu.h: 250

dpu_set_dpu_iterator_t = struct_dpu_set_dpu_iterator_t  # dpu.h: 264

# Begin inserted files
# Begin "<backends>/python/utils/ffi_footer.txt"

# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if _libs["libdpu.so"].has("Py_dpu_log_read", "cdecl"):
    Py_dpu_log_read = _libs["libdpu.so"].get("Py_dpu_log_read", "cdecl")
    Py_dpu_log_read.argtypes = [struct_dpu_set_t, ctypes.py_object]
    Py_dpu_log_read.restype = dpu_error_t

if _libs["libdpu.so"].has("Py_dpu_prepare_xfers", "cdecl"):
    Py_dpu_prepare_xfers = _libs["libdpu.so"].get(
        "Py_dpu_prepare_xfers", "cdecl")
    Py_dpu_prepare_xfers.argtypes = [
        struct_dpu_set_t, py_object, POINTER(c_size_t)]
    Py_dpu_prepare_xfers.restype = dpu_error_t

if _libs["libdpu.so"].has("Py_dpu_get_symbol_names", "cdecl"):
    Py_dpu_get_symbol_names = _libs["libdpu.so"].get(
        "Py_dpu_get_symbol_names", "cdecl")
    Py_dpu_get_symbol_names.argtypes = [POINTER(struct_dpu_program_t)]
    Py_dpu_get_symbol_names.restype = py_object

# End "<backends>/python/utils/ffi_footer.txt"

# 1 inserted files
# End inserted files

# No prefix-stripping
