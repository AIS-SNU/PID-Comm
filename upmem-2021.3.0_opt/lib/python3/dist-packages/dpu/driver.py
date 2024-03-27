# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Features to drive DPUs"""

import array
import io
import os
import struct
import sys
import tempfile
import threading

import dpu.compiler as compiler
import dpu.ffi as ffi
from .ffi import DPU_ALLOCATE_ALL as ALLOCATE_ALL

from typing import Any, Callable, Dict, Iterator, List, Optional, Sequence, Tuple, TypeVar, Union

T = TypeVar('T')
OneOrMore = Union[T, List[T]]

Buffer = Union[bytes, bytearray, memoryview, array.array]
DpuCopyBuffer = Union[str, 'DpuSymbol', Buffer, List[Optional[Buffer]]]
DpuConstructorLogObject = OneOrMore[io.IOBase]
DpuExecLogObject = Union[OneOrMore[io.IOBase], Dict['DpuSet', io.IOBase]]
DpuBinaryProgram = Union[bytes, str]
DpuSourceFile = Union[io.IOBase, str]
DpuSourceProgram = OneOrMore[DpuSourceFile]
DpuCallbackFunction = Callable[['DpuSet', int, Any], None]

_C_CALLBACK_FN_TYPE = ffi.CFUNCTYPE(
    ffi.UNCHECKED(
        ffi.dpu_error_t),
    ffi.struct_dpu_set_t,
    ffi.c_uint32,
    ffi.POINTER(None))


class _CallbackContext(ffi.Structure):
    _fields_ = [("fn", ffi.py_object), ("arg", ffi.py_object), ("single_call",
                                                                ffi.c_bool), ("ctx_id", ffi.c_uint32), ("dpu_set", ffi.py_object)]


class DpuError(Exception):
    """Error raised by the API"""
    pass


class DpuThreadContext():
    """
    Thread-specific context from a DpuContext.

    Attributes:
        idx (int): the thread index
    """

    def __init__(self, ctx: 'DpuContext', idx: int):
        self.inner = ctx
        self.idx = idx

    def regs(self) -> List[int]:
        """
        Returns:
            List[int]: a copy of the thread work register values
        """
        length = self.inner.c_ctx.info.nr_registers
        start = self.idx * length
        return list(self.inner.c_ctx.registers[start:start + length])

    def pc(self) -> int:
        """
        Returns:
            int: the thread Program Counter
        """
        return self.inner.c_ctx.pcs[self.idx]

    def zf(self) -> bool:
        """
        Returns:
            bool: the thread Zero Flag
        """
        return self.inner.c_ctx.zero_flags[self.idx]

    def cf(self) -> bool:
        """
        Returns:
            bool: the thread Carry Flag
        """
        return self.inner.c_ctx.carry_flags[self.idx]


