
CFLAGS := -I.. $(CFLAGS)
CXXFLAGS := -I.. -Wall -Wextra -std=c++11 -g $(CXXFLAGS)

LDFLAGS := $(LDFLAGS) -lboost_unit_test_framework -lpthread
LIBS := ../libqm-dsp.a

TESTS	:= test-mathutilities test-window test-filter test-filtfilt test-fft test-dct test-pvoc test-resampler test-medianfilter test-getkeymode test-chromagram

VG	:= valgrind -q

all: $(TESTS)
#	for t in $(TESTS); do echo "Running $$t"; $(VG) ./"$$t" || exit 1; done
	for t in $(TESTS); do echo "Running $$t"; ./"$$t" || exit 1; done

test-medianfilter: TestMedianFilter.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-mathutilities: TestMathUtilities.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-window: TestWindow.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-filter: TestFilter.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-filtfilt: TestFiltFilt.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-fft: TestFFT.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-dct: TestDCT.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-pvoc: TestPhaseVocoder.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-resampler: TestResampler.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-chromagram: TestChromagram.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test-getkeymode: TestGetKeyMode.o $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS)

TestMathUtilities.o: $(LIBS)
TestMedianFilter.o: $(LIBS)
TestWindow.o: $(LIBS)
TestFilter.o: $(LIBS)
TestFiltFilt.o: $(LIBS)
TestFFT.o: $(LIBS)
TestDCT.o: $(LIBS)
TestPhaseVocoder.o: $(LIBS)
TestResampler.o: $(LIBS)
TestChromagram.o: $(LIBS)
TestGetKeyMode.o: $(LIBS)

clean: 
	rm -f *.o $(TESTS)

