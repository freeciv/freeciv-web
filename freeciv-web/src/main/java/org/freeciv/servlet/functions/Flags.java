package org.freeciv.servlet.functions;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardWatchEventKinds;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;
import java.util.HashMap;
import java.util.Map;

import org.apache.commons.io.IOUtils;
import org.json.JSONObject;

public class Flags {

	private final static String FLAG_FILE = "flags.json";
	private static Flags instance;

	public static String flagPath(String flag) {
		return getInstance().getFlagPath(flag);
	}

	private static Flags getInstance() {
		if (instance == null) {
			instance = new Flags();
		}
		return instance;
	}

	private final Map<String, String> paths = new HashMap<>();

	private Flags() {

		final String watchPath = System.getProperty("catalina.base") + "/webapps/ROOT/meta/";
		final String flagsFilePath = watchPath + FLAG_FILE;

		try {
			updatePathsMap(flagsFilePath);
		} catch (IOException e1) {
			// Perhaps file is not there yet
		}

		Thread flagsFileListener = new Thread("Flags file listener") {

			@Override
			public void run() {

				try {
					Path flagsFile = Paths.get(watchPath);
					WatchService watcher = FileSystems.getDefault().newWatchService();
					flagsFile.register(watcher, //
							StandardWatchEventKinds.ENTRY_CREATE, //
							StandardWatchEventKinds.ENTRY_MODIFY);

					while (true) {

						WatchKey key;
						try {
							key = watcher.take();
						} catch (InterruptedException e) {
							return;
						}

						for (WatchEvent<?> event : key.pollEvents()) {
							WatchEvent.Kind<?> kind = event.kind();

							if (kind == StandardWatchEventKinds.OVERFLOW) {
								continue;
							} else if (kind == StandardWatchEventKinds.ENTRY_MODIFY) {

								@SuppressWarnings("unchecked")
								WatchEvent<Path> ev = (WatchEvent<Path>) event;
								String fileName = ev.context().toString();

								if (FLAG_FILE.equals(fileName)) {
									try {
										Thread.sleep(50);
									} catch (InterruptedException e) {
										return;
									}
									updatePathsMap(flagsFilePath);
								}
							}
						}

						boolean valid = key.reset();
						if (!valid) {
							break;
						}

					}
				} catch (IOException e) {
					// @TODO add logging
				}

			}

		};
		flagsFileListener.start();

	}

	private String getFlagPath(String flag) {
		synchronized (paths) {
			if (paths.containsKey(flag)) {
				return paths.get(flag);
			} else {
				return "unknown-web.png";
			}
		}
	}

	private void updatePathsMap(String path) throws FileNotFoundException, IOException {
		try (FileInputStream inputStream = new FileInputStream(path)) {
			String flagsJson = IOUtils.toString(inputStream);
			JSONObject object = new JSONObject(flagsJson);
			synchronized (paths) {
				paths.clear();
				for (String flag : object.keySet()) {
					paths.put(flag, object.getString(flag));
				}
			}
		} catch (Exception e) {
			// Nice try, will be better next time.
			return;
		}
	}
}
