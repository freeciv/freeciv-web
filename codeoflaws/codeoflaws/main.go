package main

import (
	"flag"

	"github.com/docker/go-plugins-helpers/volume"
	"github.com/freeciv/freeciv-web/codeoflaws"
	"github.com/golang/glog"
	git "gopkg.in/src-d/go-git.v4"
)

const pluginSocketPath = "/run/docker/plugins/codeoflaws.sock"

var (
	p9SocketPath = flag.String("p9_socket", "/p9.sock", "Unix socket path for 9p server")
	repoDir      = flag.String("git_repo", "/mnt/repo/freeciv.git", "Path of git repository to use")
)

func main() {
	flag.Parse()

	d := codeoflaws.NewVolumeDriver(*p9SocketPath)
	h := volume.NewHandler(d)
	go h.ServeUnix(pluginSocketPath, 0)

	r, err := git.PlainOpen(*repoDir)
	if err == git.ErrRepositoryNotExists {
		glog.Infof("Cloning repo")
		r, err = git.PlainClone(*repoDir, true, &git.CloneOptions{
			URL: "https://github.com/zekoz/freeciv.git",
		})
	}
	if err != nil {
		glog.Fatal(err)
	}

	s := &codeoflaws.Server{Repo: r}
	s.Id = "codeoflaws"
	if !s.Start(s) {
		glog.Exit("start failed")
	}
	glog.Exit(s.StartNetListener("unix", *p9SocketPath))
}
