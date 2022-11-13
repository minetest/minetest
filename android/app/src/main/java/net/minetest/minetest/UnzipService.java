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
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

public class UnzipService extends IntentService {
	public static final String ACTION_UPDATE = "net.minetest.minetest.UPDATE";
	public static final String ACTION_PROGRESS = "net.minetest.minetest.PROGRESS";
	public static final String ACTION_PROGRESS_MESSAGE = "net.minetest.minetest.PROGRESS_MESSAGE";
	public static final String ACTION_FAILURE = "net.minetest.minetest.FAILURE";
	public static final int SUCCESS = -1;
	public static final int FAILURE = -2;
	public static final int INDETERMINATE = -3;
	private final int id = 1;
	private NotificationManager mNotifyManager;
	private boolean isSuccess = true;
	private String failureMessage;

	private static boolean isRunning = false;
	public static synchronized boolean getIsRunning() {
		return isRunning;
	}
	private static synchronized void setIsRunning(boolean v) {
		isRunning = v;
	}

	public UnzipService() {
		super("net.minetest.minetest.UnzipService");
	}

	@Override
	protected void onHandleIntent(Intent intent) {
		Notification.Builder notificationBuilder = createNotification();
		final File zipFile = new File(getCacheDir(), "Minetest.zip");
		try {
			setIsRunning(true);
			File userDataDirectory = Utils.getUserDataDirectory(this);
			if (userDataDirectory == null) {
				throw new IOException("Unable to find user data directory");
			}

			try (InputStream in = this.getAssets().open(zipFile.getName())) {
				try (OutputStream out = new FileOutputStream(zipFile)) {
					int readLen;
					byte[] readBuffer = new byte[16384];
					while ((readLen = in.read(readBuffer)) != -1) {
						out.write(readBuffer, 0, readLen);
					}
				}
			}

			migrate(notificationBuilder, userDataDirectory);
			unzip(notificationBuilder, zipFile, userDataDirectory);
		} catch (IOException e) {
			isSuccess = false;
			failureMessage = e.getLocalizedMessage();
		} finally {
			setIsRunning(false);
			if (!zipFile.delete()) {
				Log.w("UnzipService", "Minetest installation ZIP cannot be deleted");
			}
		}
	}

	private Notification.Builder createNotification() {
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

		Intent notificationIntent = new Intent(this, MainActivity.class);
		notificationIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
			| Intent.FLAG_ACTIVITY_SINGLE_TOP);
		int pendingIntentFlag = 0;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
			pendingIntentFlag = PendingIntent.FLAG_MUTABLE;
		}
		PendingIntent intent = PendingIntent.getActivity(this, 0,
			notificationIntent, pendingIntentFlag);

		builder.setContentTitle(getString(R.string.notification_title))
				.setSmallIcon(R.mipmap.ic_launcher)
				.setContentText(getString(R.string.notification_description))
				.setContentIntent(intent)
				.setOngoing(true)
				.setProgress(0, 0, true);

		mNotifyManager.notify(id, builder.build());
		return builder;
	}

	private void unzip(Notification.Builder notificationBuilder, File zipFile, File userDataDirectory) throws IOException {
		int per = 0;

		int size;
		try (ZipFile zipSize = new ZipFile(zipFile)) {
			size = zipSize.size();
		}

		int readLen;
		byte[] readBuffer = new byte[16384];
		try (FileInputStream fileInputStream = new FileInputStream(zipFile);
		     ZipInputStream zipInputStream = new ZipInputStream(fileInputStream)) {
			ZipEntry ze;
			while ((ze = zipInputStream.getNextEntry()) != null) {
				if (ze.isDirectory()) {
					++per;
					Utils.createDirs(userDataDirectory, ze.getName());
					continue;
				}
				publishProgress(notificationBuilder, R.string.loading, 100 * ++per / size);
				try (OutputStream outputStream = new FileOutputStream(
						new File(userDataDirectory, ze.getName()))) {
					while ((readLen = zipInputStream.read(readBuffer)) != -1) {
						outputStream.write(readBuffer, 0, readLen);
					}
				}
			}
		}
	}

	void moveFileOrDir(@NonNull File src, @NonNull File dst) throws IOException {
		try {
			Process p = new ProcessBuilder("/system/bin/mv",
				src.getAbsolutePath(), dst.getAbsolutePath()).start();
			int exitcode = p.waitFor();
			if (exitcode != 0)
				throw new IOException("Move failed with exit code " + exitcode);
		} catch (InterruptedException e) {
			throw new IOException("Move operation interrupted");
		}
	}

	boolean recursivelyDeleteDirectory(@NonNull File loc) {
		try {
			Process p = new ProcessBuilder("/system/bin/rm", "-rf",
				loc.getAbsolutePath()).start();
			return p.waitFor() == 0;
		} catch (IOException | InterruptedException e) {
			return false;
		}
	}

	/**
	 * Migrates user data from deprecated external storage to app scoped storage
	 */
	private void migrate(Notification.Builder notificationBuilder, File newLocation) throws IOException {
		if (Build.VERSION.SDK_INT >=  Build.VERSION_CODES.R) {
			return;
		}

		File oldLocation = new File(Environment.getExternalStorageDirectory(), "Minetest");
		if (!oldLocation.isDirectory())
			return;

		publishProgress(notificationBuilder, R.string.migrating, 0);
		if (!newLocation.mkdir()) {
			Log.e("UnzipService", "New installation folder cannot be made");
		}

		String[] dirs = new String[] { "worlds", "games", "mods", "textures", "client" };
		for (int i = 0; i < dirs.length; i++) {
			publishProgress(notificationBuilder, R.string.migrating, 100 * i / dirs.length);
			File dir = new File(oldLocation, dirs[i]), dir2 = new File(newLocation, dirs[i]);
			if (dir.isDirectory() && !dir2.isDirectory()) {
				moveFileOrDir(dir, dir2);
			}
		}

		for (String filename : new String[] { "minetest.conf" }) {
			File file = new File(oldLocation, filename), file2 = new File(newLocation, filename);
			if (file.isFile() && !file2.isFile()) {
				moveFileOrDir(file, file2);
			}
		}

		if (!recursivelyDeleteDirectory(oldLocation)) {
			Log.w("UnzipService", "Old installation files cannot be deleted successfully");
		}
	}

	private void publishProgress(@Nullable  Notification.Builder notificationBuilder, @StringRes int message, int progress) {
		Intent intentUpdate = new Intent(ACTION_UPDATE);
		intentUpdate.putExtra(ACTION_PROGRESS, progress);
		intentUpdate.putExtra(ACTION_PROGRESS_MESSAGE, message);
		if (!isSuccess)
			intentUpdate.putExtra(ACTION_FAILURE, failureMessage);
		sendBroadcast(intentUpdate);

		if (notificationBuilder != null) {
			notificationBuilder.setContentText(getString(message));
			if (progress == INDETERMINATE) {
				notificationBuilder.setProgress(100, 50, true);
			} else {
				notificationBuilder.setProgress(100, progress, false);
			}
			mNotifyManager.notify(id, notificationBuilder.build());
		}
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		mNotifyManager.cancel(id);
		publishProgress(null, R.string.loading, isSuccess ? SUCCESS : FAILURE);
	}
}
