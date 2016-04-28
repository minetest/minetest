package net.minetest.minetest;

import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;

import java.io.File;

public class GameActivity extends NativeActivity {
	private static final int TEXT_DIALOG_REQUEST = 1;

	public void showDialog(String acceptButton, String hint, String current,
			int editType) {
		Intent intent = new Intent(this, TextEntryActivity.class);
		Bundle params = new Bundle();
		params.putString("acceptButton", acceptButton);
		params.putString("hint", hint);
		params.putString("current", current);
		params.putInt("editType", editType);
		intent.putExtras(params);
		startActivityForResult(intent, TEXT_DIALOG_REQUEST);
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode,
			Intent data) {
		if (requestCode == TEXT_DIALOG_REQUEST && resultCode == RESULT_OK)
			putEditBoxResult(data.getStringExtra("text"));
	}

	public static native void putEditBoxResult(String text);


	private DisplayMetrics getDM() { return getResources().getDisplayMetrics(); }
	public float getDensity() { return getDM().density; }
	public int getDisplayWidth() { return getDM().widthPixels; }
	public int getDisplayHeight() { return getDM().heightPixels; }


	public File getGameUserPath() { return getFilesDir(); }
	public File getGameSharePath() { return getGameSharePath(this); }

	public File getGameCachePath() {
		File external = getExternalCacheDir();
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
			return external;
		if (Environment.getExternalStorageState(external)
				.equals(Environment.MEDIA_MOUNTED))
			return external;
		return getCacheDir();
	}

	static public File getGameSharePath(Context ctx) {
		File external = ctx.getExternalFilesDir(null);
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
			return external;
		if (Environment.getExternalStorageState(external)
				.equals(Environment.MEDIA_MOUNTED))
			return external;
		return ctx.getFilesDir();
	}


	static {
		System.loadLibrary("openal");
		System.loadLibrary("ogg");
		System.loadLibrary("vorbis");
		System.loadLibrary("ssl");
		System.loadLibrary("crypto");
		System.loadLibrary("gmp");
		System.loadLibrary("iconv");

		/* We don't have to load Minetest ourselves, but if we do,
		 * we get nicer logcat errors when loading fails.
		 */
		System.loadLibrary("minetest");
	}
}
