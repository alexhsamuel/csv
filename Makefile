# Disable built-in rules.
.SUFFIXES: 

CFLAGS	    	= -std=c99   -Wall -I . -O3 -g # -DNDEBUG
CXXFLAGS    	= -std=c++14 -Wall -I . -O3 -g -fpermissive # -DNDEBUG
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

src/csv2:   	    	    strtod/parse_double_6.o

strtod/benchmark:   	    strtod/pandas.o strtod/str2dbl.o strtod/intstrtod.o strtod/parse_double_6.o

strtod/test_intstrtod:	    strtod/intstrtod.o

strtod/test_parse_double:   strtod/parse_double_6.o

.PHONY: clean
clean:
	rm -f src/*.o strtod/*.o
	rm -f src/csv1 src/csv2
	rm -f strtod/benchmark strtod/test_intstrtod strtod/test_parse_double