class DpuContext():
    """
    A snapshot of a Dpu state.

    Attributes:
        threads (List[DpuThreadContext]): the thread-specific contexts
    """

    def __init__(self, c_ctx: ffi.POINTER(ffi.struct_dpu_context_t)):
        self.c_ctx = c_ctx
        self.threads = [
            DpuThreadContext(
                self, idx) for idx in range(
                c_ctx.info.nr_threads)]

    def __del__(self):
        _wrap_ffi(ffi.dpu_checkpoint_free(ffi.byref(self.c_ctx)))

    @staticmethod
    def from_buffer(buf: memoryview) -> 'DpuContext':
        """
        Deserialize a DpuContext from a byte representation.

        Args:
            buf (memoryview): the byte representation of the DpuContext

        Returns:
            DpuContext: a DpuContext object

        Raises:
            DpuError: if the deserialization fails
        """
        c_ctx = ffi.struct_dpu_context_t()
        length = len(buf)
        buf = (ffi.c_ubyte * length).from_buffer(buf)
        _wrap_ffi(
            ffi.dpu_checkpoint_deserialize(
                buf,
                length,
                ffi.byref(c_ctx)))
        return DpuContext(c_ctx)

    def serialize(self) -> bytearray:
        """
        Serialize the current DpuContext to a byte representation.

        Returns:
            bytearray: a serialized representation of the DpuContext

        Raises:
            DpuError: if the serialization fails
        """
        length = ffi.dpu_checkpoint_get_serialized_context_size(
            ffi.byref(self.c_ctx))
        buf = bytearray(length)
        c_buf = ffi.cast(
            (ffi.c_ubyte * length).from_buffer(buf),
            ffi.POINTER(
                ffi.c_ubyte))
        _wrap_ffi(
            ffi.dpu_checkpoint_serialize(
                ffi.byref(
                    self.c_ctx), ffi.byref(c_buf), ffi.byref(
                    ffi.c_uint(length))))
        return buf

    def iram(self) -> Optional[List[int]]:
        """
        Returns:
            Optional[List[int]]: a copy of the IRAM instructions if the IRAM was saved, None otherwise
        """
        return list(
            self.c_ctx.iram[:self.c_ctx.info.iram_size]) if self.c_ctx.iram else None

    def mram(self) -> Optional[List[int]]:
        """
        Returns:
            Optional[List[int]]: a copy of the MRAM bytes if the MRAM was saved, None otherwise
        """
        return list(
            self.c_ctx.mram[:self.c_ctx.info.mram_size]) if self.c_ctx.mram else None

    def wram(self) -> Optional[List[int]]:
        """
        Returns:
            Optional[List[int]]: a copy of the WRAM 32-bit words if the WRAM was saved, None otherwise
        """
        return list(
            self.c_ctx.wram[:self.c_ctx.info.wram_size]) if self.c_ctx.wram else None

    def atomic_bits(self) -> List[bool]:
        """
        Returns:
            List[bool]: a copy of the atomic memory
        """
        return list(
            self.c_ctx.atomic_register[:self.c_ctx.info.nr_atomic_bits])

    def bkp_fault(self) -> Optional[Tuple[int, int]]:
        """
        Returns:
            Optional[Tuple[int, int]]: if a breakpoint fault has been triggered,
                a tuple with the responsible thread index and the fault id,
                None otherwise
        """
        return (self.c_ctx.bkp_fault_thread_index,
                self.c_ctx.bkp_fault_id) if self.c_ctx.bkp_fault else None

    def dma_fault(self) -> Optional[int]:
        """
        Returns:
            Optional[Tuple[int, int]]: if a DMA fault has been triggered,
                the responsible thread index, None otherwise
        """
        return self.c_ctx.dma_fault_thread_index if self.c_ctx.dma_fault else None

    def mem_fault(self) -> Optional[int]:
        """
        Returns:
            Optional[Tuple[int, int]]: if a memory fault has been triggered,
                the responsible thread index, None otherwise
        """
        return self.c_ctx.mem_fault_thread_index if self.c_ctx.mem_fault else None


class DpuVariable():
    def __init__(self, contents: List[memoryview]):
        self.contents = contents

    def data(self) -> OneOrMore[memoryview]:
        return _unwrap_single_element_seq(self.contents)

    def int8(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'b')

    def uint8(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'B')

    def int16(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'h')

    def uint16(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'H')

    def int32(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'i')

    def uint32(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'I')

    def int64(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'q')

    def uint64(self) -> Union[OneOrMore[int], OneOrMore[Sequence[int]]]:
        return _cast_and_unwrap_view_list(self.contents, 'Q')

    def float(self) -> Union[OneOrMore[float], OneOrMore[Sequence[float]]]:
        return _cast_and_unwrap_view_list(self.contents, 'f')

    def double(self) -> Union[OneOrMore[float], OneOrMore[Sequence[float]]]:
        return _cast_and_unwrap_view_list(self.contents, 'd')


def _cast_and_unwrap_view_list(views, fmt):
    cast_views = [_unwrap_single_element_seq(view.cast(fmt)) for view in views]
    return _unwrap_single_element_seq(cast_views)


