/*
Minetest
Copyright (C) 2014-2020 MoNTE48, Maksim Gamarnik <MoNTE48@mail.ua>
Copyright (C) 2014-2020 ubulem,  Bektur Mambetov <berkut87@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

package net.minetest.minetest;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;

public class GameActivity extends NativeActivity {
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("Minetest");
	}

	private int messageReturnCode;
	private String messageReturnValue;

	public static native void putMessageBoxResult(String text);

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		messageReturnCode = -1;
		messageReturnValue = "";
	}

	private void makeFullScreen() {
		if (Build.VERSION.SDK_INT >= 19)
			this.getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
					View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
					View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			makeFullScreen();
	}

	@Override
	protected void onResume() {
		super.onResume();
		makeFullScreen();
	}

	@Override
	public void onBackPressed() {
		// Ignore the back press so Minetest can handle it
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (requestCode == 101) {
			if (resultCode == RESULT_OK) {
				String text = data.getStringExtra("text");
				messageReturnCode = 0;
				messageReturnValue = text;
			} else
				messageReturnCode = 1;
		}
	}

	public void showDialog(String acceptButton, String hint, String current, int editType) {
		Intent intent = new Intent(this, InputDialogActivity.class);
		Bundle params = new Bundle();
		params.putString("acceptButton", acceptButton);
		params.putString("hint", hint);
		params.putString("current", current);
		params.putInt("editType", editType);
		intent.putExtras(params);
		startActivityForResult(intent, 101);
		messageReturnValue = "";
		messageReturnCode = -1;
	}

	public int getDialogState() {
		return messageReturnCode;
	}

	public String getDialogValue() {
		messageReturnCode = -1;
		return messageReturnValue;
	}

	public float getDensity() {
		return getResources().getDisplayMetrics().density;
	}

	public int getDisplayHeight() {
		return getResources().getDisplayMetrics().heightPixels;
	}

	public int getDisplayWidth() {
		return getResources().getDisplayMetrics().widthPixels;
	}

	public void openURL(String url) {
		Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
		startActivity(browserIntent);
	}
}
