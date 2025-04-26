
package com.breakfastquay.rubberband.test;

import com.breakfastquay.rubberband.RubberBandStretcher;
import com.breakfastquay.rubberband.RubberBandLiveShifter;

import java.util.TreeMap;

public class RubberBandTest
{
    public static void exerciseStretcher() {
        
        int channels = 1;
        int rate = 44100;
        
        RubberBandStretcher stretcher = new RubberBandStretcher
            (rate,
             channels,
             RubberBandStretcher.OptionEngineFiner +
             RubberBandStretcher.OptionProcessOffline,
             1.0,
             1.0);

        stretcher.setTimeRatio(1.5);
        stretcher.setPitchScale(0.8);
        
        System.out.println
            (String.format("Channel count: %d\n" +
                           "Time ratio: %f\n" +
                           "Pitch scale: %f\n" +
                           "Preferred start pad: %d\n" +
                           "Start delay: %d\n" +
                           "Process size limit: %d",
                           stretcher.getChannelCount(),
                           stretcher.getTimeRatio(),
                           stretcher.getPitchScale(),
                           stretcher.getPreferredStartPad(),
                           stretcher.getStartDelay(),
                           stretcher.getProcessSizeLimit()
                ));

        int blocksize = 1024;
        int blocks = 400;
        double freq = 440.0;
        
        stretcher.setMaxProcessSize(blocksize);

        TreeMap<Long, Long> keyFrameMap = new TreeMap<Long, Long>();
        keyFrameMap.put((long)(3 * rate), (long)(4 * rate));
        keyFrameMap.put((long)(5 * rate), (long)(5 * rate));
        stretcher.setKeyFrameMap(keyFrameMap);

        float[][] buffer = new float[channels][blocksize];

        int i0 = 0;

        for (int block = 0; block < blocks; ++block) {

            for (int c = 0; c < channels; ++c) {
                for (int i = 0; i < blocksize; ++i) {
                    buffer[c][i] = (float)Math.sin
                        ((double)i0 * freq * Math.PI * 2.0 / (double)rate);
                    if (i0 % rate == 0) {
                        buffer[c][i] = 1.f;
                    }
                    ++i0;
                }
            }
            
            stretcher.study(buffer, block + 1 == blocks);
        }

        i0 = 0;

        double sqrtotal = 0.0;
        int n = 0;

        for (int block = 0; block < blocks; ++block) {

            for (int c = 0; c < channels; ++c) {
                for (int i = 0; i < blocksize; ++i) {
                    buffer[c][i] = (float)Math.sin
                        ((double)i0 * freq * Math.PI * 2.0 / (double)rate);
                    if (i0 % rate == 0) {
                        buffer[c][i] = 1.f;
                    }
                    ++i0;
                }
            }
            
            stretcher.process(buffer, block + 1 == blocks);

            while (true) {
                int available = stretcher.available();
                if (available <= 0) {
                    break;
                }
                int requested = available;
                if (requested > blocksize) {
                    requested = blocksize;
                }
                int obtained = stretcher.retrieve(buffer, 0, requested);
                for (int i = 0; i < obtained; ++i) {
                    sqrtotal += (double)(buffer[0][i] * buffer[0][i]);
                    ++n;
                }
            }
        }

        System.out.println
            (String.format("in = %d, out = %d, rms = %f",
                           blocksize * blocks, n,
                           Math.sqrt(sqrtotal / (double)n)));
        
        stretcher.dispose();
    }

    public static void exerciseLiveShifter() {
        
        int channels = 1;
        int rate = 44100;
        
        RubberBandLiveShifter shifter = new RubberBandLiveShifter
            (rate,
             channels,
             0);

        shifter.setPitchScale(0.8);
        
        System.out.println
            (String.format("Channel count: %d\n" +
                           "Pitch scale: %f\n" +
                           "Block size: %d\n" +
                           "Start delay: %d",
                           shifter.getChannelCount(),
                           shifter.getPitchScale(),
                           shifter.getBlockSize(),
                           shifter.getStartDelay()
                ));

        int blocksize = shifter.getBlockSize();
        int blocks = 400;
        double freq = 440.0;
        
        float[][] inbuf = new float[channels][blocksize];
        float[][] outbuf = new float[channels][blocksize];

        int i0 = 0;

        double sqrtotal = 0.0;
        int n = 0;

        for (int block = 0; block < blocks; ++block) {

            for (int c = 0; c < channels; ++c) {
                for (int i = 0; i < blocksize; ++i) {
                    inbuf[c][i] = (float)Math.sin
                        ((double)i0 * freq * Math.PI * 2.0 / (double)rate);
                    if (i0 % rate == 0) {
                        inbuf[c][i] = 1.f;
                    }
                    ++i0;
                }
            }
            
            shifter.shift(inbuf, outbuf);

            for (int i = 0; i < blocksize; ++i) {
                sqrtotal += (double)(outbuf[0][i] * outbuf[0][i]);
                ++n;
            }
        }

        System.out.println
            (String.format("in = %d, out = %d, rms = %f",
                           blocksize * blocks, n,
                           Math.sqrt(sqrtotal / (double)n)));

        shifter.dispose();
    }

    public static void main(String[] args) {
        System.out.println("Exercising RubberBandStretcher through JNI...");
        exerciseStretcher();
        System.out.println("Exercising RubberBandLiveShifter through JNI...");
        exerciseLiveShifter();
        System.out.println("Done");
    }
    
}

