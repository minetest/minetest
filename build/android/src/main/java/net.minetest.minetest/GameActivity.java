package net.minetest.minetest;

import android.app.NativeActivity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.view.View;

import java.io.File;

public class GameActivity extends NativeActivity {
	private static final int TEXT_DIALOG_REQUEST = 1;


	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		// System UI flags have to be set in onCreate, because
		// onWindowFocusChanged apparently runs after the native
		// code has retreived the window dimensions, and therefore
		// everything on the screen is offset when
		// LAYOUT_HIDE_NAVIGATION is set.
		setImmersive();
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			setImmersive();
	}

	private void setImmersive() {
		if (Build.VERSION.SDK_INT < 16)
			return;

		// Hide status bar and other inessential UI items
		int visibility = View.SYSTEM_UI_FLAG_FULLSCREEN |
			View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
			View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
		if (Build.VERSION.SDK_INT >= 19) {
			// HIDE_NAVIGATION is available since API 14, but the
			// navigation re-appears on any touch events when not
			// paired with IMMERSIVE (API 19), making it essentially
			// useless alone.
			visibility |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
				View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
		}
		getWindow().getDecorView().setSystemUiVisibility(visibility);
	}


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
