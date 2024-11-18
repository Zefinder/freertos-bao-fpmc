# Add MEASURE_RESPONSE_TIME
CPPFLAGS+=-DMEASURE_RESPONSE_TIME
CPPFLAGS+=-DDEFAULT_IPI_HANDLERS
CPPFLAGS+=-DMEMORY_REQUEST_WAIT
spec_c_srcs:= main.c
spec_c_srcs+= mpeg2.c
spec_c_srcs+= countnegative.c
spec_c_srcs+= bubblesort.c