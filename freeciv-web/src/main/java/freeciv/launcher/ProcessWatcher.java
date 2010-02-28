/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


package freeciv.launcher;

import java.lang.*;
import java.util.*;

public class ProcessWatcher extends Thread {

	private Process process;
	private SortedSet<Integer> portSet;
	int civclientPort;

	public ProcessWatcher(Process process, SortedSet<Integer> portSet,
			int civclientPort) {
		this.process = process;
		this.portSet = portSet;
		this.civclientPort = civclientPort;

	}

	public void run() {
		try {
			int result = process.waitFor();
			System.out.println("civclient " + civclientPort + " ended with: "
					+ result);
			// Remove process, and make port available.
			portSet.remove(this.civclientPort);
		} catch (InterruptedException err) {
			err.printStackTrace();
		}

	}
}
