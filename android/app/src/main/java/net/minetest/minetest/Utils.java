package net.minetest.minetest;

import android.content.Context;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class Utils {
	public static File createDirs(File root, String dir) {
		File f = new File(root, dir);
		if (!f.isDirectory())
			f.mkdirs();

		return f;
	}

	public static @Nullable File getUserDataDirectory(Context context) {
		File extDir = context.getExternalFilesDir(null);
		if (extDir == null) {
			return null;
		}

		return createDirs(extDir, "Minetest");
	}

	public static @Nullable File getCacheDirectory(Context context) {
		return context.getCacheDir();
	}
}
