.POSIX:
.SUFFIXES:
.SUFFIXES: .cpp .o .unittest .profraw .profdata .coverage

VERSION    = 1.0.0

CXX        = clang++
LD         = ld
XCRUN      = xcrun
CPROF      = $(XCRUN) llvm-profdata
CCOV       = $(XCRUN) llvm-cov

PREFIX     = /opt/local
LIBDIR     = $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

SANITIZE   = -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
COVERAGE   = -fprofile-instr-generate -fcoverage-mapping
OPTS       = -std=c++17 -Werror -Weverything -Wno-c++98-compat -Wno-padded -Wno-poison-system-directories -Wno-global-constructors -Wno-exit-time-destructors

.PHONY: all
all: libargparse.a argparse.coverage

libargparse.a: argparse.o
	$(LD) -r $^ -o $@

.cpp.o:
	$(CXX) $(OPTS) -c $^ -o $@

libargparse.pc:
	printf 'prefix=%s\n' "$(PREFIX)" > $@
	printf 'exec_prefix=$${prefix}\n' >> $@
	printf 'includedir=$${prefix}/include\n' >> $@
	printf 'libdir=$${prefix}/lib\n' >> $@
	printf 'Name: libargparse\n' >> $@
	printf 'Version: %s\n' "$(VERSION)" >> $@
	printf 'Description: Command Line Argument Parser\n' >> $@

argparse.unittest: test_argparse.cpp

.profdata.coverage:
	$(CCOV) show $*.unittest -instr-profile=$< $*.cpp > $@
	! grep " 0|" $@
	echo PASS $@

.profraw.profdata:
	$(CPROF) merge -sparse $< -o $@

.unittest.profraw:
	LLVM_PROFILE_FILE=$@ ./$<

.cpp.unittest:
	$(CXX) $(SANITIZE) $(COVERAGE) $(OPTS) $^ -o $@

.PHONY: install
install: argparse.hpp libargparse.a libargparse.pc
	mkdir -p $(INCLUDEDIR)/libargparse
	mkdir -p $(LIBDIR)/pkgconfig
	install -m644 argparse.hpp $(INCLUDEDIR)/libargparse/argparse.hpp
	install -m644 libargparse.a $(LIBDIR)/libargparse.a
	install -m644 libargparse.pc $(LIBDIR)/pkgconfig/libargparse.pc

.PHONY: uninstall
uninstall:
	rm -f $(INCLUDEDIR)/libargparse/argparse.hpp
	rm -f $(LIBDIR)/libargparse.a
	rm -f $(LIBDIR)/pkgconfig/libargparse.pc

.PHONY: clean
clean:
	rm -rf libargparse.a libargparse.pc *.o *.coverage *.profdata *.profraw *.unittest*
