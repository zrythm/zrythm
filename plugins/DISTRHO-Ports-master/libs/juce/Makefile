# Compile the juce shared lib

all:
	$(MAKE) -C build-juce

clean:
	$(MAKE) clean -C build-juce
	rm -rf build-*/intermediate

distclean: clean
	rm -f build-*/Makefile
	rm -f build-*/*.make
	rm -f build-*/*.sln
	rm -f build-*/*.vcproj
