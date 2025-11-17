package com.facebook.encapp.utils;

import android.annotation.SuppressLint;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;
import android.os.SystemClock;

import com.facebook.encapp.Encoder;
import com.facebook.encapp.MainActivity;
import com.facebook.encapp.proto.Test;
import com.google.protobuf.util.JsonFormat;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import com.google.gson.Gson;
import com.google.gson.stream.JsonWriter;

import java.io.IOException;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Dictionary;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

public class Statistics {
    final static String TAG = "encapp.statistics";
    public static String NA = "na";
    private final String mId;
    private final String mDesc;
    private final ArrayList<FrameInfo> mEncodingFrames;
    private final HashMap<Long, FrameInfo> mDecodingFrames;
    int mEncodingProcessingFrames = 0;
    Test mTest;
    Date mStartDate;
    SystemLoad mLoad = new SystemLoad();
    private String mEncodedfile = "";
    private String mStatus = "";
    private String mCodec;
    private long mStartTime = -1;
    private long mStopTime = -1;
    private MediaFormat mEncoderConfigFormat;
    private MediaFormat mEncoderMediaFormat;
    private MediaFormat mDecoderMediaFormat;
    private String mDecoderName = "";
    private String mAppVersion = "";
    private boolean mIsEncoderHw = false;
    private boolean mIsDecoderHw = false;
    private static int sFrameCounter = 0;
    private static float frameRate = -1f;

    long mstartbattery;
    long mendbattery;
    double mVoltage;
    double mstartvoltage;
    double mendvoltage;
    int mstartavgcurrent;
    int mendavgcurrent;
    int startbatterycapacity;
    int endbatterycapacity;
    int startchargecounter;
    int endchargecounter;
    int startcurrentnow;
    int endcurrentnow;
    long startenergycounter;
    long endenergycounter;
    long mbatteryDifference;
    double mtotalEnergyConsumption;
    long mloopbacks = 0;
    long finalaccumulated = 0;
    double menergy_consumption_per_loopback = 0.0;

    private static List<String> MEDIAFORMAT_KEY_STRING_LIST = Arrays.asList(
        MediaFormat.KEY_FRAME_RATE,
        MediaFormat.KEY_CODECS_STRING,
        MediaFormat.KEY_MIME,
        MediaFormat.KEY_TEMPORAL_LAYERING
    );

    private static List<String> MEDIAFORMAT_KEY_INT_LIST = Arrays.asList(
        MediaFormat.KEY_BITRATE_MODE,
        MediaFormat.KEY_BIT_RATE,
        MediaFormat.KEY_COLOR_FORMAT,
        MediaFormat.KEY_COLOR_RANGE,
        MediaFormat.KEY_COLOR_STANDARD,
        MediaFormat.KEY_COLOR_TRANSFER,
        // MediaFormat.KEY_COLOR_TRANSFER_REQUEST,  // api 31
        MediaFormat.KEY_COMPLEXITY,
        // MediaFormat.KEY_CROP_BOTTOM,  // api 33
        // MediaFormat.KEY_CROP_LEFT,  // api 33
        // MediaFormat.KEY_CROP_RIGHT,  // api 33
        // MediaFormat.KEY_CROP_TOP,  // api 33
        MediaFormat.KEY_ENCODER_DELAY,
        MediaFormat.KEY_ENCODER_PADDING,
        MediaFormat.KEY_HEIGHT,
        MediaFormat.KEY_INTRA_REFRESH_PERIOD,
        MediaFormat.KEY_I_FRAME_INTERVAL,
        MediaFormat.KEY_LATENCY,
        MediaFormat.KEY_LEVEL,
        MediaFormat.KEY_LOW_LATENCY,
        MediaFormat.KEY_MAX_B_FRAMES,
        MediaFormat.KEY_MAX_HEIGHT,
        MediaFormat.KEY_MAX_INPUT_SIZE,
        MediaFormat.KEY_MAX_WIDTH,
        MediaFormat.KEY_PIXEL_ASPECT_RATIO_HEIGHT,
        MediaFormat.KEY_PIXEL_ASPECT_RATIO_WIDTH,
        MediaFormat.KEY_PREPEND_HEADER_TO_SYNC_FRAMES,
        MediaFormat.KEY_PRIORITY,
        MediaFormat.KEY_PROFILE,
        MediaFormat.KEY_QUALITY,
        MediaFormat.KEY_ROTATION,
        MediaFormat.KEY_SLICE_HEIGHT,
        MediaFormat.KEY_STRIDE,
        MediaFormat.KEY_TILE_HEIGHT,
        MediaFormat.KEY_TILE_WIDTH,
        // MediaFormat.KEY_VIDEO_ENCODING_STATISTICS_LEVEL,  // api 33
        // MediaFormat.KEY_VIDEO_QP_AVERAGE,  // api 33
        // MediaFormat.KEY_VIDEO_QP_B_MAX,  // api 31
        // MediaFormat.KEY_VIDEO_QP_B_MIN,  // api 31
        // MediaFormat.KEY_VIDEO_QP_I_MAX,  // api 31
        // MediaFormat.KEY_VIDEO_QP_I_MIN,  // api 31
        // MediaFormat.KEY_VIDEO_QP_MAX,  // api 31
        // MediaFormat.KEY_VIDEO_QP_MIN,  // api 31
        // MediaFormat.KEY_VIDEO_QP_P_MAX,  // api 31
        // MediaFormat.KEY_VIDEO_QP_B_MIN,  // api 31
        MediaFormat.KEY_WIDTH
    );
    private static List<String> MEDIAFORMAT_KEY_LONG_LIST = Arrays.asList(
        MediaFormat.KEY_DURATION,
        MediaFormat.KEY_REPEAT_PREVIOUS_FRAME_AFTER
    );
    private static List<String> MEDIAFORMAT_KEY_FLOAT_LIST = Arrays.asList(
        MediaFormat.KEY_CAPTURE_RATE,
        MediaFormat.KEY_FRAME_RATE,
        MediaFormat.KEY_MAX_FPS_TO_ENCODER
    );

