package org.freeciv.model;

public class Game {

	private String host;

	private int port;

	private String version;

	private String patches;

	private String state;

	private String message;

	private long duration;

	private int players;

	private int turn;

	private String flag;

	private String player;

	public long getDuration() {
		return duration;
	}

	public String getFlag() {
		return flag;
	}

	public String getHost() {
		return host;
	}

	public String getMessage() {
		return message;
	}

	public String getPatches() {
		return patches;
	}

	public String getPlayer() {
		return player;
	}

	public int getPlayers() {
		return players;
	}

	public int getPort() {
		return port;
	}

	public String getState() {
		return state;
	}

	public int getTurn() {
		return turn;
	}

	public String getVersion() {
		return version;
	}

	public boolean isProtected() {
		return (message != null) && message.contains("password-protected");
	}

	public Game setDuration(long duration) {
		this.duration = duration;
		return this;
	}

	public Game setFlag(String flag) {
		this.flag = flag;
		return this;
	}

	public Game setHost(String host) {
		this.host = host;
		return this;
	}

	public Game setMessage(String message) {
		this.message = message;
		return this;
	}

	public Game setPatches(String patches) {
		this.patches = patches;
		return this;
	}

	public Game setPlayer(String player) {
		this.player = player;
		return this;
	}

	public Game setPlayers(int players) {
		this.players = players;
		return this;
	}

	public Game setPort(int port) {
		this.port = port;
		return this;
	}

	public Game setState(String state) {
		this.state = state;
		return this;
	}

	public Game setTurn(int turn) {
		this.turn = turn;
		return this;
	}

	public Game setVersion(String version) {
		this.version = version;
		return this;
	}

}
