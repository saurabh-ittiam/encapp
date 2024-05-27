package com.facebook.encapp;

import static com.facebook.encapp.utils.MediaCodecInfoHelper.getMediaFormatValueFromKey;
import static com.facebook.encapp.utils.MediaCodecInfoHelper.mediaFormatComparison;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.util.Size;

import androidx.annotation.NonNull;

import com.facebook.encapp.proto.Test;
import com.facebook.encapp.utils.FileReader;
import com.facebook.encapp.utils.FrameInfo;
import com.facebook.encapp.utils.SizeUtils;
import com.facebook.encapp.utils.Statistics;
import com.facebook.encapp.utils.TestDefinitionHelper;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Dictionary;
import java.util.Hashtable;
import java.util.Locale;
import java.util.Set;


/**
 * Created by jobl on 2018-02-27.
 */

class BufferX264Encoder extends Encoder {
    protected static final String TAG = "encapp.buffer_x264_encoder";

    static{
        try {
            System.loadLibrary("x264");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load x264 library: " + e.getMessage());
        }
    }

    private native int Create(int width, int height);
    private native int EncodeFrame(ByteBuffer frameData, int frameSize);
    private native void Close();

    public BufferX264Encoder(Test test) {
        super(test);
        mStats = new Statistics("raw encoder", mTest);
    }

    public int JNI_x264_EcoderCall(Test test) {
        int JNIstatus = Create(1280, 720);
        return JNIstatus;
    }

    public String start() {
        Log.d(TAG, "** Raw buffer encoding - " + mTest.getCommon().getDescription() + " **");
        try {
            if (TestDefinitionHelper.checkBasicSettings(mTest)) {
                mTest = TestDefinitionHelper.updateBasicSettings(mTest);
            }
        } catch (RuntimeException e) {
            Log.e(TAG, "Error: " + e.getMessage());
        }
        if (mTest.hasRuntime())
            mRuntimeParams = mTest.getRuntime();
        if (mTest.getInput().hasRealtime())
            mRealtime = mTest.getInput().getRealtime();

        boolean useImage = false;

        mFrameRate = mTest.getConfigure().getFramerate();
        mWriteFile = !mTest.getConfigure().hasEncode() || mTest.getConfigure().getEncode();
        mSkipped = 0;
        mFramesAdded = 0;
        Size sourceResolution = SizeUtils.parseXString(mTest.getInput().getResolution());
        mRefFramesizeInBytes = (int) (sourceResolution.getWidth() * sourceResolution.getHeight() * 1.5);
        mYuvReader = new FileReader();

        if (!mYuvReader.openFile(checkFilePath(mTest.getInput().getFilepath()), mTest.getInput().getPixFmt())) {
            return "Could not open file";
        }

        int x264Result = JNI_x264_EcoderCall(mTest);
        if (x264Result != 1) {
            return "Failed to create x264 encoder";
        }

        try {
            Log.d(TAG, "Start encoder");
        } catch (Exception ex) {
            Log.e(TAG, "Start failed: " + ex.getMessage());
            return "Start encoding failed";
        }

        float mReferenceFrameRate = mTest.getInput().getFramerate();
        mKeepInterval = mReferenceFrameRate / mFrameRate;
        mRefFrameTime = calculateFrameTimingUsec(mReferenceFrameRate);

        boolean input_done = false;
        boolean output_done = false;
        synchronized (this) {
            Log.d(TAG, "Wait for synchronized start");
            try {
                mInitDone = true;
                wait(WAIT_TIME_MS);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        mStats.start();
        int current_loop = 1;
        while (!input_done || !output_done) {
            int index;
            if (mFramesAdded % 100 == 0) {
                Log.d(TAG, mTest.getCommon().getId() + " - BufferX264Encoder: frames: " + mFramesAdded +
                        " inframes: " + mInFramesCount +
                        " current_loop: " + current_loop +
                        " current_time: " + mCurrentTimeSec);
            }
            try {
                long timeoutUs = VIDEO_CODEC_WAIT_TIME_US;
                index = mCodec.dequeueInputBuffer(timeoutUs);
                int flags = 0;
                if (doneReading(mTest, mYuvReader, mInFramesCount, mCurrentTimeSec, false)) {
                    input_done = true;
                }
                if (mRealtime) {
                    sleepUntilNextFrame();
                }

                ByteBuffer byteBuffer = ByteBuffer.allocateDirect(mRefFramesizeInBytes);
                int size = -1;
                while (size < 0 && !input_done) {
                    try {
                        size = queueInputBufferEncoder(
                                mYuvReader,
                                mCodec,
                                byteBuffer,
                                index,
                                mInFramesCount,
                                flags,
                                mRefFramesizeInBytes,
                                useImage);
                        //EncodeFrame(byteBuffer, size);
                        mInFramesCount++;
                    } catch (IllegalStateException isx) {
                        Log.e(TAG, "Queue encoder failed, mess: " + isx.getMessage());
                    }
                    if (size == -2) {
                        continue;
                    } else if (size <= 0) {
                        mYuvReader.closeFile();
                        current_loop++;
                        if (doneReading(mTest, mYuvReader, mInFramesCount, mCurrentTimeSec, true)) {
                            input_done = true;
                            flags += 1;
                            size = queueInputBufferEncoder(
                                    mYuvReader,
                                    mCodec,
                                    byteBuffer,
                                    index,
                                    mInFramesCount,
                                    flags,
                                    mRefFramesizeInBytes,
                                    useImage);
                            //EncodeFrame(byteBuffer, size);
                        }

                        if (!input_done) {
                            Log.d(TAG, " *********** OPEN FILE AGAIN *******");
                            mYuvReader.openFile(mTest.getInput().getFilepath(), mTest.getInput().getPixFmt());
                            Log.d(TAG, "*** Loop ended start " + current_loop + "***");
                        }
                    }
                }

                if (size > 0) {
                    int encodeStatus = EncodeFrame(byteBuffer, size);
                    if (encodeStatus != 0) {
                        return "Failed to encode frame";
                    }
                }

            } catch (IllegalStateException ex) {
                Log.e(TAG, "QueueInputBuffer: IllegalStateException error");
                ex.printStackTrace();
                return "QueueInputBuffer: IllegalStateException error";
            }
        }
        mStats.stop();

        Log.d(TAG, "Close encoder and streams");
        Close();

        mYuvReader.closeFile();
        return "";
    }

    public void writeToBuffer(@NonNull MediaCodec codec, int index, boolean encoder) {
        // Not needed
    }

    public void readFromBuffer(@NonNull MediaCodec codec, int index, boolean encoder, MediaCodec.BufferInfo info) {
        // Not needed
    }

    public void stopAllActivity() {
        // Not needed
    }

    public void release() {
        // Not needed
    }
}
