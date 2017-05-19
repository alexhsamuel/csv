# Disable built-in rules.
.SUFFIXES: 

CFLAGS	    	= -Wall -O3 -g
CXXFLAGS    	= $(CFLAGS) -std=c++14

# How to compile a C or C++ file, and generate automatic dependencies.
%.o:	    	    	%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $< -c -o $@

%.o:	    	    	%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) $< -c -o $@

# How to link an executable. 
%:  	    	    	%.o
%:  	    	    	%.o 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

#-------------------------------------------------------------------------------

src/csv1:   	    	src/parse_float.o

src/test_parse_float:	src/parse_float.o

