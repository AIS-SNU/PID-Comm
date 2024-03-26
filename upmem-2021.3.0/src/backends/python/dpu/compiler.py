# Copyright 2020 UPMEM. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides standard C compilation features."""

import os
import subprocess
import sys

"""Optimization level corresponding to '-O0'"""
O0 = '-O0'
"""Optimization level corresponding to '-O1'"""
O1 = '-O1'
"""Optimization level corresponding to '-O2'"""
O2 = '-O2'
"""Optimization level corresponding to '-O3'"""
O3 = '-O3'
"""Optimization level corresponding to '-Os'"""
Os = '-Os'
"""Optimization level corresponding to '-Oz'"""
Oz = '-Oz'


class Compiler:
    """
    A representation of a compiler with some parameters

    Attributes:
        path (str): Path to the compiler executable
        target (str, optional): if specified, the target to use instead of the default one
        verbose (bool, optional): if set to True, outputs some details on the executed command and its outputs
    """

    def __init__(self, path, target=None, verbose=False):
        """
        Args:
            path (str): Path to the compiler executable
            target (str, optional): if specified, the target to use instead of the default one
            verbose (bool, optional): if set to True, outputs some details on the executed command and its outputs
        """
        self.path = path
        self.target = target
        self.verbose = verbose

    def compile(
            self,
            sources,
            output=None,
            includes=[],
            defines={},
            options=[],
            target=None,
            opt_lvl=None,
            debug=True,
            verbose=False):
        """Compiles some sources files

        Args:
            sources (str or list of str): The source files
            output (str, optional): The output file path (default is None)
            includes ((list of str), optional): The include directories (default is [])
            defines ((dict of str: str, optional), optional): The defines to add during preprocessing (default is {})
            options ((list of str), optional): Additional options for the compiler execution (default is [])
            target (str, optional): The target for the compilation (default is None)
            opt_lvl (str, optional): The optimization level to use (default is None)
            debug (bool, optional): Whether the debug information should be added to the output (default is True)
            verbose (bool, optional): Whether compilation information concerning the executed command and its outputs should be printed (default is False)

        Returns:
            CompletedProcess: The status of the finish compiler executable execution

        Raises:
            CalledProcessError: If the compiler executable returns a non-zero exit code
        """
        args = [self.path]

        if target is not None:
            args.append('--target={}'.format(target))
        elif self.target is not None:
            args.append('--target={}'.format(self.target))

        if opt_lvl is not None:
            args.append(opt_lvl)

        if not debug:
            args.append('-g0')

        for name, value in defines.items():
            args.append('-D')
            if value is None:
                args.append(name)
            else:
                args.append('{}={}'.format(name, value))

        for include in includes:
            args.append('-I')
            args.append(include)

        if isinstance(sources, str):
            sources = [sources]
        for src in sources:
            args.append(src)

        for opt in options:
            args.append(opt)

        if output is not None:
            args.append('-o')
            args.append(output)

        if verbose or self.verbose:
            print(' '.join(args))
            stdout = sys.stdout
            stderr = sys.stderr
        else:
            stdout = subprocess.PIPE
            stderr = subprocess.PIPE

        return subprocess.run(
            args,
            check=True,
            stdout=stdout,
            stderr=stderr,
            universal_newlines=True)


def _find_default_compiler():
    executable = os.path.join(
        os.path.dirname(
            os.path.dirname(
                os.path.dirname(
                    os.path.dirname(
                        os.path.dirname(
                            os.path.abspath(__file__)))))),
        'bin', 'clang')
    if not os.path.exists(executable):
        executable = 'clang'

    return Compiler(executable, 'dpu-upmem-dpurte')


"""Default clang compiler found in the same package as this module, targetting the DPU"""
DEFAULT_DPU_COMPILER = _find_default_compiler()
