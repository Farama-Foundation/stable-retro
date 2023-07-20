# Copyright (c) 2013-2016 Jeffrey Pfau
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
from ._pylib import ffi, lib


class MemoryView:
    def __init__(self, core, width, size, base=0, sign="u"):
        self._core = core
        self._width = width
        self._size = size
        self._base = base
        self._busRead = getattr(self._core, "busRead" + str(width * 8))
        self._busWrite = getattr(self._core, "busWrite" + str(width * 8))
        self._rawRead = getattr(self._core, "rawRead" + str(width * 8))
        self._rawWrite = getattr(self._core, "rawWrite" + str(width * 8))
        self._mask = (
            1 << (width * 8)
        ) - 1  # Used to force values to fit within range so that negative values work
        if sign == "u" or sign == "unsigned":
            self._type = f"uint{width * 8}_t"
        elif sign == "i" or sign == "s" or sign == "signed":
            self._type = f"int{width * 8}_t"
        else:
            raise ValueError(f"Invalid sign type: '{sign}'")

    def _addrCheck(self, address):
        if isinstance(address, slice):
            start = address.start or 0
            stop = self._size - self._width if address.stop is None else address.stop
        else:
            start = address
            stop = address + self._width
        if start >= self._size or stop > self._size:
            raise IndexError()
        if start < 0 or stop < 0:
            raise IndexError()

    def __len__(self):
        return self._size

    def __getitem__(self, address):
        self._addrCheck(address)
        if isinstance(address, slice):
            start = address.start or 0
            stop = self._size - self._width if address.stop is None else address.stop
            step = address.step or self._width
            return [
                int(ffi.cast(self._type, self._busRead(self._core, self._base + a)))
                for a in range(start, stop, step)
            ]
        else:
            return int(
                ffi.cast(self._type, self._busRead(self._core, self._base + address))
            )

    def __setitem__(self, address, value):
        self._addrCheck(address)
        if isinstance(address, slice):
            start = address.start or 0
            stop = self._size - self._width if address.stop is None else address.stop
            step = address.step or self._width
            for a in range(start, stop, step):
                self._busWrite(self._core, self._base + a, value[a] & self._mask)
        else:
            self._busWrite(self._core, self._base + address, value & self._mask)

    def rawRead(self, address, segment=-1):
        self._addrCheck(address)
        return int(
            ffi.cast(
                self._type, self._rawRead(self._core, self._base + address, segment)
            )
        )

    def rawWrite(self, address, value, segment=-1):
        self._addrCheck(address)
        self._rawWrite(self._core, self._base + address, segment, value & self._mask)


class MemorySearchResult:
    def __init__(self, memory, result):
        self.address = result.address
        self.segment = result.segment
        self.guessDivisor = result.guessDivisor
        self.type = result.type

        if result.type == Memory.SEARCH_8:
            self._memory = memory.u8
        elif result.type == Memory.SEARCH_16:
            self._memory = memory.u16
        elif result.type == Memory.SEARCH_32:
            self._memory = memory.u32
        elif result.type == Memory.SEARCH_STRING:
            self._memory = memory.u8
        else:
            raise ValueError("Unknown type: %X" % result.type)

    @property
    def value(self):
        if self.type == Memory.SEARCH_STRING:
            raise ValueError
        return self._memory[self.address] * self.guessDivisor

    @value.setter
    def value(self, v):
        if self.type == Memory.SEARCH_STRING:
            raise IndexError
        self._memory[self.address] = v // self.guessDivisor


class Memory:
    SEARCH_INT = lib.mCORE_MEMORY_SEARCH_INT
    SEARCH_STRING = lib.mCORE_MEMORY_SEARCH_STRING
    SEARCH_GUESS = lib.mCORE_MEMORY_SEARCH_GUESS

    SEARCH_EQUAL = lib.mCORE_MEMORY_SEARCH_EQUAL

    READ = lib.mCORE_MEMORY_READ
    WRITE = lib.mCORE_MEMORY_READ
    RW = lib.mCORE_MEMORY_RW

    def __init__(self, core, size, base=0):
        self.size = size
        self.base = base
        self._core = core

        self.u8 = MemoryView(core, 1, size, base, "u")
        self.u16 = MemoryView(core, 2, size, base, "u")
        self.u32 = MemoryView(core, 4, size, base, "u")
        self.s8 = MemoryView(core, 1, size, base, "s")
        self.s16 = MemoryView(core, 2, size, base, "s")
        self.s32 = MemoryView(core, 4, size, base, "s")

    def __len__(self):
        return self._size

    def search(self, value, type=SEARCH_GUESS, flags=RW, limit=10000, old_results=[]):
        results = ffi.new("struct mCoreMemorySearchResults*")
        lib.mCoreMemorySearchResultsInit(results, len(old_results))
        params = ffi.new("struct mCoreMemorySearchParams*")
        params.memoryFlags = flags
        params.type = type
        params.op = self.SEARCH_EQUAL
        if type == self.SEARCH_INT:
            params.valueInt = int(value)
        else:
            params.valueStr = ffi.new("char[]", str(value).encode("ascii"))

        for result in old_results:
            r = lib.mCoreMemorySearchResultsAppend(results)
            r.address = result.address
            r.segment = result.segment
            r.guessDivisor = result.guessDivisor
            r.type = result.type
        if old_results:
            lib.mCoreMemorySearchRepeat(self._core, params, results)
        else:
            lib.mCoreMemorySearch(self._core, params, results, limit)
        new_results = [
            MemorySearchResult(self, lib.mCoreMemorySearchResultsGetPointer(results, i))
            for i in range(lib.mCoreMemorySearchResultsSize(results))
        ]
        lib.mCoreMemorySearchResultsDeinit(results)
        return new_results

    def __getitem__(self, address):
        if isinstance(address, slice):
            return bytearray(self.u8[address])
        else:
            return self.u8[address]