def _unwrap_single_element_seq(seq):
    return seq if len(seq) != 1 else seq[0]


class DpuSymbol():
    """
    Symbol in a DPU program

    Attributes:
        name (str): the name of the symbol
    """

    def __init__(self, name: str,
                 c_symbol: Union[ffi.struct_dpu_symbol_t, Tuple[int, int]]):
        self.name = name
        if isinstance(c_symbol, ffi.struct_dpu_symbol_t):
            self.c_symbol = c_symbol
        else:
            self.c_symbol = ffi.struct_dpu_symbol_t()
            self.c_symbol.address = c_symbol[0]
            self.c_symbol.size = c_symbol[1]

    def value(self) -> int:
        """
        Fetch the symbol value (ie. when done on a variable, its address)

        Returns:
            int: the symbol value
        """
        return self.c_symbol.address

    def size(self) -> int:
        """
        Fetch the symbol size

        Returns:
            int: the symbol size
        """
        return self.c_symbol.size


class DpuProgram:
    """
    Representation of DPU program
    """

    def __init__(self, c_program: ffi.POINTER(ffi.struct_dpu_program_t)):
        self.c_program = c_program
        self.symbols = {}

    def get_symbol(self, symbol: str) -> Optional[DpuSymbol]:
        """
        Fetch a symbol in the program

        Args:
            symbol (str): the name of the symbol to look for in the program

        Returns:
            Optional[DpuSymbol]: the symbol if found, None otherwise
        """
        if symbol not in self.symbols:
            c_symbol = ffi.struct_dpu_symbol_t()
            err = ffi.dpu_get_symbol(
                self.c_program, symbol, ffi.byref(c_symbol))
            if err != ffi.DPU_OK:
                return None
            self.symbols[symbol] = DpuSymbol(symbol, c_symbol)
        return self.symbols[symbol]

    def fetch_all_symbols(self) -> Dict[str, DpuSymbol]:
        symbol_names = ffi.Py_dpu_get_symbol_names(self.c_program)
        for name in symbol_names:
            self.get_symbol(name)
        return self.symbols


