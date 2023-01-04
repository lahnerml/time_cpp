CXX=g++

CXX_STANDARD=c++11
CXX_FLAGS_ALL=-Wall -Wextra -Wpedantic -Werror
CXX_FLAGS_REL=-O3 -march=native
CXX_FLAGS_DEB=-O0 -g3

SRCS=main.cpp
NAME=time_cxx11

all: release

release:
	$(CXX) -std=$(CXX_STANDARD) $(CXX_FLAGS_ALL) $(CXX_FLAGS_REL) -o $(NAME) $(SRCS)

debug:
	$(CXX) -std=$(CXX_STANDARD) $(CXX_FLAGS_ALL) $(CXX_FLAGS_DEB) -o $(NAME)d $(SRCS)

clean: cleandebug cleanrelease

cleandebug:
	rm -rf $(NAME)d $(NAME)d.dSYM

cleanrelease:
	rm -rf $(NAME)
