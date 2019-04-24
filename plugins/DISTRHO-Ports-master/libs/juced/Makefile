# Compile the juced shared lib

all:
	$(MAKE) -C build-juced

clean:
	$(MAKE) clean -C build-juced
	rm -rf build-*/intermediate

distclean: clean
	rm -f build-*/Makefile
	rm -f build-*/*.make
	rm -f build-*/*.sln
	rm -f build-*/*.vcproj