class DpuSet:
    """
    A set of DPUs

    Args:
        nr_dpus (Optional[int]): the number of DPUs to allocate
            ALLOCATE_ALL will allocate all available DPUs
        nr_ranks (Optional[int]): the number of DPU ranks to allocate
            ALLOCATE_ALL will allocate all available DPU ranks
            Cannot be set at the same time as nr_dpus. Setting none of them will behave as ALLOCATE_ALL.
        profile (str): the profile of the DPUs
        binary (Optional[DpuBinaryProgram]): a DPU binary program to load after the allocation
        c_source (Optional[DpuSourceProgram]): a DPU C source code to compile and load after the allocation
        asm_source (Optional[DpuSourceProgram]): a DPU assembly source code to compile and load after the allocation
        log (Optional[DpuConstructorLogObject]): a object where the DPU log will be written.
            The object can either be a file object or a list of file object
            with one object per dpu
        async_mode (Optional[bool]): the default behavior for copy and exec methods

    Raises:
        DpuError: if the allocation fails
    """

    def __init__(
            self,
            nr_dpus: Optional[int] = None,
            nr_ranks: Optional[int] = None,
            profile: str = '',
            binary: Optional[DpuBinaryProgram] = None,
            c_source: Optional[DpuSourceProgram] = None,
            asm_source: Optional[DpuSourceProgram] = None,
            log: Optional[DpuConstructorLogObject] = None,
            async_mode: Optional[bool] = False,
            _do_internals=True):
        self._symbols = {}
        self._dpus = []
        self._ranks = []
        self._top_set = None
        self._rank_set = None
        self.c_set = None
        self._async_mode = async_mode
        self._callback_ctxts = {}
        self._callback_ctxts_cnt = {}
        self._callback_id_lock = threading.Lock()
        self._callback_id = 0

        if not isinstance(log, list):
            self._log = log

        if not _do_internals:
            return

        if nr_dpus is not None and nr_ranks is not None:
            raise DpuError('cannot specify nr_dpus and nr_ranks')

        if nr_dpus is None and nr_ranks is None:
            nr_dpus = ALLOCATE_ALL

        self.c_set = ffi.struct_dpu_set_t()

        if nr_dpus is not None:
            _wrap_ffi(ffi.dpu_alloc(nr_dpus, profile, ffi.byref(self.c_set)))
        else:
            _wrap_ffi(
                ffi.dpu_alloc_ranks(
                    nr_ranks,
                    profile,
                    ffi.byref(
                        self.c_set)))

        rank_iterator = _dpu_set_iter(
            ffi.dpu_set_rank_iterator_from(self.c_set),
            ffi.dpu_set_rank_iterator_next)

        for c_rank in rank_iterator:
            rank_set = DpuSet(
                log=log,
                _do_internals=False,
                async_mode=async_mode)
            rank_set.c_set = c_rank
            self._ranks.append(rank_set)
            rank_set._ranks.append(rank_set)
            rank_set._top_set = self

            dpu_iterator = _dpu_set_iter(
                ffi.dpu_set_dpu_iterator_from(c_rank),
                ffi.dpu_set_dpu_iterator_next)

            for c_dpu in dpu_iterator:
                dpu = DpuSet(
                    log=log,
                    _do_internals=False,
                    async_mode=async_mode)
                dpu.c_set = c_dpu
                self._dpus.append(dpu)
                rank_set._dpus.append(dpu)
                dpu._dpus.append(dpu)
                dpu._ranks.append(rank_set)
                rank_set._top_set = self
                dpu._top_set = self
                dpu._rank_set = rank_set

        try:
            if isinstance(log, list):
                if len(log) != len(self._dpus):
                    raise DpuError(
                        'log list length does not match number of DPUs in DpuSet')
                for dpu, dpu_log in zip(self._dpus, log):
                    dpu._log = dpu_log

            if c_source is not None or asm_source is not None or binary is not None:
                self.load(binary, c_source, asm_source)
        except BaseException:
            ffi.dpu_free(self.c_set)
            raise

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.free()

    def __iter__(self) -> Iterator['DpuSet']:
        return iter(self.dpus())

    def __len__(self) -> int:
        return len(self._dpus)

    def __getattribute__(self, name):
        if name == '_symbols':
            return super(DpuSet, self).__getattribute__(name)

        symbol = self._symbols.get(name)

        if symbol is None:
            return super(DpuSet, self).__getattribute__(name)

        return self.__get_symbol(symbol)

    def __setattr__(self, name, value):
        if name == '_symbols':
            super(DpuSet, self).__setattr__(name, value)
            return

        symbol = self._symbols.get(name)

        if symbol is None:
            super(DpuSet, self).__setattr__(name, value)
        else:
            self.copy(symbol, value)

    def dpus(self) -> List['DpuSet']:
        """Provides a List on the DPUs of the DPU set"""
        return self._dpus

    def ranks(self) -> List['DpuSet']:
        """Provides a List on the ranks of the DPU set"""
        return self._ranks

    def free(self):
        """Release the DPUs of the set"""
        _wrap_ffi(ffi.dpu_free(self.c_set))

    def load(
            self,
            binary: Optional[DpuBinaryProgram] = None,
            c_source: Optional[DpuSourceProgram] = None,
            asm_source: Optional[DpuSourceProgram] = None) -> DpuProgram:
        """
        Load a program in the DPUs

        Args:
            binary (Optional[DpuBinaryProgram]): the DPU binary program to load
            c_source (Optional[DpuSourceProgram]): the DPU C source code to compile and load
            asm_source (Optional[DpuSourceProgram]): the DPU assembly source code to compile and load

        Returns:
            DpuProgram: a representation of the loaded program

        Raises:
            DpuError: if the program cannot be loaded
        """
        if (binary is None) == (c_source is None and asm_source is None):
            raise DpuError(
                'one and only one source or binary program must be defined')

        with tempfile.TemporaryDirectory() as tmpdir:
            sources = []
            if c_source is not None:
                if not isinstance(c_source, list):
                    source = [c_source]

                for idx, src in enumerate(source):
                    if not isinstance(src, str):
                        source_name = os.path.join(
                            tmpdir, 'src' + str(idx) + '.c')
                        with open(source_name, 'w') as source_file:
                            source_file.write(src.read())
                        src = source_name

                    sources.append(src)
            if asm_source is not None:
                if not isinstance(asm_source, list):
                    source = [asm_source]

                for idx, src in enumerate(source):
                    if not isinstance(src, str):
                        source_name = os.path.join(
                            tmpdir, 'src' + str(idx) + '.S')
                        with open(source_name, 'w') as source_file:
                            source_file.write(src.read())
                        src = source_name

                    sources.append(src)

            if len(sources) != 0:
                options = []
                if c_source is None:
                    options.append("-nostartfiles")
                binary = os.path.join(tmpdir, 'main.dpu')
                compiler.DEFAULT_DPU_COMPILER.compile(
                    sources,
                    output=binary,
                    options=options,
                    opt_lvl=compiler.O2)

            c_program = ffi.POINTER(ffi.struct_dpu_program_t)()
            c_program_ref = ffi.byref(c_program)
            if isinstance(binary, str):
                ffi_result = ffi.dpu_load(self.c_set, binary, c_program_ref)
            else:
                program_len = len(binary)
                program_buf = (ffi.c_byte * program_len).from_buffer(binary)
                ffi_result = ffi.dpu_load_from_memory(
                    self.c_set, program_buf, program_len, c_program_ref)
        _wrap_ffi(ffi_result)
        program = DpuProgram(c_program)
        self.__update_symbol_attributes(program)
        return program

    def __update_symbol_attributes(self, program: DpuProgram):
        symbols = program.fetch_all_symbols()

        if self._top_set is None:
            for rank in self.ranks():
                rank.__del_symbol_attributes()
                rank.__add_symbol_attributes(symbols)
        else:
            self._top_set.__del_symbol_attributes()

        if self._rank_set is None:
            for dpu in self.dpus():
                dpu.__del_symbol_attributes()
                dpu.__add_symbol_attributes(symbols)
        else:
            self._rank_set.__del_symbol_attributes()

        self.__del_symbol_attributes()
        self.__add_symbol_attributes(symbols)

    def __add_symbol_attributes(self, symbols: Dict[str, DpuSymbol]):
        for symbol in symbols:
            setattr(self, symbol, None)
        self._symbols = symbols

    def __del_symbol_attributes(self):
        for symbol in self._symbols:
            delattr(self, symbol)
        self._symbols = {}

    def __get_symbol(self, symbol: DpuSymbol) -> DpuVariable:
        nr_dpus = len(self)
        size = symbol.size()
        view = memoryview(bytearray(nr_dpus * size))
        buf = [view[i * size:(i + 1) * size] for i in range(nr_dpus)]
        self.copy(buf, symbol)
        return DpuVariable(buf)

    def __get_xfer_mode(self, async_mode: bool):
        if async_mode is not None:
            if async_mode:
                return ffi.DPU_XFER_ASYNC
            else:
                return ffi.DPU_XFER_DEFAULT
        elif self._async_mode:
            return ffi.DPU_XFER_ASYNC
        else:
            return ffi.DPU_XFER_DEFAULT

    def copy(
            self,
            dst: DpuCopyBuffer,
            src: DpuCopyBuffer,
            size: Optional[int] = None,
            offset: int = 0,
            async_mode: Optional[bool] = None):
        """
        Copy data between the Host and the DPUs

        Args:
            dst (DpuCopyBuffer): the destination buffer for the copy.
                If the buffer is on the Host, this must be a bytearray, or a list of optional bytearrays.
                If the buffer is on the DPUs, this must be a str, representing a DPU symbol, or directly a DpuSymbol.
            src (DpuCopyBuffer): the source buffer for the copy.
                If the buffer is on the Host, this must be a bytearray, or a list of optional bytearrays.
                If the buffer is on the DPUs, this must be a str, representing a DPU symbol, or directly a DpuSymbol.
            size (Optional[int]): number of bytes to transfer.
                By default set to None, which will use the length of the Host buffer. If multiple Host buffers are
                provided, all buffers must have the same size.
            offset (int): offset in bytes for the DPU symbol.
                By default set to 0.
            async_mode (Optional[bool]): override the default behavior set in the constructor.

        Raises:
            DpuError: if the copy fails, because of invalid arguments, unknown symbols or out-of-bound accesses
        """
        dpu_symbol_is_str = isinstance(dst, str) or isinstance(src, str)
        multiple_buffers = isinstance(dst, list) or isinstance(src, list)
        to_dpu = isinstance(dst, DpuSymbol) or isinstance(dst, str)
        from_dpu = isinstance(src, DpuSymbol) or isinstance(src, str)

        if to_dpu == from_dpu:
            raise DpuError('can only execute Host <-> DPU transfers')

        if to_dpu:
            host = src
            device = dst
            xfer_dir = ffi.DPU_XFER_TO_DPU
        else:
            host = dst
            device = src
            xfer_dir = ffi.DPU_XFER_FROM_DPU

        if multiple_buffers and (len(host) == 1) and (len(self._dpus) == 1):
            multiple_buffers = False
            host = host[0]

        if to_dpu and not multiple_buffers:
            ffi_fn_str = ffi.dpu_broadcast_to
            ffi_fn_sym = ffi.dpu_broadcast_to_symbol
        else:
            ffi_fn_str = ffi.dpu_push_xfer
            ffi_fn_sym = ffi.dpu_push_xfer_symbol

        if dpu_symbol_is_str:
            ffi_fn = ffi_fn_str
        else:
            ffi_fn = ffi_fn_sym
            device = device.c_symbol

        buffer_size = None
        if multiple_buffers:
            dpu_list = self._dpus

            if len(host) != len(dpu_list):
                raise DpuError(
                    'invalid number of host buffers for this DPU set')

            buffer_size = ffi.c_size_t()
            _wrap_ffi(
                ffi.Py_dpu_prepare_xfers(
                    self.c_set,
                    host,
                    ffi.byref(buffer_size)))
            buffer_size = buffer_size.value
        else:
            buffer_size = len(host)
            host = (ffi.c_byte * buffer_size).from_buffer(host)

        if size is None:
            size = buffer_size
        elif buffer_size < size:
            raise DpuError('host buffer is too small')

        xfer_mode = self.__get_xfer_mode(async_mode)
        if multiple_buffers:
            _wrap_ffi(
                ffi_fn(
                    self.c_set,
                    xfer_dir,
                    device,
                    offset,
                    size,
                    xfer_mode))
        elif ffi_fn_str == ffi.dpu_broadcast_to:
            _wrap_ffi(
                ffi_fn(
                    self.c_set,
                    device,
                    offset,
                    host,
                    size,
                    xfer_mode))
        else:
            _wrap_ffi(ffi.dpu_prepare_xfer(self.c_set, host))
            _wrap_ffi(
                ffi_fn(
                    self.c_set,
                    xfer_dir,
                    device,
                    offset,
                    size,
                    xfer_mode))

    def exec(
            self,
            log: Optional[DpuExecLogObject] = None,
            async_mode: Optional[bool] = None):
        """
        Execute a program on the DPUs

        Args:
            log (Optional[DpuExecLogObject]): a object where the DPU log will be written.
                The object can either be a file object or a list of file object
                with one object per dpu or a dictionary of file object
            async_mode (Optional[bool]): override the default behavior set in the constructor.
                When in asynchronous mode, the log argument is ignored.

        Raises:
            DpuError: if the program cannot be loaded, or the execution triggers a fault
        """
        if async_mode is not None and async_mode:
            _wrap_ffi(ffi.dpu_launch(self.c_set, ffi.DPU_ASYNCHRONOUS))
            return
        elif self._async_mode:
            _wrap_ffi(ffi.dpu_launch(self.c_set, ffi.DPU_ASYNCHRONOUS))
            return

        _wrap_ffi(ffi.dpu_launch(self.c_set, ffi.DPU_SYNCHRONOUS))
        self._logfn(log, None)

    def log(self, stream: Optional[DpuExecLogObject] = None):
        """
        Print the DPU log on the Host standard output

        Args:
            stream: a writable object. If not specify, uses either the one
                specified in DpuSet's constructor or sys.stdout otherwise

        Raises:
            DpuError: if the DPU log buffer cannot be fetched, or the DPU set is invalid for this operation
        """
        self._logfn(stream, sys.stdout)

    def _logfn(self,
               log: Optional[DpuExecLogObject],
               default: Optional[io.IOBase]):
        log_streams = {
            dpu: dpu._log if dpu._log is not None else default for dpu in self._dpus}

        if log is not None:
            if isinstance(log, list):
                if len(log) != len(self._dpus):
                    raise DpuError(
                        'cannot log, log list length does not match the number of DPUs in DpuSet')
                for dpu, dpu_log in zip(self._dpus, log):
                    if dpu_log is not None:
                        log_streams[dpu] = dpu_log
            elif isinstance(log, dict):
                for dpu, dpu_log in log.items():
                    log_streams[dpu] = dpu_log
            else:
                for dpu in self._dpus:
                    log_streams[dpu] = log

        for dpu, log_stream in log_streams.items():
            if log_stream is not None:
                if not callable(getattr(log_stream, "write", None)):
                    raise DpuError(
                        "stream argument is not an object with a 'write' method")
                _wrap_ffi(ffi.Py_dpu_log_read(dpu.c_set, log_stream))

    def checkpoint(self, iram: bool = True, mram: bool = True,
                   wram: bool = True) -> DpuContext:
        """
        Extract the complete context of the DPU

        Args:
            iram (bool): also save the IRAM (default is True)
            mram (bool): also save the MRAM (default is True)
            wram (bool): also save the WRAM (default is True)

        Raises:
            DpuError: if the DPU context cannot be fetched, or the DPU set is invalid for this operation
        """
        flags = _build_checkpoint_flags(iram, mram, wram)
        c_ctx = ffi.struct_dpu_context_t()
        _wrap_ffi(ffi.dpu_checkpoint_save(self.c_set, flags, ffi.byref(c_ctx)))
        return DpuContext(c_ctx)

    def restore(
            self,
            context: DpuContext,
            iram: bool = True,
            mram: bool = True,
            wram: bool = True):
        """
        Apply the context to the DPU

        Args:
            context: the context to restore
            iram (bool): also restore the IRAM (default is True)
            mram (bool): also restore the MRAM (default is True)
            wram (bool): also restore the WRAM (default is True)

        Raises:
            DpuError: if the DPU context cannot be restored, or the DPU set is invalid for this operation
        """
        flags = _build_checkpoint_flags(iram, mram, wram)
        _wrap_ffi(
            ffi.dpu_checkpoint_restore(
                self.c_set, flags, ffi.byref(
                    context.c_ctx)))

    def sync(self):
        """
        Wait for all asynchronous operations to be complete

        Raises:
           DpuError: if the DPU set is invalid for this operation
        """
        _wrap_ffi(ffi.dpu_sync(self.c_set))

    def call(
            self,
            fn: DpuCallbackFunction,
            arg: Any,
            async_mode: Optional[bool] = None,
            is_blocking: bool = True,
            single_call: bool = False):
        """
        Call the user-provided function on the DPU set.

        Args:
            fn (DpuCallbackFunction): function called on the DPU set
            arg (Any): argument passed to the function
            async_mode (Optional[bool]): override the default behavior set in the constructor
            is_blocking (bool): whether the DPU set must wait the end of the function before accessing the DPU set again (default is True)
            single_call (bool): whether the function is called once for the whole DPU set, instead of once per DPU rank (default is False)

        Raises:
            DpuError: if the operation fails
        """
        if async_mode is None:
            async_mode = self._async_mode

        with self._callback_id_lock:
            ctx_id = self._callback_id
            self._callback_id = self._callback_id + 1
            ctx = _CallbackContext(fn, arg, single_call, ctx_id, self)
            self._callback_ctxts[ctx_id] = ctx
            self._callback_ctxts_cnt[ctx_id] = 1 if single_call else len(
                self._ranks)
        flags = _build_callback_flags(async_mode, is_blocking, single_call)
        _wrap_ffi(
            ffi.dpu_callback(
                self.c_set,
                _callback,
                ffi.byref(ctx),
                flags))


