package net.minetest.minetest;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import java.io.File;
import java.util.Objects;

public class Utils {
	@NonNull
	public static File createDirs(@NonNull File root, @NonNull String dir) {
		File f = new File(root, dir);
		if (!f.isDirectory())
			if (!f.mkdirs())
				Log.e("Utils", "Directory " + dir + " cannot be created");

		return f;
	}

	@NonNull
	public static File getUserDataDirectory(@NonNull Context context) {
		File extDir = Objects.requireNonNull(
			context.getExternalFilesDir(null),
			"Cannot get external file directory"
		);
		return createDirs(extDir, "Minetest");
	}

	@NonNull
	public static File getCacheDirectory(@NonNull Context context) {
		return Objects.requireNonNull(
			context.getCacheDir(),
			"Cannot get cache directory"
		);
	}

	public static boolean isInstallValid(@NonNull Context context) {
		File userDataDirectory = getUserDataDirectory(context);
		return userDataDirectory.isDirectory() &&
			new File(userDataDirectory, "games").isDirectory() &&
			new File(userDataDirectory, "builtin").isDirectory() &&
			new File(userDataDirectory, "client").isDirectory() &&
			new File(userDataDirectory, "textures").isDirectory();
	}
}