    public Statistics(String desc, Test test) {
//        Encoder coder = null;
        mDesc = desc;
        mEncodingFrames = new ArrayList<>(20);
        mDecodingFrames = new HashMap<>(20);
        mTest = test;
        mStartDate = new Date();
        mId = "encapp_" + UUID.randomUUID().toString();
//        mId = "encapp_" + coder.getOutputFilename();
    }

    public void setAppVersion(String mAppVersion) {
        this.mAppVersion = mAppVersion;
    }
    public String getId() {
        return mId;
    }

    public String toString() {
        ArrayList<FrameInfo> allEncodingFrames = mEncodingFrames;
        Comparator<FrameInfo> compareByPts = (FrameInfo o1, FrameInfo o2) -> Long.valueOf(o1.getPts()).compareTo(Long.valueOf(o2.getPts()));
        Collections.sort(allEncodingFrames, compareByPts);

        StringBuffer buffer = new StringBuffer();
        int counter = 0;
        for (FrameInfo info : allEncodingFrames) {
            buffer.append(mId + ", " +
                    counter + ", " +
                    info.isIFrame() + ", " +
                    info.getSize() + ", " +
                    info.getPts() + ", " +
                    info.getProcessingTime() + "\n");
            counter++;
        }

        return buffer.toString();
    }

    public void start() {
        mStartTime = SystemClock.elapsedRealtimeNanos();
        mLoad.start();
    }

    public void stop() {
        mStopTime = SystemClock.elapsedRealtimeNanos();
        mLoad.stop();
    }

    public void startEncodingFrame(long pts, int originalFrame) {
        FrameInfo frame = new FrameInfo(pts, originalFrame);
        try {
            if(frameRate == -1) {
                JSONObject settings = getSettingsFromMediaFormat(mDecoderMediaFormat);
                JSONObject encoderSettings = getSettingsFromMediaFormat(mEncoderMediaFormat);
//                Log.d(TAG, "Statistics settings : " + settings);
                if (settings.has("frame-rate")) {
                    frameRate = (float) settings.getDouble("frame-rate");
                } else if(encoderSettings.has("frame-rate")){
                    frameRate = (float) encoderSettings.getDouble("frame-rate");
                } else {
                    frameRate = -1f;
                }
            }
        } catch (JSONException e) {
            e.printStackTrace();
            frameRate = -1f;
        }
        sFrameCounter++;
//        Log.d(TAG, "sFrameCount : " + sFrameCounter);
//        Log.d(TAG, "sFrameRate : " + frameRate);
        if(sFrameCounter == Math.ceil(frameRate)) {
            frame.setAverageCurrent();
            frame.setBatteryVoltage();
            frame.setBatteryCapacity();
            frame.setBatteryChargeCounter();
            frame.setBatteryCurrentNow();
            frame.setBatteryEnergyCounter();
            sFrameCounter = 0;
        }
        frame.start();
        mEncodingFrames.add(frame);
        mEncodingProcessingFrames += 1;
    }

