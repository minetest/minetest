package net.minetest.minetest;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.File;

public class Utils {
	public static @NonNull File createDirs(File root, String dir) {
		File f = new File(root, dir);
		if (!f.isDirectory())
			if (!f.mkdirs())
				Log.e("Utils", "Directory " + dir + " cannot be created");

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

	public static boolean isInstallValid(Context context) {
		File userDataDirectory = getUserDataDirectory(context);
		return userDataDirectory != null && userDataDirectory.isDirectory() &&
			new File(userDataDirectory, "games").isDirectory() &&
			new File(userDataDirectory, "builtin").isDirectory() &&
			new File(userDataDirectory, "client").isDirectory() &&
			new File(userDataDirectory, "textures").isDirectory();
	}
}
