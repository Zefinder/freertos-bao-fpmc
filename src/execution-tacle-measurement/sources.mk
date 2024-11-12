# Add MEASURE_RESPONSE_TIME
CPPFLAGS+=-DMEASURE_RESPONSE_TIME
spec_c_srcs:= main.c
spec_c_srcs+= mpeg2.c
spec_c_srcs+= countnegative.c
spec_c_srcs+= binarysearch.c