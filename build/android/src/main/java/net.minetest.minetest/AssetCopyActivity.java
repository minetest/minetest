package net.minetest.minetest;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Display;
import android.widget.ProgressBar;
import android.widget.TextView;

public class AssetCopyActivity extends Activity
{
	ProgressBar progressBar;
	TextView filenameView;
	AssetCopyTask copyTask;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.asset_copy);

		progressBar = (ProgressBar) findViewById(R.id.progressBar);
		filenameView = (TextView) findViewById(R.id.progressText);

		// Set progress bar to 80% width (can't be done in layout file)
		Display display = getWindowManager().getDefaultDisplay();
		// getWidth is deprecated in favor of getSize, but that isn't available until API level 13
		progressBar.getLayoutParams().width = (int)(display.getWidth() * 0.8);
		progressBar.invalidate();

		// Reuse copy in progress if possible
		AssetCopyActivity prevActivity = (AssetCopyActivity) getLastNonConfigurationInstance();
		if (prevActivity != null) {
			copyTask = prevActivity.copyTask;
			copyTask.setActivity(this);
		} else {
			copyTask = new AssetCopyTask(this);
			copyTask.execute();
		}

		// Request permissions if necessary
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
			requestPermissions();
	}

	@TargetApi(Build.VERSION_CODES.M)
	private void requestPermissions()
	{
		if (checkSelfPermission(Manifest.permission.INTERNET) !=
				PackageManager.PERMISSION_GRANTED) {
			requestPermissions(new String[]{Manifest.permission.INTERNET}, 0);
		}
	}

	/* Preserve asset copy background task to prevent restart of copying
	 * this way of doing it is not recommended for latest android version
	 * but the recommended way isn't available on Android 2.x.
	 */
	@Override
	public Object onRetainNonConfigurationInstance() { return this; }
}
