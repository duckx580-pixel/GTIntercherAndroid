package com.rtsoft.growtopia;

import java.util.LinkedList;
import java.util.ListIterator;

/* JADX INFO: loaded from: classes2.dex */
public class SharedMultiTouchInput {
    public static SharedActivity app;
    static LinkedList<TouchInfo> listTouches;

    static class TouchInfo {
        int fingerID;
        public int pointerID;

        TouchInfo() {
        }
    }

    public static void init(SharedActivity sharedActivity) {
        app = sharedActivity;
        listTouches = new LinkedList<>();
    }

    public static int GetNextAvailableFingerID() {
        int i = 0;
        while (i < 12) {
            Boolean bool = true;
            ListIterator<TouchInfo> listIterator = listTouches.listIterator();
            while (true) {
                if (!listIterator.hasNext()) {
                    break;
                }
                if (i == listIterator.next().fingerID) {
                    bool = false;
                    break;
                }
            }
            if (bool.booleanValue()) {
                break;
            }
            i++;
        }
        return i;
    }

    public static int GetFingerByPointerID(int i) {
        ListIterator<TouchInfo> listIterator = listTouches.listIterator();
        while (listIterator.hasNext()) {
            TouchInfo next = listIterator.next();
            if (i == next.pointerID) {
                return next.fingerID;
            }
        }
        TouchInfo touchInfo = new TouchInfo();
        touchInfo.pointerID = i;
        touchInfo.fingerID = GetNextAvailableFingerID();
        listTouches.add(touchInfo);
        return touchInfo.fingerID;
    }

    public static void RemoveFinger(int i) {
        ListIterator<TouchInfo> listIterator = listTouches.listIterator();
        while (listIterator.hasNext()) {
            if (i == listIterator.next().pointerID) {
                listIterator.remove();
                return;
            }
        }
    }

    public static void processMouse(int i, float f, float f2, int i2) {
        int iGetFingerByPointerID = GetFingerByPointerID(i2);
        if (i == 1) {
            RemoveFinger(i2);
        }
        AppGLSurfaceView.nativeOnTouch(i, f, f2, iGetFingerByPointerID);
    }

    // JADX could not decompile this method and left it as a smali dump plus
    // a throw UnsupportedOperationException stub (see git history). That
    // compiles, but crashes the game the moment any multi-touch gesture
    // reaches it -- WrapSharedMultiTouchInput.OnInput calls straight into
    // this. The dumped smali is fully legible register-transfer code
    // though, so reconstructed it directly rather than leaving the stub:
    // ACTION_DOWN/POINTER_DOWN and ACTION_UP/POINTER_UP both resolve to the
    // pointer at getActionIndex(); ACTION_MOVE reports every pointer's
    // current position; ACTION_CANCEL drops all tracked touches. The action
    // codes passed to processMouse (0/1/2) match what
    // AppGLSurfaceView.nativeOnTouch expects for down/up/move.
    public static boolean OnInput(android.view.MotionEvent motionEvent) {
        int actionIndex = motionEvent.getActionIndex();
        switch (motionEvent.getActionMasked()) {
            case android.view.MotionEvent.ACTION_DOWN:
            case android.view.MotionEvent.ACTION_POINTER_DOWN:
                processMouse(0,
                    motionEvent.getX(actionIndex),
                    motionEvent.getY(actionIndex),
                    motionEvent.getPointerId(actionIndex));
                break;
            case android.view.MotionEvent.ACTION_UP:
            case android.view.MotionEvent.ACTION_POINTER_UP:
                processMouse(1,
                    motionEvent.getX(actionIndex),
                    motionEvent.getY(actionIndex),
                    motionEvent.getPointerId(actionIndex));
                break;
            case android.view.MotionEvent.ACTION_MOVE:
                int pointerCount = motionEvent.getPointerCount();
                for (int i = 0; i < pointerCount; i++) {
                    processMouse(2,
                        motionEvent.getX(i),
                        motionEvent.getY(i),
                        motionEvent.getPointerId(i));
                }
                break;
            case android.view.MotionEvent.ACTION_CANCEL:
                listTouches.clear();
                break;
            default:
                break;
        }
        return true;
    }
}