    public FrameInfo stopEncodingFrame(long pts, long size, boolean isIFrame) {
        FrameInfo frame = getClosestMatch(pts);
        if (frame != null) {
            frame.stop();
            frame.setSize(size);
            frame.isIFrame(isIFrame);
        } else {
            Log.e(TAG, "No matching pts! Error in time handling. Pts = " + pts);
        }
        mEncodingProcessingFrames -= 1;
        return frame;
    }

    private FrameInfo getClosestMatch(long pts) {
        long minDist = Long.MAX_VALUE;
        FrameInfo match = null;
        for (int i = mEncodingFrames.size() - 1; i  >= 0; i--) {
            FrameInfo info = mEncodingFrames.get(i);
            if (info == null) {
                Log.e(TAG, "Failed to lookip object at " + i);
            }
            long dist = Math.abs(pts - info.getPts());
            if (dist <= minDist) {
                minDist = dist;
                match = info;
            } else if (dist > minDist) {
                return match;
            }
        }
        return match;
    }

    public void startDecodingFrame(long pts, long size, int flags) {
        FrameInfo frame = new FrameInfo(pts);
        frame.setSize(size);
        frame.setFlags(flags);
        frame.start();
        mDecodingFrames.put(Long.valueOf(pts), frame);
    }

    public FrameInfo stopDecodingFrame(long pts) {
        FrameInfo frame = mDecodingFrames.get(Long.valueOf(pts));
        if (frame != null) {
            frame.stop();
        }

        return frame;
    }

    public long getProcessingTime() {
        return mStopTime - mStartTime;
    }

    public int getEncodedFrameCount() {
        return mEncodingFrames.size();
    }

    public int getDecodedFrameCount() {
        return mDecodingFrames.size();
    }

    public void setCodec(String codec) {
        mCodec = codec;
    }

    public int getAverageBitrate() {
        ArrayList<FrameInfo> allFrames = mEncodingFrames;
        Comparator<FrameInfo> compareByPts = (FrameInfo o1, FrameInfo o2) -> Long.valueOf(o1.getPts()).compareTo(Long.valueOf(o2.getPts()));
        Collections.sort(allFrames, compareByPts);
        int framecount = allFrames.size();
        if (framecount > 0) {
            long startPts = allFrames.get(0).mPts;
            //We just ignore the last frame, for the average does not mean much.
            long lastTime = allFrames.get(allFrames.size() - 1).mPts;
            double totalTime = ((double) (lastTime - startPts)) / 1000000.0;
            long totalSize = 0;
            for (FrameInfo info : allFrames) {
                totalSize += info.getSize();
            }
            totalSize -= allFrames.get(framecount - 1).mSize;
            return (int) (Math.round(8 * totalSize / (totalTime))); // bytes/Secs -> bit/sec
        } else {
            return 0;
        }
    }

    public void setEncodedfile(String filename) {
        mEncodedfile = filename;
    }
    public void setStatus(String status) {
        mStatus = status;
    }

    public void BatteryTest(long startbattery,long endbattery,double Voltage, double StartVoltage, double EndVoltage, int startAvgCurrent, int endAvgCurrent, int startBatteryCapacity, int endBatteryCapacity, int startChargeCounter, int endChargeCounter, int startCurrentNow, int endCurrentNow, long startEnergyCounter, long endEnergyCounter) {
        mstartbattery = startbattery; //In MicroAmps
        mendbattery = endbattery; //In MicroAmps
        mstartavgcurrent = startAvgCurrent;
        mendavgcurrent = endAvgCurrent;
        startbatterycapacity = startBatteryCapacity;
        endbatterycapacity = endBatteryCapacity;
        startchargecounter = startChargeCounter;
        endchargecounter = endChargeCounter;
        startcurrentnow = startCurrentNow;
        endcurrentnow = endCurrentNow;
        startenergycounter = startEnergyCounter;
        endenergycounter = endEnergyCounter;
        if(Voltage > 10) {
            mVoltage = Voltage/1000; //In Volts
        }
        else {
            mVoltage = Voltage;
        }
        if(StartVoltage > 10) {
            mstartvoltage = StartVoltage/1000;
        }
        else {
            mstartvoltage = StartVoltage;
        }
        if(EndVoltage > 10) {
            mendvoltage = EndVoltage/1000;
        }
        else {
            mendvoltage = EndVoltage;
        }
//        mVoltage = Voltage/1000; //In Volts
        mbatteryDifference = startbattery - endbattery; //In MicroAmps
        mtotalEnergyConsumption = mbatteryDifference * mVoltage;
//        if(mloopbacks > 0) {
//            menergy_consumption_per_loopback = mtotalEnergyConsumption/mloopbacks;//In microwatts
//        }
    }

//    public void LoopbackData(long no_of_loopbacks, long accumulatedtime) {
//        mloopbacks = no_of_loopbacks;
//        finalaccumulated = accumulatedtime;
//    }

