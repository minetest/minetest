package net.minetest.minetest;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;

public class TextEntryActivity extends Activity {
	public AlertDialog inputDialog;
	public EditText inputWidget;

	private final int INPUT_MULTILINE = 0;
	private final int INPUT_SINGLELINE = 1;
	private final int INPUT_PASSWORD = 2;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Bundle b = getIntent().getExtras();
		String acceptButton = b.getString("acceptButton");
		String hint = b.getString("hint");
		String current = b.getString("current");
		int editType = b.getInt("editType");

		inputWidget = new EditText(this);
		inputWidget.setHint(hint);
		inputWidget.setText(current);
		inputWidget.setMinWidth(300);
		inputWidget.setBackgroundColor(0x000000);
		if (editType == INPUT_PASSWORD) {
			inputWidget.setInputType(InputType.TYPE_CLASS_TEXT |
				InputType.TYPE_TEXT_VARIATION_PASSWORD);
		} else {
			inputWidget.setInputType(InputType.TYPE_CLASS_TEXT);
		}

		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setView(inputWidget);

		if (editType == INPUT_MULTILINE) {
			builder.setPositiveButton(acceptButton, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton)  {
					pushResult(inputWidget.getText().toString());
				}
			});
		} else {
			inputWidget.setOnKeyListener(new OnKeyListener() {
				public boolean onKey(View view, int KeyCode, KeyEvent event) {
					if (KeyCode == KeyEvent.KEYCODE_ENTER) {
						pushResult(inputWidget.getText().toString());
						return true;
					}
					return false;
				}
			});
		}

		builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
			public void onCancel(DialogInterface dialog) {
				cancelDialog();
			}
		});

		inputDialog = builder.create();
		inputDialog.show();
	}

	public void pushResult(String text) {
		Intent resultData = new Intent();
		resultData.putExtra("text", text);
		setResult(Activity.RESULT_OK, resultData);
		inputDialog.dismiss();
		finish();
	}

	public void cancelDialog() {
		setResult(Activity.RESULT_CANCELED);
		inputDialog.dismiss();
		finish();
	}
}
