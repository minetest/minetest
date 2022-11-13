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
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

import androidx.annotation.Keep;
import androidx.appcompat.app.AlertDialog;
import androidx.core.content.FileProvider;

import java.io.File;
import java.util.Objects;

// Native code finds these methods by name (see porting_android.cpp).
// This annotation prevents the minifier/Proguard from mangling them.
@Keep
@SuppressWarnings("unused")
public class GameActivity extends NativeActivity {
	static {
		System.loadLibrary("c++_shared");
		System.loadLibrary("Minetest");
	}

	private int messageReturnCode = -1;
	private String messageReturnValue = "";

	public static native void putMessageBoxResult(String text);

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
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

	public void showDialog(String acceptButton, String hint, String current, int editType) {
		runOnUiThread(() -> showDialogUI(hint, current, editType));
	}

	private void showDialogUI(String hint, String current, int editType) {
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		LinearLayout container = new LinearLayout(this);
		container.setOrientation(LinearLayout.VERTICAL);
		builder.setView(container);
		AlertDialog alertDialog = builder.create();
		EditText editText;
		// For multi-line, do not close the dialog after pressing back button
		if (editType == 1) {
			editText = new EditText(this);
		} else {
			editText = new CustomEditText(this);
		}
		container.addView(editText);
		editText.setMaxLines(8);
		editText.requestFocus();
		editText.setHint(hint);
		editText.setText(current);
		final InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
		Objects.requireNonNull(imm).toggleSoftInput(InputMethodManager.SHOW_FORCED,
				InputMethodManager.HIDE_IMPLICIT_ONLY);
		if (editType == 1)
			editText.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_FLAG_MULTI_LINE);
		else if (editType == 3)
			editText.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_VARIATION_PASSWORD);
		else
			editText.setInputType(InputType.TYPE_CLASS_TEXT);
		editText.setSelection(editText.getText().length());
		editText.setOnKeyListener((view, keyCode, event) -> {
			// For multi-line, do not submit the text after pressing Enter key
			if (keyCode == KeyEvent.KEYCODE_ENTER && editType != 1) {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				messageReturnCode = 0;
				messageReturnValue = editText.getText().toString();
				alertDialog.dismiss();
				return true;
			}
			return false;
		});
		// For multi-line, add Done button since Enter key does not submit text
		if (editType == 1) {
			Button doneButton = new Button(this);
			container.addView(doneButton);
			doneButton.setText(R.string.ime_dialog_done);
			doneButton.setOnClickListener((view -> {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				messageReturnCode = 0;
				messageReturnValue = editText.getText().toString();
				alertDialog.dismiss();
			}));
		}
		alertDialog.show();
		alertDialog.setOnCancelListener(dialog -> {
			getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
			messageReturnValue = current;
			messageReturnCode = -1;
		});
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

	public void openURI(String uri) {
		Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(uri));
		startActivity(browserIntent);
	}

	public String getUserDataPath() {
		return Objects.requireNonNull(
			Utils.getUserDataDirectory(this),
			"Cannot get user data directory"
		).getAbsolutePath();
	}

	public String getCachePath() {
		return Objects.requireNonNull(
			Utils.getCacheDirectory(this),
			"Cannot get cache directory"
		).getAbsolutePath();
	}

	public void shareFile(String path) {
		File file = new File(path);
		if (!file.exists()) {
			Log.e("GameActivity", "File " + file.getAbsolutePath() + " doesn't exist");
			return;
		}

		Uri fileUri = FileProvider.getUriForFile(this, "net.minetest.minetest.fileprovider", file);

		Intent intent = new Intent(Intent.ACTION_SEND, fileUri);
		intent.setDataAndType(fileUri, getContentResolver().getType(fileUri));
		intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
		intent.putExtra(Intent.EXTRA_STREAM, fileUri);

		Intent shareIntent = Intent.createChooser(intent, null);
		startActivity(shareIntent);
	}
}
