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

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import java.util.Objects;

public class InputDialogActivity extends AppCompatActivity {
	private AlertDialog alertDialog;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Bundle b = getIntent().getExtras();
		int editType = Objects.requireNonNull(b).getInt("editType");
		String hint = b.getString("hint");
		String current = b.getString("current");
		final AlertDialog.Builder builder = new AlertDialog.Builder(this);
		EditText editText = new EditText(this);
		builder.setView(editText);
		editText.requestFocus();
		editText.setHint(hint);
		editText.setText(current);
		final InputMethodManager imm = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
		Objects.requireNonNull(imm).toggleSoftInput(InputMethodManager.SHOW_FORCED,
				InputMethodManager.HIDE_IMPLICIT_ONLY);
		if (editType == 3)
			editText.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_VARIATION_PASSWORD);
		else
			editText.setInputType(InputType.TYPE_CLASS_TEXT);
		editText.setOnKeyListener((view, KeyCode, event) -> {
			if (KeyCode == KeyEvent.KEYCODE_ENTER) {
				imm.hideSoftInputFromWindow(editText.getWindowToken(), 0);
				pushResult(editText.getText().toString());
				return true;
			}
			return false;
		});
		alertDialog = builder.create();
		if (!this.isFinishing())
			alertDialog.show();
		alertDialog.setOnCancelListener(dialog -> {
			pushResult(editText.getText().toString());
			setResult(Activity.RESULT_CANCELED);
			alertDialog.dismiss();
			makeFullScreen();
			finish();
		});
	}

	private void pushResult(String text) {
		Intent resultData = new Intent();
		resultData.putExtra("text", text);
		setResult(AppCompatActivity.RESULT_OK, resultData);
		alertDialog.dismiss();
		makeFullScreen();
		finish();
	}

	private void makeFullScreen() {
		if (Build.VERSION.SDK_INT >= 19)
			this.getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
							View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}
}
