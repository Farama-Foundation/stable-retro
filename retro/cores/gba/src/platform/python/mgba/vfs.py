# Copyright (c) 2013-2016 Jeffrey Pfau
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
import os

from ._pylib import ffi, lib


@ffi.def_extern()
def _vfpClose(vf):
    vfp = ffi.cast("struct VFilePy*", vf)
    ffi.from_handle(vfp.fileobj).close()
    return True


@ffi.def_extern()
def _vfpSeek(vf, offset, whence):
    vfp = ffi.cast("struct VFilePy*", vf)
    f = ffi.from_handle(vfp.fileobj)
    f.seek(offset, whence)
    return f.tell()


@ffi.def_extern()
def _vfpRead(vf, buffer, size):
    vfp = ffi.cast("struct VFilePy*", vf)
    pybuf = ffi.buffer(buffer, size)
    ffi.from_handle(vfp.fileobj).readinto(pybuf)
    return size


@ffi.def_extern()
def _vfpWrite(vf, buffer, size):
    vfp = ffi.cast("struct VFilePy*", vf)
    pybuf = ffi.buffer(buffer, size)
    ffi.from_handle(vfp.fileobj).write(pybuf)
    return size


@ffi.def_extern()
def _vfpMap(vf, size, flags):
    pass


@ffi.def_extern()
def _vfpUnmap(vf, memory, size):
    pass


@ffi.def_extern()
def _vfpTruncate(vf, size):
    vfp = ffi.cast("struct VFilePy*", vf)
    ffi.from_handle(vfp.fileobj).truncate(size)


@ffi.def_extern()
def _vfpSize(vf):
    vfp = ffi.cast("struct VFilePy*", vf)
    f = ffi.from_handle(vfp.fileobj)
    pos = f.tell()
    f.seek(0, os.SEEK_END)
    size = f.tell()
    f.seek(pos, os.SEEK_SET)
    return size


@ffi.def_extern()
def _vfpSync(vf, buffer, size):
    vfp = ffi.cast("struct VFilePy*", vf)
    f = ffi.from_handle(vfp.fileobj)
    if buffer and size:
        pos = f.tell()
        f.seek(0, os.SEEK_SET)
        _vfpWrite(vf, buffer, size)
        f.seek(pos, os.SEEK_SET)
    f.flush()
    os.fsync()
    return True


def open(f):
    handle = ffi.new_handle(f)
    vf = VFile(lib.VFileFromPython(handle))
    # Prevent garbage collection
    vf._fileobj = f
    vf._handle = handle
    return vf


def openPath(path, mode="r"):
    flags = 0
    if mode.startswith("r"):
        flags |= os.O_RDONLY
    elif mode.startswith("w"):
        flags |= os.O_WRONLY | os.O_CREAT | os.O_TRUNC
    elif mode.startswith("a"):
        flags |= os.O_WRONLY | os.O_CREAT | os.O_APPEND
    else:
        return None

    if "+" in mode[1:]:
        flags |= os.O_RDWR
    if "x" in mode[1:]:
        flags |= os.O_EXCL

    vf = lib.VFileOpen(path.encode("UTF-8"), flags)
    if vf == ffi.NULL:
        return None
    return VFile(vf)


class VFile:
    def __init__(self, vf):
        self.handle = vf

    def close(self):
        return bool(self.handle.close(self.handle))

    def seek(self, offset, whence):
        return self.handle.seek(self.handle, offset, whence)

    def read(self, buffer, size):
        return self.handle.read(self.handle, buffer, size)

    def readAll(self, size=0):
        if not size:
            size = self.size()
        buffer = ffi.new("char[%i]" % size)
        size = self.handle.read(self.handle, buffer, size)
        return ffi.unpack(buffer, size)

    def readline(self, buffer, size):
        return self.handle.readline(self.handle, buffer, size)

    def write(self, buffer, size):
        return self.handle.write(self.handle, buffer, size)

    def map(self, size, flags):
        return self.handle.map(self.handle, size, flags)

    def unmap(self, memory, size):
        self.handle.unmap(self.handle, memory, size)

    def truncate(self, size):
        self.handle.truncate(self.handle, size)

    def size(self):
        return self.handle.size(self.handle)

    def sync(self, buffer, size):
        return self.handle.sync(self.handle, buffer, size)
