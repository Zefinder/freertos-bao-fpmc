# Add MEASURE_RESPONSE_TIME
CPPFLAGS+=-DMEASURE_RESPONSE_TIME
CPPFLAGS+=-DDEFAULT_IPI_HANDLERS
CPPFLAGS+=-DMEMORY_REQUEST_WAIT

ifeq ($(LEGACY),y)
spec_c_srcs:= main_legacy.c
else
spec_c_srcs:= main_prem.c
endif
spec_c_srcs+= mpeg2.c
spec_c_srcs+= countnegative.c
spec_c_srcs+= bubblesort.c