package com.lody.virtual.server.location;


import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemClock;
import android.util.Log;
import android.util.SparseArray;

import com.lody.virtual.client.ipc.VActivityManager;
import com.lody.virtual.helper.utils.Reflect;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.atomic.AtomicReference;

public class VLocationManagerService extends ILocationManager.Stub {
    private final SparseArray<Location> mLocations = new SparseArray<>();
    private Context mContext;
    private static final AtomicReference<VLocationManagerService> gService = new AtomicReference<>();
    private final List<IGpsStatusListener> mGpsStatusListeners = new ArrayList<>();
    private final List<ILocationListener> mLocationListeners = new ArrayList<>();
    private final static boolean DEBUG = false;
    private final static int MSG_HANDLE_LOCATION = 1;
    private long mLastGPS, mLastLocation;
    private static final boolean REPORT_LOCATION_WITH_GPS_STATUS = false;
    final Random mRandom = new Random(System.currentTimeMillis());
    private final static int HANDLE_TIME_GPS = 5 * 1000;
    /**
     * 报告位置时间间隔
     */
    private final static int HANDLE_TIME = 10 * 1000;
    /***
     * 多少时间报告一次
     */
    private final static int HANDLE_TIME_LOCATION = 30 * 1000;

    private final HandlerThread mHandlerThread;

    private class WorkHandler extends Handler {
        public WorkHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_HANDLE_LOCATION:
                    handLocationChanged(false, false);
                    startHandleTask();
                    return;

            }
            super.handleMessage(msg);
        }
    }

    private final WorkHandler mHandler;

    public static void systemReady(Context context) {
        VLocationManagerService instance = new VLocationManagerService(context);
        gService.set(instance);
    }

    public static VLocationManagerService get() {
        return gService.get();
    }

    private VLocationManagerService(Context context) {
        mContext = context;
        mHandlerThread = new HandlerThread("location_work");
        mHandlerThread.start();
        mHandler = new WorkHandler(mHandlerThread.getLooper());
    }

    @Override
    public void setVirtualLocation(Location loc, String packageName, int userId) {
        synchronized (mLocations) {
            mLocations.put(userId, loc);
        }
        handLocationChanged(false, true);
    }

    @Override
    public boolean hasVirtualLocation(String packageName, int userId) {
        if (DEBUG) {
            return true;
        }
        synchronized (mLocations) {
            return mLocations.get(userId) != null;
        }
    }

    @Override
    public Location getVirtualLocation(Location loc, String packageName, int userId) {
        Location location;
        if (DEBUG) {
            location = new Location(LocationManager.GPS_PROVIDER);
            location.setAltitude(5.1f);
            //30.4770829328,114.6423339844
            location.setAccuracy(120f);
            location.setLatitude(30.479449d);
            location.setLongitude(114.66834d);

            Bundle extras = new Bundle();
            if (extras.get("satellites") == null) {
                extras.putInt("satellites", 5);
            }
            location.setExtras(extras);
        } else {
            synchronized (mLocations) {
                location = mLocations.get(userId);
            }
        }
        if (location != null) {
            location.setTime(System.currentTimeMillis());
            if (Build.VERSION.SDK_INT >= 17) {
                location.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos());
                Reflect.on(location).call("makeComplete");
            }
        }
        return location;
    }

    @Override
    public void addGpsStatusListener(IGpsStatusListener listener) {
        synchronized (mGpsStatusListeners) {
            if (!mGpsStatusListeners.contains(listener)) {
                mGpsStatusListeners.add(listener);
            }
        }
    }

    @Override
    public void removeGpsStatusListener(IGpsStatusListener listener) {
        synchronized (mGpsStatusListeners) {
            mGpsStatusListeners.remove(listener);
        }
    }

    //LocationRequest, ListenerTransport
    @Override
    public void addLocationListener(ILocationListener listener) {
        synchronized (mLocationListeners) {
            if (mLocationListeners.contains(listener)) {
                handLocationChanged(true, false);
                return;
            }
            mLocationListeners.add(listener);
        }
        handLocationChanged(true, false);
        startHandleTask();
    }

    @Override
    public void stopAllLocationRequest() {
        synchronized (mGpsStatusListeners) {
            mGpsStatusListeners.clear();
        }
        synchronized (mLocationListeners) {
            mLocationListeners.clear();
        }
    }

    @Override
    public void removeLocationListener(ILocationListener listener) {
        synchronized (mLocationListeners) {
            mLocationListeners.remove(listener);
        }
    }

    private void startHandleTask() {
        synchronized (mLocationListeners) {
            if (mLocationListeners.size() == 0) {
                return;
            }
        }
        mHandler.removeMessages(MSG_HANDLE_LOCATION);
        mHandler.sendEmptyMessageDelayed(MSG_HANDLE_LOCATION, HANDLE_TIME + mRandom.nextInt(1000));
    }

    private void handLocationChanged(boolean start, boolean force) {
//        if (!start) {
//            if (System.currentTimeMillis() - mLastGPS >= HANDLE_TIME_GPS) {
//                reportGps = true;
//                mLastGPS = System.currentTimeMillis();
//            }
//        } else {
//            mLastGPS = System.currentTimeMillis();
//        }
        if(REPORT_LOCATION_WITH_GPS_STATUS) {
            synchronized (mGpsStatusListeners) {
                for (int i = mGpsStatusListeners.size() - 1; i >= 0; i--) {
                    IGpsStatusListener listener = mGpsStatusListeners.get(i);
                    try {
                        if (listener.asBinder().isBinderAlive()
                                && listener.isAlive()) {
                            if (DEBUG)
                                Log.d("tmap", listener.getPackageName() + ":IGpsStatusListener");

                            if (start) {
                                listener.onGpsStarted();
                                GpsStatusGenerate.fakeGpsStatus(listener);
                            }
                        } else {
                            mGpsStatusListeners.remove(i);
                            if (DEBUG)
                                Log.d("tmap", "remove GpsStatusListener:");
                        }
                    } catch (Throwable e) {
                        mGpsStatusListeners.remove(i);
                        //
                    }
                }
            }
        }
//        if (!start) {
//            if (System.currentTimeMillis() - mLastLocation < HANDLE_TIME_LOCATION) {
//                return;
//            }
//            mLastLocation = System.currentTimeMillis();
//        } else {
//            mLastLocation = System.currentTimeMillis();
//        }
        synchronized (mLocationListeners) {
            for (int i = mLocationListeners.size() - 1; i >= 0; i--) {
                ILocationListener listener = mLocationListeners.get(i);
                try {
                    if (listener.asBinder().isBinderAlive() && listener.isAlive()) {
                        if (!start) {
                            Location loc = getVirtualLocation(null, listener.getPackageName(), listener.getUserId());
                            if (DEBUG)
                                Log.d("tmap", listener.getPackageName() + ":onLocationChanged:" + loc);
                            listener.onLocationChanged(loc);
                        } else {
                            listener.onProviderEnabled(LocationManager.GPS_PROVIDER);
                        }
                    } else {
                        mLocationListeners.remove(listener);
                        if (DEBUG)
                            Log.d("tmap", "remove LocationListener:");
                    }
                } catch (Throwable e) {
                    //ignore
                    mLocationListeners.remove(i);
                    if (DEBUG)
                        Log.w("tmap", "onLocationChanged", e);
                }
            }
        }
    }
}