    public void setEncoderMediaFormat(MediaFormat format) {
        mEncoderMediaFormat = format;
    }

    public void setEncoderConfigMediaFormat(MediaFormat format) {
        mEncoderConfigFormat = format;
    }

    public void setDecoderMediaFormat(MediaFormat format) {
        mDecoderMediaFormat = format;
    }

    public void setDecoder(String decoderName) {
        mDecoderName = decoderName;
    }

    public void setEncoderIsHardwareAccelerated(boolean accelerated) { mIsEncoderHw = accelerated; }

    public void setDecoderIsHardwareAccelerated(boolean accelerated) { mIsDecoderHw = accelerated; }

    private JSONObject getSettingsFromMediaFormat(MediaFormat mediaFormat) {
        // Log.d(TAG, "mediaFormat: " + mediaFormat);
        JSONObject json = new JSONObject();
        if (mediaFormat == null) {
            return json;
        }

        if (Build.VERSION.SDK_INT >= 29) {
            Set<String> features = mediaFormat.getFeatures();
            for (String feature : features) {
                Log.d(TAG, "MediaFormat: " + feature);
            }

            Set<String> keys = mediaFormat.getKeys();
            for (String key : keys) {
                int type = mediaFormat.getValueTypeForKey(key);
                try {
                    switch (type) {
                        case MediaFormat.TYPE_BYTE_BUFFER:
                            json.put(key, "bytebuffer");
                            break;
                        case MediaFormat.TYPE_FLOAT:
                            json.put(key, mediaFormat.getFloat(key));
                            break;
                        case MediaFormat.TYPE_INTEGER:
                            json.put(key, mediaFormat.getInteger(key));
                            break;
                        case MediaFormat.TYPE_LONG:
                            json.put(key, mediaFormat.getLong(key));
                            break;
                        case MediaFormat.TYPE_NULL:
                            json.put(key, "");
                            break;
                        case MediaFormat.TYPE_STRING:
                            json.put(key, mediaFormat.getString(key));
                            break;
                    }
                } catch (JSONException jex) {
                    Log.d(TAG, key + ", Failed to parse MediaFormat: " + jex.getMessage());
                }
            }
        } else {
            // go through some settings (API 28 or lower)
            try {
                // keys containing different types
                for (String key : MEDIAFORMAT_KEY_STRING_LIST) {
                    if (mediaFormat.getString(key) != null) {
                        json.put(key, mediaFormat.getString(key));
                    }
                }
                for (String key : MEDIAFORMAT_KEY_INT_LIST) {
                    try {
                        json.put(key, mediaFormat.getInteger(key));
                    } catch (NullPointerException e) {
                        // key does not exist or the stored value for the key is null
                    }
                }
                for (String key : MEDIAFORMAT_KEY_LONG_LIST) {
                    try {
                        json.put(key, mediaFormat.getLong(key));
                    } catch (NullPointerException e) {
                        // key does not exist or the stored value for the key is null
                    }
                }
                for (String key : MEDIAFORMAT_KEY_FLOAT_LIST) {
                    try {
                        json.put(key, mediaFormat.getInteger(key));
                        json.put(key, mediaFormat.getFloat(key));
                    } catch (NullPointerException e) {
                        // key does not exist or the stored value for the key is null
                    }
                }
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
        return json;
    }

    private String getVal(MediaFormat format, String key, String val) {
        if (format == null) {
            return val;
        }
        if (format.containsKey(key)) {
            return format.getString(key);
        }
        return val;
    }

    @SuppressLint("DefaultLocale")
    public void writeJSON(Writer writer) throws IOException, JSONException {
        Log.d(TAG, "Write stats for " + mId);
        JSONObject json = null;
        try {
            json = new JSONObject();

            json.put("id", mId);
            json.put("description", mDesc);

            //Battery info
            JSONObject batteryData = new JSONObject();
            batteryData.put("Start Battery (In MicroAmps)", mstartbattery);
            batteryData.put("End Battery (In MicroAmps)",mendbattery);
            batteryData.put("StartVoltage", mstartvoltage);
            batteryData.put("EndVoltage:",mendvoltage);
            batteryData.put("StartAvgCurrent:",mstartavgcurrent);
            batteryData.put("EndAvgCurrent:",mendavgcurrent);
            batteryData.put("StartBatteryCapacity:",startbatterycapacity);
            batteryData.put("EndBatteryCapacity:",endbatterycapacity);
            batteryData.put("StartChargeCounter:",startchargecounter);
            batteryData.put("EndChargeCounter:",endchargecounter);
            batteryData.put("StartCurrentNow:",startcurrentnow);
            batteryData.put("EndCurrentNow:",endcurrentnow);
            batteryData.put("StartEnergyCounter:",startenergycounter);
            batteryData.put("EndEnergyCounter:",endenergycounter);
            batteryData.put("Battery Difference (In MicroAmps)",mbatteryDifference);
            batteryData.put("Voltage (In Volts)",mVoltage);
            batteryData.put("Total Energy Consumption (In microwatts)",mtotalEnergyConsumption);
            json.put("battery_data", batteryData);

            //Loopback info
//            JSONObject loopbackData = new JSONObject();
//            loopbackData.put("Number_of_iteration", mloopbacks);
//            loopbackData.put("Energy_Consumption_single_iteration (In microwatts)", menergy_consumption_per_loopback);
//            String accumulatedInMins = String.valueOf(TimeUnit.MINUTES.convert(finalaccumulated, TimeUnit.MICROSECONDS));
//            loopbackData.put("Total_time_taken_to_finish_transcoding", accumulatedInMins+" mins");
//            json.put("Loopback_data", loopbackData);


            // convert the test configuration to json
            String jsonStr = JsonFormat.printer().includingDefaultValueFields().print(mTest);
            json.put("test", new JSONObject(jsonStr));
            // add environment
            json.put("environment", new JSONObject(System.getenv()));
            // derived test configuration items
            if (mEncodingFrames.size() > 0) {
                json.put("codec", mCodec);
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    json.put("encoder_hw_accelerated", mIsEncoderHw);
                }
            }
            json.put("meanbitrate", getAverageBitrate());
            json.put("date", mStartDate.toString());
            Log.d(TAG, "log app version: " + mAppVersion);
            json.put("encapp_version", mAppVersion);
            json.put("proctime", getProcessingTime());
            json.put("framecount", getEncodedFrameCount());
            json.put("encodedfile", mEncodedfile);
            String[] tmp = mTest.getInput().getFilepath().split("/");
            json.put("sourcefile", tmp[tmp.length - 1]);

            json.put("encoder_media_format", getSettingsFromMediaFormat(mEncoderMediaFormat));
            if (mDecodingFrames.size() > 0) {
                json.put("decoder", mDecoderName);
                json.put("decoder_media_format", getSettingsFromMediaFormat(mDecoderMediaFormat));

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    json.put("decoder_hw_accelerated", mIsDecoderHw);
                }
            }
            json.put("Status msg:", mStatus);
            ArrayList<FrameInfo> allFrames = mEncodingFrames;
            Comparator<FrameInfo> compareByPts = (FrameInfo o1, FrameInfo o2) -> Long.valueOf(o1.getPts()).compareTo(Long.valueOf(o2.getPts()));
            Collections.sort(allFrames, compareByPts);
            int counter = 0;
            JSONArray jsonArray = new JSONArray();

            JSONObject obj = null;
            ArrayList<FrameInfo> frameCopy = (ArrayList<FrameInfo>) allFrames.clone();
            for (FrameInfo info : frameCopy) {
                obj = new JSONObject();
                obj.put("frame", counter++);
                obj.put("original_frame", info.getOriginalFrame());
                obj.put("iframe", (info.isIFrame()) ? 1 : 0);
                obj.put("size", info.getSize());
                obj.put("pts", info.getPts());
                if (info.getStopTime() == 0) {
                    Log.w(TAG, "Frame did not finish: " + (counter - 1) + ", orig: " + info.getOriginalFrame());
                    obj.put("proctime", 0);
                } else {
                    obj.put("proctime", info.getProcessingTime());
                }
                obj.put("starttime", info.getStartTime());
                obj.put("stoptime", info.getStopTime());
                obj.put("avgcurrent", info.getAverageCurrent());
                obj.put("currentvoltage", info.getBatteryVoltage());
                obj.put("BatteryCapacity", info.getBatteryCapacity());
                obj.put("BatteryChargeCounter", info.getBatteryChargeCounter());
                obj.put("BatteryCurrentNow", info.getBatteryCurrentNow());
                obj.put("BatteryEnergyCounter", info.getBatteryEnergyCounter());

                Dictionary<String, Object> dict = info.getInfo();
                if (dict != null) {
                    Enumeration<String> keys = dict.keys();
                    while (keys.hasMoreElements()) {
                        String key = keys.nextElement();
                        obj.put(key, dict.get(key).toString());
                    }
                }
                jsonArray.put(obj);
            }
            json.put("frames", jsonArray);
            if (mDecodingFrames.size() > 0) {

                allFrames = new ArrayList<>(mDecodingFrames.values());
                Collections.sort(allFrames, compareByPts);
                counter = 1;
                jsonArray = new JSONArray();

                obj = null;
                for (FrameInfo info : allFrames) {
                    long proc_time = info.getProcessingTime();
                    if (proc_time > 0) {
                        obj = new JSONObject();

                        obj.put("frame", counter++);
                        obj.put("flags", info.getFlags());
                        obj.put("size", info.getSize());
                        obj.put("pts", info.getPts());
                        obj.put("proctime", info.getProcessingTime());
                        obj.put("starttime", info.getStartTime());
                        obj.put("stoptime", info.getStopTime());
                        Dictionary<String, Object> dict = info.getInfo();
                        if (dict != null) {
                            Enumeration<String> keys = dict.keys();
                            while (keys.hasMoreElements()) {
                                String key = keys.nextElement();
                                obj.put(key, dict.get(key).toString());
                            }
                        }
                        jsonArray.put(obj);
                    }
                }
                json.put("decoded_frames", jsonArray);
            }

            //CPU info
            JSONObject cpuData = new JSONObject();
            int cores = Runtime.getRuntime().availableProcessors();
            cpuData.put("cores", String.valueOf(cores));
            json.put("cpu_data", cpuData);


            // GPU info
            JSONObject gpuData = new JSONObject();
            HashMap<String, String> gpuInfo = mLoad.getGPUInfo();

            for (String key : gpuInfo.keySet()) {
                gpuData.put(key, gpuInfo.get(key));
            }

            counter = 1;
            jsonArray = new JSONArray();

            int[] gpuload = mLoad.getGPULoadPercentagePerTimeUnit();
            float timer = (float) (1.0 / mLoad.getSampleFrequency());
            obj = null;
            for (int load : gpuload) {
                obj = new JSONObject();
                int msec = Math.round(counter * timer * 1000);
                obj.put("time_sec", msec / 1000.0);
                obj.put("load_percentage", load);
                counter++;
                jsonArray.put(obj);
            }
            gpuData.put("gpu_load_percentage", jsonArray);
            obj = null;
            jsonArray = new JSONArray();
            counter = 0;
            for (String clock : mLoad.getGPUClockFreqPerTimeUnit()) {
                obj = new JSONObject();

                int msec = Math.round(counter * timer * 1000);
                obj.put("time_sec", msec / 1000.0);
                obj.put("clock_MHz", clock);
                counter++;
                jsonArray.put(obj);
            }
            gpuData.put("gpu_clock_freq", jsonArray);
            json.put("gpu_data", gpuData);
//            writer.write(json.toString(2));

            JsonWriter jsonWriter = new JsonWriter(writer);
            jsonWriter.setIndent("  ");

            Gson gson = new Gson();

            gson.toJson(json, JSONObject.class, jsonWriter);
            jsonWriter.close();

        } catch (JSONException e) {
            Log.e(TAG, "Failed writing stats");
            e.printStackTrace();
        } catch (Exception e) {
            Log.e(TAG, "Failed writing stats");
            e.printStackTrace();
        }
        Log.d(TAG, "Done written stats report: " + mId);
        sFrameCounter = 0;
        writer.close();
    }

}
