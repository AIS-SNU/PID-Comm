/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

package com.upmem.dpu;

/**
 * A symbol in a DPU program.
 *
 * <p>Used in the copy methods of {@link DpuSet} and {@link Dpu}.</p>
 *
 * @see DpuSet
 * @see Dpu
 */
public final class DpuSymbol {
    private final int address;
    private final int size;
    private final String name;

    /**
     * Create a DPU symbol.
     *
     * @param address The memory address of the symbol.
     * @param size The memory size of the symbol.
     * @param name The name of the symbol.
     */
    public DpuSymbol(int address, int size, String name) {
        this.address = address;
        this.size = size;
        this.name = name;
    }

    /**
     * Create an anonymous DPU symbol.
     *
     * @param address The memory address of the symbol.
     * @param size The memory size of the symbol.
     */
    public DpuSymbol(int address, int size) {
        this(address, size, null);
    }
}
