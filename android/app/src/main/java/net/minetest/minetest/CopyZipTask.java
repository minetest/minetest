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

import android.content.Intent;
import android.os.AsyncTask;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;

public class CopyZipTask extends AsyncTask<String, Void, String> {

	private final WeakReference<AppCompatActivity> activityRef;

	CopyZipTask(AppCompatActivity activity) {
		activityRef = new WeakReference<>(activity);
	}

	protected String doInBackground(String... params) {
		copyAsset(params[0]);
		return params[0];
	}

	@Override
	protected void onPostExecute(String result) {
		startUnzipService(result);
	}

	private void copyAsset(String zipName) {
		String filename = zipName.substring(zipName.lastIndexOf("/") + 1);
		try (InputStream in = activityRef.get().getAssets().open(filename);
		     OutputStream out = new FileOutputStream(zipName)) {
			copyFile(in, out);
		} catch (IOException e) {
			AppCompatActivity activity = activityRef.get();
			if (activity != null) {
				activity.runOnUiThread(() -> Toast.makeText(activityRef.get(), e.getLocalizedMessage(), Toast.LENGTH_LONG).show());
			}
			cancel(true);
		}
	}

	private void copyFile(InputStream in, OutputStream out) throws IOException {
		byte[] buffer = new byte[1024];
		int read;
		while ((read = in.read(buffer)) != -1)
			out.write(buffer, 0, read);
	}

	private void startUnzipService(String file) {
		Intent intent = new Intent(activityRef.get(), UnzipService.class);
		intent.putExtra(UnzipService.EXTRA_KEY_IN_FILE, file);
		AppCompatActivity activity = activityRef.get();
		if (activity != null) {
			activity.startService(intent);
		}
	}
}
