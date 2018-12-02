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

public class MinetestTextEntry extends Activity {
	private final int MultiLineTextInput = 1;
	private final int SingleLineTextInput = 2;
	private final int SingleLinePasswordInput = 3;
	public AlertDialog mTextInputDialog;
	public EditText mTextInputWidget;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Bundle b = getIntent().getExtras();
		String acceptButton = b.getString("EnterButton");
		String hint = b.getString("hint");
		String current = b.getString("current");
		int editType = b.getInt("editType");

		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		mTextInputWidget = new EditText(this);
		mTextInputWidget.setHint(hint);
		mTextInputWidget.setText(current);
		mTextInputWidget.setMinWidth(300);
		if (editType == SingleLinePasswordInput) {
			mTextInputWidget.setInputType(InputType.TYPE_CLASS_TEXT |
					InputType.TYPE_TEXT_VARIATION_PASSWORD);
		} else {
			mTextInputWidget.setInputType(InputType.TYPE_CLASS_TEXT);
		}

		builder.setView(mTextInputWidget);

		if (editType == MultiLineTextInput) {
			builder.setPositiveButton(acceptButton, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					pushResult(mTextInputWidget.getText().toString());
				}
			});
		}

		builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
			public void onCancel(DialogInterface dialog) {
				cancelDialog();
			}
		});

		mTextInputWidget.setOnKeyListener(new OnKeyListener() {
			@Override
			public boolean onKey(View view, int KeyCode, KeyEvent event) {
				if (KeyCode == KeyEvent.KEYCODE_ENTER) {

					pushResult(mTextInputWidget.getText().toString());
					return true;
				}
				return false;
			}
		});

		mTextInputDialog = builder.create();
		mTextInputDialog.show();
	}

	public void pushResult(String text) {
		Intent resultData = new Intent();
		resultData.putExtra("text", text);
		setResult(Activity.RESULT_OK, resultData);
		mTextInputDialog.dismiss();
		finish();
	}

	public void cancelDialog() {
		setResult(Activity.RESULT_CANCELED);
		mTextInputDialog.dismiss();
		finish();
	}
}