@ffi.CFUNCTYPE(
    ffi.UNCHECKED(
        ffi.dpu_error_t),
    ffi.struct_dpu_set_t,
    ffi.c_uint32,
    ffi.POINTER(None))
def _callback(c_set: ffi.struct_dpu_set_t, idx: ffi.c_uint32,
              args: ffi.POINTER(None)) -> ffi.UNCHECKED(ffi.dpu_error_t):
    args = ffi.cast(args, ffi.POINTER(_CallbackContext)).contents
    dpu_set = None
    fn = args.fn
    arg = args.arg
    ctx_id = args.ctx_id
    single_call = args.single_call
    calling_set = args.dpu_set

    if single_call or (c_set.kind == ffi.DPU_SET_DPU):
        dpu_set = calling_set
    else:
        c_rank = c_set.unnamed_1.list.ranks[0]
        c_rank_addr = ffi.addressof(c_rank)
        for rank in calling_set._ranks:
            other_c_rank = rank.c_set.unnamed_1.list.ranks[0]
            other_c_rank_addr = ffi.addressof(other_c_rank)
            if c_rank_addr == other_c_rank_addr:
                dpu_set = rank
                break

    fn(dpu_set, idx, arg)

    with calling_set._callback_id_lock:
        calling_set._callback_ctxts_cnt[ctx_id] = calling_set._callback_ctxts_cnt[ctx_id]
        if calling_set._callback_ctxts_cnt[ctx_id] == 0:
            del calling_set._callback_ctxts[ctx_id]
            del calling_set._callback_ctxts_cnt[ctx_id]

    return ffi.DPU_OK


