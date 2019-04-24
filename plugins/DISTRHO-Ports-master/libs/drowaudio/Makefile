# Compile the drowaudio shared lib

all:
	$(MAKE) -C build-drowaudio

clean:
	$(MAKE) clean -C build-drowaudio
	rm -rf build-*/intermediate

distclean: clean
	rm -f build-*/Makefile
	rm -f build-*/*.make
	rm -f build-*/*.sln
	rm -f build-*/*.vcproj
