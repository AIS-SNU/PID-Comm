DPU_DIR := GNN_kernel_1
DPU_DIR2 := GNN_kernel_2
DPU_DIR3 := data_relocate_comm
HOST_DIR := host
BUILDDIR ?= bin
TYPE ?= INT32
NR_TASKLETS ?= 16


define conf_filename
	${BUILDDIR}/.NR_TASKLETS_$(1)_TYPE_$(2).conf
endef
CONF := $(call conf_filename,${NR_TASKLETS},${TYPE})

HOST_TARGET := ${BUILDDIR}/host
DPU_TARGET := ${BUILDDIR}/GNN_kernel_1
DPU_TARGET2 := ${BUILDDIR}/GNN_kernel_2
DPU_TARGET3 := ${BUILDDIR}/data_relocate_comm

COMMON_INCLUDES := support
HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/*.c)
DPU_SOURCES2 := $(wildcard ${DPU_DIR2}/*.c)
DPU_SOURCES3 := $(wildcard ${DPU_DIR3}/*.c)

.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})


COMMON_FLAGS := -g -I${COMMON_INCLUDES}
HOST_FLAGS := ${COMMON_FLAGS} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu` -fopenmp -lm -D${TYPE} -DNR_TASKLETS=${NR_TASKLETS}
DPU_FLAGS := ${COMMON_FLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -D${TYPE} 

all: ${HOST_TARGET} ${DPU_TARGET} ${DPU_TARGET2} ${DPU_TARGET3}

${CONF}:
	$(RM) $(call conf_filename,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES} ${CONF}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES}

${DPU_TARGET2}: ${DPU_SOURCES2} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES2}

${DPU_TARGET3}: ${DPU_SOURCES3} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES3}

clean:
	$(RM) $(BUILDDIR)/host
	$(RM) $(BUILDDIR)/GNN_kernel_1
	$(RM) $(BUILDDIR)/GNN_kernel_2
	$(RM) $(BUILDDIR)/data_relocate_comm


test: all
	./${HOST_TARGET}