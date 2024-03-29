DPU_DIR1 := dpu1
DPU_DIR2 := dpu2
DPU_DIR3 := dpu3
DPU_DIR4 := dpu4
DPU_DIR5 := dpu5
DPU_DIR6 := dpu6
DPU_DIR7 := dpu7
DPU_DIR8 := dpu8

HOST_DIR := host
BUILDDIR ?= bin
TYPE ?= INT32
NR_TASKLETS ?= 16
NR_DPUS ?= 1024

define conf_filename
	${BUILDDIR}/.NR_DPUS_$(1)_NR_TASKLETS_$(2)_TYPE_$(3).conf
endef
CONF := $(call conf_filename,${NR_DPUS},${NR_TASKLETS},${TYPE})

HOST_TARGET := ${BUILDDIR}/host_rs
DPU_TARGET1 := ${BUILDDIR}/dpu_relocate_inplace
DPU_TARGET2 := ${BUILDDIR}/alltoall_x_2
DPU_TARGET3 := ${BUILDDIR}/dpu_ar_2
DPU_TARGET4 := ${BUILDDIR}/dpu_ar_2_y
DPU_TARGET5 := ${BUILDDIR}/alltoall_22
DPU_TARGET6 := ${BUILDDIR}/rs_24
DPU_TARGET7 := ${BUILDDIR}/rs_22
DPU_TARGET8 := ${BUILDDIR}/ar_24


COMMON_INCLUDES := support
HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES1 := $(wildcard ${DPU_DIR1}/*.c)
DPU_SOURCES2 := $(wildcard ${DPU_DIR2}/*.c)
DPU_SOURCES3 := $(wildcard ${DPU_DIR3}/*.c)
DPU_SOURCES4 := $(wildcard ${DPU_DIR4}/*.c)
DPU_SOURCES5 := $(wildcard ${DPU_DIR5}/*.c)
DPU_SOURCES6 := $(wildcard ${DPU_DIR6}/*.c)
DPU_SOURCES7 := $(wildcard ${DPU_DIR7}/*.c)
DPU_SOURCES8 := $(wildcard ${DPU_DIR8}/*.c)


.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})

#COMMON_FLAGS := -Wall -Wextra -g -I${COMMON_INCLUDES}
COMMON_FLAGS := -g -I${COMMON_INCLUDES} 
HOST_FLAGS := ${COMMON_FLAGS} -Wall -Wextra `dpu-pkg-config --cflags --libs dpu` -fopenmp -D${TYPE} -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS}
DPU_FLAGS := ${COMMON_FLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -D${TYPE} 

all: ${HOST_TARGET} ${DPU_TARGET1} ${DPU_TARGET2} ${DPU_TARGET3} ${DPU_TARGET4} ${DPU_TARGET5} ${DPU_TARGET6} ${DPU_TARGET7} ${DPU_TARGET8}
${CONF}:
	$(RM) $(call conf_filename,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES} ${CONF}
	gcc --std=c99 -o $@ ${HOST_SOURCES} ${HOST_FLAGS} #-L./lib

${DPU_TARGET1}: ${DPU_SOURCES1} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES1} #-L./lib

${DPU_TARGET2}: ${DPU_SOURCES2} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES2} #-L./lib

${DPU_TARGET3}: ${DPU_SOURCES3} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES3} #-L./lib

${DPU_TARGET4}: ${DPU_SOURCES4} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES4} #-L./lib

${DPU_TARGET5}: ${DPU_SOURCES5} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES5} #-L./lib

${DPU_TARGET6}: ${DPU_SOURCES6} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES6} #-L./lib

${DPU_TARGET7}: ${DPU_SOURCES7} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES7} #-L./lib

${DPU_TARGET8}: ${DPU_SOURCES8} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES8} #-L./lib

clean:
	$(RM) -r $(BUILDDIR)

test: all
	./${HOST_TARGET}