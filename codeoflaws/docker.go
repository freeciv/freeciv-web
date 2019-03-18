package codeoflaws

import (
	"fmt"
	"os"
	"os/exec"
	"sync"
	"syscall"

	"github.com/docker/go-plugins-helpers/volume"
	"github.com/golang/glog"
	"github.com/google/uuid"
)

type VolumeDriver struct {
	p9SocketPath string

	sync.Mutex
	volumes map[string]*p9Volume
}

type p9Volume struct {
	mountPoint string
	revision   string
}

func NewVolumeDriver(p9SocketPath string) *VolumeDriver {
	return &VolumeDriver{
		p9SocketPath: p9SocketPath,
		volumes:      make(map[string]*p9Volume),
	}
}

func (d *VolumeDriver) Create(req *volume.CreateRequest) error {
	v := &p9Volume{}

	for key, val := range req.Options {
		switch key {
		case "revision":
			v.revision = val
		default:
		}
	}

	d.Lock()
	d.volumes[req.Name] = v
	d.Unlock()

	return nil

}

func (d *VolumeDriver) Remove(req *volume.RemoveRequest) error {
	d.Lock()
	defer d.Unlock()

	v, ok := d.volumes[req.Name]
	if !ok {
		return fmt.Errorf("volume %s not found", req.Name)
	}

	if err := os.RemoveAll(v.mountPoint); err != nil {
		return err
	}
	delete(d.volumes, req.Name)
	return nil
}

func (d *VolumeDriver) Path(req *volume.PathRequest) (*volume.PathResponse, error) {
	d.Lock()
	defer d.Unlock()

	v, ok := d.volumes[req.Name]
	if !ok {
		return &volume.PathResponse{}, fmt.Errorf("volume %s not found", req.Name)
	}

	return &volume.PathResponse{Mountpoint: v.mountPoint}, nil
}

func (d *VolumeDriver) Mount(req *volume.MountRequest) (*volume.MountResponse, error) {
	d.Lock()
	defer d.Unlock()

	v, ok := d.volumes[req.Name]
	if !ok {
		return &volume.MountResponse{}, fmt.Errorf("volume %s not found", req.Name)
	}

	v.mountPoint = "/mnt/volumes/" + uuid.New().String()
	if err := os.MkdirAll(v.mountPoint, 0755); err != nil {
		glog.Error(err)
		return &volume.MountResponse{}, err
	}

	//if err := syscall.Mount(d.p9SocketPath, v.mountPoint, "9p", uintptr(0), "version=9p2000.L,trans=unix"); err != nil {
	if _, err := exec.Command("/bin/mount", "-t", "9p", d.p9SocketPath, v.mountPoint, "-o", "version=9p2000.L,trans=unix").Output(); err != nil {
		glog.Error(err)
		return &volume.MountResponse{}, err
	}

	return &volume.MountResponse{Mountpoint: v.mountPoint}, nil
}

func (d *VolumeDriver) Unmount(req *volume.UnmountRequest) error {
	d.Lock()
	defer d.Unlock()
	v, ok := d.volumes[req.Name]
	if !ok {
		return fmt.Errorf("volume %s not found", req.Name)
	}

	return syscall.Unmount(v.mountPoint, 0)
}

func (d *VolumeDriver) Get(req *volume.GetRequest) (*volume.GetResponse, error) {
	d.Lock()
	defer d.Unlock()

	v, ok := d.volumes[req.Name]
	if !ok {
		return &volume.GetResponse{}, fmt.Errorf("volume %s not found", req.Name)
	}

	return &volume.GetResponse{Volume: &volume.Volume{Name: req.Name, Mountpoint: v.mountPoint}}, nil
}

func (d *VolumeDriver) List() (*volume.ListResponse, error) {
	d.Lock()
	defer d.Unlock()

	var vols []*volume.Volume
	for name, v := range d.volumes {
		vols = append(vols, &volume.Volume{Name: name, Mountpoint: v.mountPoint})
	}
	return &volume.ListResponse{Volumes: vols}, nil
}

func (d *VolumeDriver) Capabilities() *volume.CapabilitiesResponse {
	return &volume.CapabilitiesResponse{Capabilities: volume.Capability{Scope: "local"}}
}
