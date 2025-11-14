package com.facebook.encapp.utils;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.SystemClock;

import com.facebook.encapp.MainActivity;

import java.util.Dictionary;

public class FrameInfo {
    long mPts;
    long mSize;
    long mProcessTime;
    long mStartTime;
    long mStopTime;
    boolean mIsIframe;
    int mFlags;
    int mOriginalFrame;

    int mBatteryVoltage = -1;
    int mAverageCurrent = -1;

    Dictionary<String, Object> mInfo;

    public FrameInfo(long pts) {
        mPts = pts;
        mOriginalFrame = -1; // When this does not make sense
    }

    public FrameInfo(long pts, int originalFrame) {
        mPts = pts;
        mOriginalFrame = originalFrame;
    }

    public void setSize(long size) {
        mSize = size;
    }

    public long getSize() {
        return mSize;
    }

    public long getPts(){
        return mPts;
    }

    public int getOriginalFrame() {return mOriginalFrame;}

    public void isIFrame(boolean isIFrame) {
        mIsIframe = isIFrame;
    }

    public boolean isIFrame() {
        return mIsIframe;
    }

    public int getFlags() {return mFlags;}

    public void setFlags(int flags) {
        mFlags = flags;
    }
    public void start(){
        mStartTime = SystemClock.elapsedRealtimeNanos();
    }

    public void stop(){
        mStopTime = SystemClock.elapsedRealtimeNanos();
        if (mStopTime < mStartTime) {
            mStopTime = -1;
            mStartTime = 0;
        }
    }

    public long getProcessingTime() {
        return mStopTime - mStartTime;
    }

    public long getStartTime() { return mStartTime;}
    public long getStopTime() { return mStopTime;}

    public Dictionary getInfo() {
        return mInfo;
    }
    public void addInfo(Dictionary<String, Object> info) {
        mInfo = info;
    }

    public void setAverageCurrent() {
        Context ctx = MainActivity.getAppContext();
        if (ctx == null) { mAverageCurrent = -1; return; }

        BatteryManager bm = (BatteryManager) ctx.getSystemService(Context.BATTERY_SERVICE);
        mAverageCurrent = (bm != null)
                ? bm.getIntProperty(BatteryManager.BATTERY_PROPERTY_CURRENT_AVERAGE)
                : -1;
    }

    public void setBatteryVoltage() {
        Context ctx = MainActivity.getAppContext();
        if (ctx == null) { mBatteryVoltage = -1; return; }

        IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent status = ctx.registerReceiver(null, filter);
        mBatteryVoltage = (status != null)
                ? status.getIntExtra(BatteryManager.EXTRA_VOLTAGE, -1)
                : -1;
    }

    public int getBatteryVoltage() { return mBatteryVoltage; }
    public int getAverageCurrent() { return mAverageCurrent; }
}
