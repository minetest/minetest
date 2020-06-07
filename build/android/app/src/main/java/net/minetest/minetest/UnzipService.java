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

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Environment;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

public class UnzipService extends IntentService {
	public static final String ACTION_UPDATE = "net.minetest.minetest.UPDATE";
	public static final String ACTION_PROGRESS = "net.minetest.minetest.PROGRESS";
	public static final String ACTION_FAILURE = "net.minetest.minetest.FAILURE";
	public static final String EXTRA_KEY_IN_FILE = "file";
	public static final int SUCCESS = -1;
	public static final int FAILURE = -2;
	private final int id = 1;
	private NotificationManager mNotifyManager;
	private boolean isSuccess = true;
	private String failureMessage;

	public UnzipService() {
		super("net.minetest.minetest.UnzipService");
	}

	private void isDir(String dir, String location) {
		File f = new File(location, dir);
		if (!f.isDirectory())
			f.mkdirs();
	}

	@Override
	protected void onHandleIntent(Intent intent) {
		createNotification();
		unzip(intent);
	}

	private void createNotification() {
		String name = "net.minetest.minetest";
		String channelId = "Minetest channel";
		String description = "notifications from Minetest";
		Notification.Builder builder;
		if (mNotifyManager == null)
			mNotifyManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
			int importance = NotificationManager.IMPORTANCE_LOW;
			NotificationChannel mChannel = null;
			if (mNotifyManager != null)
				mChannel = mNotifyManager.getNotificationChannel(channelId);
			if (mChannel == null) {
				mChannel = new NotificationChannel(channelId, name, importance);
				mChannel.setDescription(description);
				// Configure the notification channel, NO SOUND
				mChannel.setSound(null, null);
				mChannel.enableLights(false);
				mChannel.enableVibration(false);
				mNotifyManager.createNotificationChannel(mChannel);
			}
			builder = new Notification.Builder(this, channelId);
		} else {
			builder = new Notification.Builder(this);
		}
		builder.setContentTitle(getString(R.string.notification_title))
				.setSmallIcon(R.mipmap.ic_launcher)
				.setContentText(getString(R.string.notification_description));
		mNotifyManager.notify(id, builder.build());
	}

	private void unzip(Intent intent) {
		String zip = intent.getStringExtra(EXTRA_KEY_IN_FILE);
		isDir("Minetest", Environment.getExternalStorageDirectory().toString());
		String location = Environment.getExternalStorageDirectory() + File.separator + "Minetest" + File.separator;
		int per = 0;
		int size = getSummarySize(zip);
		File zipFile = new File(zip);
		int readLen;
		byte[] readBuffer = new byte[8192];
		try (FileInputStream fileInputStream = new FileInputStream(zipFile);
		     ZipInputStream zipInputStream = new ZipInputStream(fileInputStream)) {
			ZipEntry ze;
			while ((ze = zipInputStream.getNextEntry()) != null) {
				if (ze.isDirectory()) {
					++per;
					isDir(ze.getName(), location);
				} else {
					publishProgress(100 * ++per / size);
					try (OutputStream outputStream = new FileOutputStream(location + ze.getName())) {
						while ((readLen = zipInputStream.read(readBuffer)) != -1) {
							outputStream.write(readBuffer, 0, readLen);
						}
					}
				}
				zipFile.delete();
			}
		} catch (IOException e) {
			isSuccess = false;
			failureMessage = e.getLocalizedMessage();
		}
	}

	private void publishProgress(int progress) {
		Intent intentUpdate = new Intent(ACTION_UPDATE);
		intentUpdate.putExtra(ACTION_PROGRESS, progress);
		if (!isSuccess) intentUpdate.putExtra(ACTION_FAILURE, failureMessage);
		sendBroadcast(intentUpdate);
	}

	private int getSummarySize(String zip) {
		int size = 0;
		try {
			ZipFile zipSize = new ZipFile(zip);
			size += zipSize.size();
		} catch (IOException e) {
			Toast.makeText(this, e.getLocalizedMessage(), Toast.LENGTH_LONG).show();
		}
		return size;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		mNotifyManager.cancel(id);
		publishProgress(isSuccess ? SUCCESS : FAILURE);
	}
}
