package org.minetest.minetest;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Vector;

import android.app.Activity;
import android.content.res.AssetFileDescriptor;

import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.Display;
import android.widget.ProgressBar;
import android.widget.TextView;

public class MinetestAssetCopy extends Activity {
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.assetcopy);
		
		m_ProgressBar = (ProgressBar) findViewById(R.id.progressBar1);
		m_Filename = (TextView) findViewById(R.id.textView1);
		
		Display display = getWindowManager().getDefaultDisplay();
		m_ProgressBar.getLayoutParams().width = (int) (display.getWidth() * 0.8);
		m_ProgressBar.invalidate();
		
		m_AssetCopy = new copyAssetTask();
		m_AssetCopy.execute();
	}
	
	ProgressBar m_ProgressBar;
	TextView m_Filename;
	
	copyAssetTask m_AssetCopy;
	
	private class copyAssetTask extends AsyncTask<String, Integer, String>{
		
		private void copyElement(String name, String path) {
			String baseDir = Environment.getExternalStorageDirectory().getAbsolutePath();
			String full_path;
			if (path != "") {
				full_path = path + "/" + name;
			}
			else {
				full_path = name;
			}
			//is a folder read asset list
			if (m_foldernames.contains(full_path)) {
				m_Foldername = full_path;
				publishProgress(0);
				File current_folder = new File(baseDir + "/" + full_path);
				if (!current_folder.exists()) {
					if (!current_folder.mkdirs()) {
						Log.w("MinetestAssetCopy","\t failed create folder: " + baseDir + "/" + full_path);
					}
					else {
						Log.w("MinetestAssetCopy","\t created folder: " + baseDir + "/" + full_path);
					}
				}
				try {
					String[] current_assets = getAssets().list(full_path);
					for(int i=0; i < current_assets.length; i++) {
						copyElement(current_assets[i],full_path);
					}
				} catch (IOException e) {
					Log.w("MinetestAssetCopy","\t failed to read contents of folder");
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			//is a file just copy
			else {
				boolean refresh = true;
				
				File testme = new File(baseDir + "/" + full_path);
				
				long asset_filesize = -1;
				long stored_filesize = -1;
				
				if (testme.exists()) {
					try {
						AssetFileDescriptor fd = getAssets().openFd(full_path);
						asset_filesize = fd.getLength();
						fd.close();
					} catch (IOException e) {
						refresh = true;
						m_asset_size_unknown.add(full_path);
					}
					
					stored_filesize = testme.length();
					
					if (asset_filesize == stored_filesize) {
						refresh = false;
					}
					
				}
				
				if (refresh) {
					m_tocopy.add(full_path);
				}
			}
		}
		
		private long getFullSize(String filename) {
			long size = 0;
			try {
			InputStream src = getAssets().open(filename);
			byte[] buf = new byte[1024];
			
			int len = 0;
			while ((len = src.read(buf)) > 0) {
				size += len;
			}
			}
			catch (IOException e) {
				e.printStackTrace();
			}
			return size;
		}

		@Override
		protected String doInBackground(String... files) {
			
			m_foldernames  = new Vector<String>();
			m_tocopy       = new Vector<String>();
			m_asset_size_unknown = new Vector<String>();
			String baseDir = Environment.getExternalStorageDirectory().getAbsolutePath() + "/";
			
			File TempFolder = new File(baseDir + "Minetest/tmp/");
			
			if (!TempFolder.exists()) {
				TempFolder.mkdir();
			}
			else {
				File[] todel = TempFolder.listFiles();
				
				for(int i=0; i < todel.length; i++) {
					Log.w("MinetestAssetCopy","deleting: " + todel[i].getAbsolutePath());
					todel[i].delete();
				}
			}
			
			// add a .nomedia file
			try {
				OutputStream dst = new FileOutputStream(baseDir + "Minetest/.nomedia");
				dst.close();
			} catch (IOException e) {
				Log.w("MinetestAssetCopy","Failed to create .nomedia file");
				e.printStackTrace();
			}
			
			try {
				InputStream is = getAssets().open("index.txt");
				BufferedReader reader = new BufferedReader(new InputStreamReader(is));
		
				String line = reader.readLine();
				while(line != null){
					m_foldernames.add(line);
					line = reader.readLine();
				}
			} catch (IOException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			
			copyElement("Minetest","");
			
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
						
						if(testme.exists()) {
							filesize = testme.length();
						}
						asset_size_unknown = true;
					}
					
					InputStream src;
					try {
						src = getAssets().open(filename);
					} catch (IOException e) {
						Log.w("MinetestAssetCopy","Copying file: " + filename + " FAILED (not in assets)");
						// TODO Auto-generated catch block
						e.printStackTrace();
						continue;
					}
					
					// Transfer bytes from in to out
					byte[] buf = new byte[1*1024];
					int len = src.read(buf, 0, 1024);
					
					/* following handling is crazy but we need to deal with    */
					/* compressed assets.Flash chips limited livetime sue to   */
					/* write operations, we can't allow large files to destroy */
					/* users flash.                                            */
					if (asset_size_unknown) {
						if ( (len > 0) && (len < buf.length) && (len == filesize)) {
							src.close();
							continue;
						}
						
						if (len == buf.length) {
							src.close();
							long size = getFullSize(filename);
							if ( size == filesize) {
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
							Log.w("MinetestAssetCopy","Copying file: " + baseDir + 
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
						Log.w("MinetestAssetCopy","Copied file: " + m_tocopy.get(i) + " (" + total_filesize + " bytes)");
					}
					else if (len < 0) {
						Log.w("MinetestAssetCopy","Copying file: " + m_tocopy.get(i) + " failed, size < 0");
					}
					src.close();
				} catch (IOException e) {
					Log.w("MinetestAssetCopy","Copying file: " + m_tocopy.get(i) + " failed");
					e.printStackTrace();
				}
			}
			
			return "";
		}
		
		protected void onProgressUpdate(Integer... progress) {
			if (m_copy_started) {
				m_ProgressBar.setProgress(progress[0]);
				m_Filename.setText(m_tocopy.get(progress[0]));
			}
			else {
				m_Filename.setText("scanning " + m_Foldername + " ...");
			}
		}
		
		protected void onPostExecute (String result) {
			finish();
		}
		boolean m_copy_started = false;
		String m_Foldername = "media";
		Vector<String> m_foldernames;
		Vector<String> m_tocopy;
		Vector<String> m_asset_size_unknown;
	}
}
