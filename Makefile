# Disable built-in rules.
.SUFFIXES: 

CFLAGS	    	= -std=c99   -Wall -O3 -g # -DNDEBUG
CXXFLAGS    	= -std=c++14 -Wall -O3 -g # -DNDEBUG

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

src/csv2:   	    	    src/parse_double.o

strtod/benchmark:   	    strtod/pandas.o strtod/str2dbl.o strtod/intstrtod.o

strtod/test_intstrtod:	    strtod/intstrtod.o


