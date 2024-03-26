/* Copyright 2020 UPMEM. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

package com.upmem.dpu;

import java.io.PrintStream;

/**
 * Asynchronous version of the methods provided by a DPU set.
 *
 * @see DpuSet
 */
public final class DpuSetAsync {
    private final NativeDpuSet set;
    private final PrintStream logStream;

    DpuSetAsync(NativeDpuSet set, PrintStream logStream) {
        this.set = set;
        this.logStream = logStream;
    }

    public void copy(String dstSymbol, byte[] srcBuffer) throws DpuException {
        this.set.broadcast(dstSymbol, srcBuffer, true);
    }

    public void copy(DpuSymbol dstSymbol, byte[] srcBuffer) throws DpuException {
        this.set.broadcast(dstSymbol, srcBuffer, true);
    }

    public void copy(String dstSymbol, byte[][] srcBuffers) throws DpuException {
        this.set.copy(dstSymbol, srcBuffers, true, true);
    }

    public void copy(DpuSymbol dstSymbol, byte[][] srcBuffers) throws DpuException {
        this.set.copy(dstSymbol, srcBuffers, true, true);
    }

    public void copy(byte[][] dstBuffers, String srcSymbol) throws DpuException {
        this.set.copy(srcSymbol, dstBuffers, false, true);
    }

    public void copy(byte[][] dstBuffers, DpuSymbol srcSymbol) throws DpuException {
        this.set.copy(srcSymbol, dstBuffers, false, true);
    }

    public void exec() throws DpuException {
        this.exec(this.logStream);
    }

    public void exec(PrintStream logStream) throws DpuException {
        this.set.launch(true);

        if (logStream != null) {
            this.call((s, i) -> s.log(logStream));
        }
    }

    /**
     * Asynchronously call the provided callback on the DPU set.
     *
     * @param callback The function to be called on the DPU set.
     * @param blocking Whether other asynchronous operations must wait the end of the callback
     *                 before starting.
     * @param singleCall Whether the callback is done once for the whole DPU set or once per DPU rank.
     * @exception DpuException When the callback cannot be registered.
     * @see DpuCallback
     */
    public void call(DpuCallback callback, boolean blocking, boolean singleCall) throws DpuException {
        this.set.call(callback, blocking, singleCall);
    }

    /**
     * Asynchronously call the provided callback on the DPU set.
     *
     * <p>The callback is called once per DPU rank and blocks the other asynchronous operations.</p>
     *
     * @param callback The function to be called on each DpuRank of the DPU set.
     * @exception DpuException When the callback cannot be registered.
     * @see DpuCallback
     */
    public void call(DpuCallback callback) throws DpuException {
        this.call(callback, true, false);
    }

    /**
     * Wait for the end of all enqueued asynchronous operations.
     *
     * @exception DpuException When any of the asynchronous methods throws an Exception.
     * @see DpuException
     */
    public void sync() throws DpuException {
        this.set.sync();
    }
}
