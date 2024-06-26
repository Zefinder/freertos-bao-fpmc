# Prepare enviroment for baremetal-runtime
NAME:=freertos
ROOT_DIR:=$(realpath .)
BUILD_DIR:=$(ROOT_DIR)/build/$(PLATFORM)

# Setup baremetal-runtime build
bmrt_dir:=$(ROOT_DIR)/src/baremetal-runtime
include $(bmrt_dir)/setup.mk

# From here we know that platform is defined
CPPFLAGS+=-DPLATFORM=$(PLATFORM)

# Main freertos application sources
app_src_dir:=$(ROOT_DIR)/src
include $(app_src_dir)/sources.mk
C_SRC+=$(addprefix $(app_src_dir)/, $(src_c_srcs))
ASM_SRC+=$(addprefix $(app_src_dir)/, $(src_s_srcs))
INC_DIRS+=$(app_src_dir)/inc

# Specific freertos application sources
ifeq ($(SELECTED_MAIN),)
	$(error Application not selected, you must choose an application)
endif
spec_src_dir:=$(app_src_dir)/$(SELECTED_MAIN)
include $(spec_src_dir)/sources.mk
C_SRC+=$(addprefix $(spec_src_dir)/, $(spec_c_srcs))
ASM_SRC+=$(addprefix $(spec_src_dir)/, $(spec_s_srcs))

# Freertos kernel sources
freertos_dir:=$(ROOT_DIR)/src/freertos
SRC_DIRS+=$(freertos_dir) $(freertos_memmng_dir)
INC_DIRS+=$(freertos_dir)/include
C_SRC+=$(wildcard $(freertos_dir)/*.c)
freertos_memmng_dir:=$(freertos_dir)/portable/MemMang
C_SRC+=$(freertos_memmng_dir)/heap_4.c

# Freertos port arch-specific
arch_dir=$(ROOT_DIR)/src/arch/$(ARCH)
SRC_DIRS+=$(arch_dir)
INC_DIRS+=$(arch_dir)/inc
-include $(arch_dir)/arch.mk
-include $(arch_dir)/sources.mk
C_SRC+=$(addprefix $(arch_dir)/, $(arch_c_srcs))
ASM_SRC+=$(addprefix $(arch_dir)/, $(arch_s_srcs))

# Include the final baremetal-runtime build
# Add default IPI handlers for PREM define
ifeq ($(DEFAULT_IPI),y)
CPPFLAGS+=-DDEFAULT_IPI_HANDLERS
endif

# Enable PREM memory request waiting time
ifeq ($(MEMORY_REQUEST_WAIT),y)
CPPFLAGS+=-DMEMORY_REQUEST_WAIT
endif
include $(bmrt_dir)/build.mk
