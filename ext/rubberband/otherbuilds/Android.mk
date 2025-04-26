
LOCAL_MODULE := rubberband
LOCAL_MODULE_FILENAME := librubberband-jni

LOCAL_C_INCLUDES := $(LOCAL_PATH)/rubberband $(LOCAL_PATH)/rubberband/src

RUBBERBAND_PATH := rubberband
RUBBERBAND_SRC_PATH := $(RUBBERBAND_PATH)/src

RUBBERBAND_JNI_FILES := \
	$(RUBBERBAND_SRC_PATH)/jni/RubberBandStretcherJNI.cpp

RUBBERBAND_SRC_FILES := \
	$(RUBBERBAND_SRC_PATH)/rubberband-c.cpp \
	$(RUBBERBAND_SRC_PATH)/RubberBandStretcher.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/AudioCurveCalculator.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/CompoundAudioCurve.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/HighFrequencyAudioCurve.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/SilentAudioCurve.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/PercussiveAudioCurve.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/R2Stretcher.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/StretcherChannelData.cpp \
	$(RUBBERBAND_SRC_PATH)/faster/StretcherProcess.cpp \
	$(RUBBERBAND_SRC_PATH)/common/Allocators.cpp \
	$(RUBBERBAND_SRC_PATH)/common/BQResampler.cpp \
	$(RUBBERBAND_SRC_PATH)/common/FFT.cpp \
	$(RUBBERBAND_SRC_PATH)/common/Log.cpp \
	$(RUBBERBAND_SRC_PATH)/common/Profiler.cpp \
	$(RUBBERBAND_SRC_PATH)/common/Resampler.cpp \
	$(RUBBERBAND_SRC_PATH)/common/StretchCalculator.cpp \
	$(RUBBERBAND_SRC_PATH)/common/sysutils.cpp \
	$(RUBBERBAND_SRC_PATH)/common/mathmisc.cpp \
	$(RUBBERBAND_SRC_PATH)/common/Thread.cpp \
	$(RUBBERBAND_SRC_PATH)/finer/R3Stretcher.cpp

LOCAL_SRC_FILES += \
	$(RUBBERBAND_JNI_FILES) \
        $(RUBBERBAND_SRC_FILES)

LOCAL_CFLAGS_DEBUG := \
	-g \
	-DWANT_TIMING \
	-DFFT_MEASUREMENT

LOCAL_CFLAGS_RELEASE := \
	-O3 \
	-ffast-math \
	-ftree-vectorize \
	-DNO_TIMING \
	-DNO_TIMING_COMPLETE_NOOP

LOCAL_CFLAGS := \
	-Wall \
	-I$(RUBBERBAND_PATH) \
	-I$(RUBBERBAND_SRC_PATH) \
	-DUSE_BQRESAMPLER \
	-DUSE_BUILTIN_FFT \
	-DLACK_POSIX_MEMALIGN \
	-DUSE_OWN_ALIGNED_MALLOC \
	-DLACK_SINCOS \
	-DNO_EXCEPTIONS \
	-DNO_THREADING \
	-DNO_THREAD_CHECKS \
	$(LOCAL_CFLAGS_RELEASE)

LOCAL_LDLIBS += -llog

TARGET_ARCH_ABI	:= armeabi-v7a
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

include $(BUILD_SHARED_LIBRARY)

