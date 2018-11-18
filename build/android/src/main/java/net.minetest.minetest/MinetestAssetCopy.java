package net.minetest.minetest;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Iterator;
import java.util.Vector;

public class MinetestAssetCopy extends Activity {
	ProgressBar m_ProgressBar;
	TextView m_Filename;
	copyAssetTask m_AssetCopy;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.assetcopy);
		m_ProgressBar = findViewById(R.id.progressBar1);
		m_Filename = findViewById(R.id.textView1);
		Display display = getWindowManager().getDefaultDisplay();
		m_ProgressBar.getLayoutParams().width = (int) (display.getWidth() * 0.8);
		m_ProgressBar.invalidate();

		/* check if there's already a copy in progress and reuse in case it is*/
		MinetestAssetCopy prevActivity =
				(MinetestAssetCopy) getLastNonConfigurationInstance();
		if (prevActivity != null) {
			m_AssetCopy = prevActivity.m_AssetCopy;
		} else {
			m_AssetCopy = new copyAssetTask();
			m_AssetCopy.execute();
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		makeFullScreen();
	}

	public void makeFullScreen() {
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			this.getWindow().getDecorView().setSystemUiVisibility(
					View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
			);
		}
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			makeFullScreen();
		}
	}

	/* preserve asset copy background task to prevent restart of copying */
	/* this way of doing it is not recommended for latest android version */
	/* but the recommended way isn't available on android 2.x */
	public Object onRetainNonConfigurationInstance() {
		return this;
	}

	private class copyAssetTask extends AsyncTask<String, Integer, String> {
		boolean m_copy_started = false;
		String m_Foldername = "media";
		Vector<String> m_foldernames;
		Vector<String> m_filenames;
		Vector<String> m_tocopy;
		Vector<String> m_asset_size_unknown;

		private long getFullSize(String filename) {
			long size = 0;
			try {
				InputStream src = getAssets().open(filename);
				byte[] buf = new byte[4096];

				int len = 0;
				while ((len = src.read(buf)) > 0) {
					size += len;
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
			return size;
		}

		@Override
		protected String doInBackground(String... files) {
			m_foldernames = new Vector<String>();
			m_filenames = new Vector<String>();
			m_tocopy = new Vector<String>();
			m_asset_size_unknown = new Vector<String>();
			String baseDir =
					Environment.getExternalStorageDirectory().getAbsolutePath()
							+ "/";


			// prepare temp folder
			File TempFolder = new File(baseDir + "Minetest/tmp/");

			if (!TempFolder.exists()) {
				TempFolder.mkdir();
			} else {
				File[] todel = TempFolder.listFiles();

				for (int i = 0; i < todel.length; i++) {
					Log.v("MinetestAssetCopy", "deleting: " + todel[i].getAbsolutePath());
					todel[i].delete();
				}
			}

			// add a .nomedia file
			try {
				OutputStream dst = new FileOutputStream(baseDir + "Minetest/.nomedia");
				dst.close();
			} catch (IOException e) {
				Log.e("MinetestAssetCopy", "Failed to create .nomedia file");
				e.printStackTrace();
			}


			// build lists from prepared data
			BuildFolderList();
			BuildFileList();

			// scan filelist
			ProcessFileList();

			// doing work
			m_copy_started = true;
			m_ProgressBar.setMax(m_tocopy.size());

			for (int i = 0; i < m_tocopy.size(); i++) {
				try {
					String filename = m_tocopy.get(i);
					publishProgress(i);

					boolean asset_size_unknown = false;
					long filesize = -1;

					if (m_asset_size_unknown.contains(filename)) {
						File testme = new File(baseDir + "/" + filename);

						if (testme.exists()) {
							filesize = testme.length();
						}
						asset_size_unknown = true;
					}

					InputStream src;
					try {
						src = getAssets().open(filename);
					} catch (IOException e) {
						Log.e("MinetestAssetCopy", "Copying file: " + filename + " FAILED (not in assets)");
						e.printStackTrace();
						continue;
					}

					// Transfer bytes from in to out
					byte[] buf = new byte[1024];
					int len = src.read(buf, 0, 1024);

					/* following handling is crazy but we need to deal with    */
					/* compressed assets.Flash chips limited livetime due to   */
					/* write operations, we can't allow large files to destroy */
					/* users flash.                                            */
					if (asset_size_unknown) {
						if ((len > 0) && (len < buf.length) && (len == filesize)) {
							src.close();
							continue;
						}

						if (len == buf.length) {
							src.close();
							long size = getFullSize(filename);
							if (size == filesize) {
								continue;
							}
							src = getAssets().open(filename);
							len = src.read(buf, 0, 1024);
						}
					}
					if (len > 0) {
						int total_filesize = 0;
						OutputStream dst;
						try {
							dst = new FileOutputStream(baseDir + "/" + filename);
						} catch (IOException e) {
							Log.e("MinetestAssetCopy", "Copying file: " + baseDir +
									"/" + filename + " FAILED (couldn't open output file)");
							e.printStackTrace();
							src.close();
							continue;
						}
						dst.write(buf, 0, len);
						total_filesize += len;

						while ((len = src.read(buf)) > 0) {
							dst.write(buf, 0, len);
							total_filesize += len;
						}

						dst.close();
						Log.v("MinetestAssetCopy", "Copied file: " +
								m_tocopy.get(i) + " (" + total_filesize +
								" bytes)");
					} else if (len < 0) {
						Log.e("MinetestAssetCopy", "Copying file: " +
								m_tocopy.get(i) + " failed, size < 0");
					}
					src.close();
				} catch (IOException e) {
					Log.e("MinetestAssetCopy", "Copying file: " +
							m_tocopy.get(i) + " failed");
					e.printStackTrace();
				}
			}
			return "";
		}

		/**
		 * update progress bar
		 */
		protected void onProgressUpdate(Integer... progress) {

			if (m_copy_started) {
				boolean shortened = false;
				String todisplay = m_tocopy.get(progress[0]);
				m_ProgressBar.setProgress(progress[0]);
				m_Filename.setText(todisplay);
			} else {
				boolean shortened = false;
				String todisplay = m_Foldername;
				String full_text = "scanning " + todisplay + " ...";
				m_Filename.setText(full_text);
			}
		}

		/**
		 * check all files and folders in filelist
		 */
		protected void ProcessFileList() {
			String FlashBaseDir =
					Environment.getExternalStorageDirectory().getAbsolutePath();

			Iterator itr = m_filenames.iterator();

			while (itr.hasNext()) {
				String current_path = (String) itr.next();
				String FlashPath = FlashBaseDir + "/" + current_path;

				if (isAssetFolder(current_path)) {
					/* store information and update gui */
					m_Foldername = current_path;
					publishProgress(0);

					/* open file in order to check if it's a folder */
					File current_folder = new File(FlashPath);
					if (!current_folder.exists()) {
						if (!current_folder.mkdirs()) {
							Log.e("MinetestAssetCopy", "\t failed create folder: " +
									FlashPath);
						} else {
							Log.v("MinetestAssetCopy", "\t created folder: " +
									FlashPath);
						}
					}

					continue;
				}

				/* if it's not a folder it's most likely a file */
				boolean refresh = true;

				File testme = new File(FlashPath);

				long asset_filesize = -1;
				long stored_filesize = -1;

				if (testme.exists()) {
					try {
						AssetFileDescriptor fd = getAssets().openFd(current_path);
						asset_filesize = fd.getLength();
						fd.close();
					} catch (IOException e) {
						refresh = true;
						m_asset_size_unknown.add(current_path);
						Log.e("MinetestAssetCopy", "Failed to open asset file \"" +
								FlashPath + "\" for size check");
					}

					stored_filesize = testme.length();

					if (asset_filesize == stored_filesize) {
						refresh = false;
					}

				}

				if (refresh) {
					m_tocopy.add(current_path);
				}
			}
		}

		/**
		 * read list of folders prepared on package build
		 */
		protected void BuildFolderList() {
			try {
				InputStream is = getAssets().open("index.txt");
				BufferedReader reader = new BufferedReader(new InputStreamReader(is));

				String line = reader.readLine();
				while (line != null) {
					m_foldernames.add(line);
					line = reader.readLine();
				}
				is.close();
			} catch (IOException e1) {
				Log.e("MinetestAssetCopy", "Error on processing index.txt");
				e1.printStackTrace();
			}
		}

		/**
		 * read list of asset files prepared on package build
		 */
		protected void BuildFileList() {
			long entrycount = 0;
			try {
				InputStream is = getAssets().open("filelist.txt");
				BufferedReader reader = new BufferedReader(new InputStreamReader(is));

				String line = reader.readLine();
				while (line != null) {
					m_filenames.add(line);
					line = reader.readLine();
					entrycount++;
				}
				is.close();
			} catch (IOException e1) {
				Log.e("MinetestAssetCopy", "Error on processing filelist.txt");
				e1.printStackTrace();
			}
		}

		protected void onPostExecute(String result) {
			finish();
		}

		protected boolean isAssetFolder(String path) {
			return m_foldernames.contains(path);
		}
	}
}
