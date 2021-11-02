package com.graph89.common;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

public class PermissionHelper {
    public final static int MEDIA_PERMISSIONS_REQUEST = 0;
    public final static String[] mediaPermissions = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE
    };

    public static boolean isMediaPermissionsGranted(Context context) {
        for (String perm : mediaPermissions) {
            if (ContextCompat.checkSelfPermission(context, perm) != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    public static void requestMediaPermissions(Activity context, int requestCode) {
        ActivityCompat.requestPermissions(context, mediaPermissions, requestCode);
    }
}
