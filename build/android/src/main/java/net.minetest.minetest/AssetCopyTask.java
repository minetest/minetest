package net.minetest.minetest;

import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Vector;

class AssetCopyTask extends AsyncTask<String, Integer, String> {
	private static final String TAG = "AssetCopyTask";
	private static final String gameAssetPrefix = "files" + File.separator;

	private File basePath;
	private AssetCopyActivity activity;

	private boolean copyStarted = false;
	private String folderName = "media";

	private Vector<String> folderNames = new Vector<>();
	private Vector<String> fileNames = new Vector<>();
	private Vector<String> toCopy = new Vector<>();
	private Vector<String> assetSizeUnknown = new Vector<>();

	public AssetCopyTask(AssetCopyActivity act) {
		activity = act;
		basePath = GameActivity.getGameSharePath(activity);
	}

	public void setActivity(AssetCopyActivity act) { activity = act; }

	private long getFullSize(InputStream stream) {
		try {
			byte[] buf = new byte[8192];
			long size = 0;
			int len;
			while ((len = stream.read(buf)) > 0) {
				size += len;
			}
			return size;
		} catch (IOException e) {
			Log.e(TAG, "Retrieving full file size failed.", e);
			return -1;
		}
	}

	@Override
	protected String doInBackground(String... files) {
		// This won't work on KitKat and above, since we don't request
		// the permission on later versions.
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
			migrateBasePath();

		// Prepare temp folder
		File tempFolder = new File(activity.getCacheDir(), "tmp");
		if (!tempFolder.exists()) {
			if (!tempFolder.mkdir()) {
				Log.e(TAG, "Failed to tmp folder " +
					tempFolder.getAbsolutePath());
			}
		} else {
			for (File f : tempFolder.listFiles()) {
				Log.v(TAG, "Deleting: " + f.getAbsolutePath());
				if (!f.delete()) {
					Log.e(TAG, "Failed to delete " + f.getAbsolutePath());
				}
			}
		}

		// Build lists from prepared data
		buildFolderList();
		buildFileList();

		// Scan file list
		processFileList();

		copyStarted = true;

		for (int i = 0; i < toCopy.size(); ++i) {
			try {
				publishProgress(i);
				copyFile(toCopy.get(i));
			} catch (IOException e) {
				Log.e(TAG, "Copying file: " +
					toCopy.get(i) + " failed.", e);
			}
		}

		activity.startActivity(new Intent(activity, GameActivity.class));

		return "";
	}

	private void migrateBasePath() {
		File oldBasePath = new File(Environment.getExternalStorageDirectory(), "Minetest");
		if (!oldBasePath.exists())
			return;
		if (oldBasePath.renameTo(basePath)) {
			Log.i(TAG, "Migrated old base path.");
			return;
		}
		Log.e(TAG, "Failed to migrate old base path from " +
			oldBasePath.getAbsolutePath() + " to " +
			basePath.getAbsolutePath() + " deleting it instead.");
		if (!oldBasePath.delete()) {
			Log.e(TAG, "Failed to delete old base path, leaving it.");
		}
	}

	private void copyFile(String filename) throws IOException {
		boolean asset_size_unknown = assetSizeUnknown.contains(filename);
		long file_size = -1;

		if (asset_size_unknown) {
			File test = new File(basePath, filename);
			if (test.exists())
				file_size = test.length();
		}

		InputStream src;
		try {
			src = activity.getAssets().open(gameAssetPrefix + filename);
		} catch (IOException e) {
			Log.e(TAG, "Failed to copy file (not in assets): " +
				filename, e);
			return;
		}

		long asset_size;

		if (asset_size_unknown) {
			asset_size = getFullSize(src);
			if (asset_size == file_size) {
				src.close();
				return;
			}
			src.reset();
		}

		OutputStream dst;
		File dstFile = new File(basePath, filename);
		try {
			dst = new FileOutputStream(dstFile);
		} catch (IOException e) {
			Log.e(TAG, "Failed to copy file (couldn't open output file): " +
				dstFile.getAbsolutePath(), e);
			src.close();
			return;
		}

		byte[] buf = new byte[8192];
		int len;
		long total_filesize = 0;
		while ((len = src.read(buf)) > 0) {
			dst.write(buf, 0, len);
			total_filesize += len;
		}

		dst.close();
		src.close();
		Log.v(TAG, "Copied file: " + filename +
			" (" + total_filesize + " bytes).");
	}

	/// Update progress bar
	@Override
	protected void onProgressUpdate(Integer... progress) {
		if (copyStarted) {
			String filename = toCopy.get(progress[0]);
			int text_res = R.string.copying;
			if (assetSizeUnknown.contains(filename))
				text_res = R.string.checking;

			activity.filenameView.setText(String.format(
					activity.getString(text_res),
					filename));

			activity.progressBar.setMax(toCopy.size());
			activity.progressBar.setProgress(progress[0]);
		} else {
			activity.filenameView.setText(String.format(
					activity.getString(R.string.scanning),
					folderName));
		}
	}

	/// Check all files and folders in file list
	protected void processFileList() {
		for (String name : fileNames) {
			processListFile(name);
		}
	}

	/** Check if a file is up-to-date on the storage.
	 * This uses a simple file size check to determine if the file is valid.
	 * This check can fail if the asset is compressed, in which case this
	 * function will add the file to both the `toCopy` and `assetSizeUnknown`
	 * vectors, indicating that a more comprehensive check must be done.
	 */
	private void processListFile(String filename) {
		File path = new File(basePath, filename);

		if (folderNames.contains(filename)) {
			// Store information and update GUI
			folderName = filename;
			publishProgress(0);

			if (!path.exists()) {
				if (path.mkdirs()) {
					Log.v(TAG, "Created folder: " +
						path.getAbsolutePath());
				} else {
					Log.e(TAG, "Failed to create folder: " +
						path.getAbsolutePath());
				}
			}

			return;
		}

		// If it's not a folder it's most likely a file
		boolean refresh = true;

		if (path.exists()) {
			try {
				AssetFileDescriptor fd = activity.getAssets()
					.openFd(gameAssetPrefix + filename);
				if (fd.getLength() == path.length())
					refresh = false;
				fd.close();
			} catch (IOException e) {
				// We probably can't open it as an FD because it's compressed
				refresh = true;
				assetSizeUnknown.add(filename);
				Log.d(TAG, "Failed to open asset file \"" +
					path + "\" for size check, proceeding " +
					"with unknown size.");
			}
		}

		if (refresh) {
			toCopy.add(filename);
		}
	}

	/// Read list of folders prepared on package build
	protected void buildFolderList() {
		try {
			InputStream is = activity.getAssets().open("index.txt");
			BufferedReader reader = new BufferedReader(new InputStreamReader(is));

			String line;
			while ((line = reader.readLine()) != null) {
				folderNames.add(line);
			}
			is.close();
		} catch (IOException e) {
			Log.e(TAG, "Error processing index.txt.", e);
		}
	}

	/// Read list of asset files prepared on package build
	protected void buildFileList() {
		try {
			InputStream is = activity.getAssets().open("filelist.txt");
			BufferedReader reader = new BufferedReader(new InputStreamReader(is));

			String line;
			while ((line = reader.readLine()) != null) {
				fileNames.add(line);
			}
			is.close();
		} catch (IOException e) {
			Log.e(TAG, "Error processing filelist.txt.", e);
		}
	}

	@Override
	protected void onPostExecute (String result) { activity.finish(); }
}