def _build_callback_flags(
        is_async: bool,
        is_blocking: bool,
        is_single_call: bool) -> int:
    flags = ffi.DPU_CALLBACK_DEFAULT

    if is_async:
        flags |= ffi.DPU_CALLBACK_ASYNC

    if not is_blocking:
        flags |= ffi.DPU_CALLBACK_NONBLOCKING

    if is_single_call:
        flags |= ffi.DPU_CALLBACK_SINGLE_CALL

    return flags


def _build_checkpoint_flags(iram: bool, mram: bool, wram: bool) -> int:
    flags = ffi.DPU_CHECKPOINT_ALL

    if not iram:
        flags &= ~ffi.DPU_CHECKPOINT_IRAM

    if not mram:
        flags &= ~ffi.DPU_CHECKPOINT_MRAM

    if not wram:
        flags &= ~ffi.DPU_CHECKPOINT_WRAM

    return flags


def _wrap_ffi(err: ffi.dpu_error_t):
    if err != ffi.DPU_OK:
        raise DpuError(ffi.dpu_error_to_string(err))


def _dpu_set_iter(c_iterator, next_fn) -> Iterator[ffi.struct_dpu_set_t]:
    while c_iterator.has_next:
        c_set = type(c_iterator.next)()
        ffi.pointer(c_set)[0] = c_iterator.next
        next_fn(ffi.byref(c_iterator))
        yield c_set
