# Disable built-in rules.
.SUFFIXES: 

CFLAGS	    	= -std=c99   -Wall -I . -O3 -g # -DNDEBUG
CXXFLAGS    	= -std=c++14 -Wall -I . -I vendor/fast_double_parser/include -O3 -g -fpermissive -fPIC # -DNDEBUG
LDLIBS          = -lpthread

# How to compile a C or C++ file, and generate automatic dependencies.
%.o:	    	    	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $< -c -o $@

%.o:	    	    	%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) $< -c -o $@

# How to link an executable. 
%:  	    	    	%.o
%:  	    	    	%.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# How to generate assembly.
%.s:	    	    	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $< -S -o $@

%.s:	    	    	%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) $< -S -o $@

#-------------------------------------------------------------------------------

src/csv2:   	    	    strtod/parse_double_6.o src/main.o

src/libcsv2.a: 	    	    strtod/parse_double_6.o src/csv2.o
	rm -f $@
	ar -cq $@ $^

strtod/benchmark:   	    strtod/pandas.o \
			    strtod/str2dbl.o \
                            strtod/intstrtod.o \
                            strtod/parse_double_6.o \
                            strtod/parse_double_7.o \
                            strtod/gay/dtoa.o \
                            double-conversion/bignum.o \
                            double-conversion/bignum-dtoa.o \
                            double-conversion/cached-powers.o \
                            double-conversion/diy-fp.o \
                            double-conversion/double-conversion.o \
                            double-conversion/fast-dtoa.o \
                            double-conversion/fixed-dtoa.o \
                            double-conversion/strtod.o

strtod/benchmark2:   	    strtod/pandas.o \
			    strtod/str2dbl.o \
                            strtod/parse_double_6.o \
                            strtod/parse_double_7.o \
                            strtod/gay/dtoa.o \
                            double-conversion/bignum.o \
                            double-conversion/bignum-dtoa.o \
                            double-conversion/cached-powers.o \
                            double-conversion/diy-fp.o \
                            double-conversion/double-conversion.o \
                            double-conversion/fast-dtoa.o \
                            double-conversion/fixed-dtoa.o \
                            double-conversion/strtod.o

strtod/test_s2dc:   	    double-conversion/bignum.o \
                            double-conversion/bignum-dtoa.o \
                            double-conversion/cached-powers.o \
                            double-conversion/diy-fp.o \
                            double-conversion/double-conversion.o \
                            double-conversion/fast-dtoa.o \
                            double-conversion/fixed-dtoa.o \
                            double-conversion/strtod.o

strtod/test_exh:    	    strtod/parse_double_7.o

strtod/pd7: 	    	    strtod/parse_double_7.o
strtod/bm7: 	    	    strtod/parse_double_7.o

strtod/test_intstrtod:	    strtod/intstrtod.o

strtod/test:   	    	    strtod/parse_double_6.o

strtod/gay/dtoa.o: CFLAGS += -DIEEE_8087

.PHONY: clean
clean:
	rm -f src/*.o strtod/*.o
	rm -f src/csv1 src/csv2
	rm -f strtod/benchmark strtod/test_intstrtod strtod/test_parse_double strtod/nans

#-------------------------------------------------------------------------------

GTEST_ROOT  	= vendor/googletest
GTEST_LIB   	= $(GTEST_ROOT)/make/gtest_main.a

$(GTEST_LIB): 
	$(MAKE) -C $(GTEST_ROOT)/make

strtod/test_parse_double:	strtod/parse_double_6.o $(GTEST_LIB)
CPPFLAGS    	+= -I $(GTEST_ROOT)/include

