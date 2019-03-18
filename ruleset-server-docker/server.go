package codeoflaws

import (
	"context"
	"encoding/binary"
	"strings"
	"time"

	"github.com/golang/glog"
	"github.com/rminnich/go9p"
	git "gopkg.in/src-d/go-git.v4"
	"gopkg.in/src-d/go-git.v4/config"
	"gopkg.in/src-d/go-git.v4/plumbing"
	"gopkg.in/src-d/go-git.v4/plumbing/object"
)

type Server struct {
	go9p.Srv

	Repo *git.Repository
}

type fid struct {
	path     string
	tree     *object.Tree
	qid      go9p.Qid
	contents []byte
}

func hashQid(hash plumbing.Hash) go9p.Qid {
	return go9p.Qid{Path: binary.LittleEndian.Uint64(hash[0:8])}
}

func treeFileFid(tree *object.Tree, path string) (*fid, error) {
	te, err := tree.FindEntry(path)
	if err != nil {
		return nil, err
	}

	fi, err := tree.TreeEntryFile(te)
	if err != nil {
		return nil, err
	}

	cont, err := fi.Contents()
	if err != nil {
		return nil, err
	}

	return &fid{
		path:     path,
		qid:      hashQid(fi.Hash),
		contents: []byte(cont),
	}, nil
}

func (s *Server) revisionTree(rev string) (*object.Tree, error) {
	h, err := s.Repo.ResolveRevision(plumbing.Revision(rev))
	if err != nil {
		return nil, err
	}
	if h == nil {
		return nil, err
	}

	commit, err := s.Repo.CommitObject(*h)
	if err != nil {
		return nil, err
	}

	rt, err := s.Repo.TreeObject(commit.TreeHash)
	if err != nil {
		return nil, err
	}

	return rt.Tree("data")
}

func (s *Server) Attach(req *go9p.SrvReq) {
	if req.Afid != nil {
		req.RespondError(go9p.Enoauth)
		return
	}

	tc := req.Tc
	rev := tc.Aname
	if rev == "" {
		rev = "master"
	}

	ctx := context.Background()
	if err := s.Repo.FetchContext(ctx, &git.FetchOptions{
		RefSpecs: []config.RefSpec{"+refs/heads/*:refs/heads/*"},
	}); err != nil && err != git.NoErrAlreadyUpToDate {
		glog.Error(err)
		req.RespondError(go9p.Enoent)
		return
	}

	t, err := s.revisionTree(rev)
	if err != nil {
		glog.Error(err)
		req.RespondError(go9p.Enoent)
		return
	}
	req.Fid.Aux = &fid{
		tree: t,
		qid:  hashQid(t.Hash),
	}

	q := hashQid(t.Hash)
	q.Type |= go9p.QTDIR

	req.RespondRattach(&q)
}

func (*Server) Clunk(req *go9p.SrvReq) { req.RespondRclunk() }

func (s *Server) Remove(req *go9p.SrvReq) {
	req.RespondError(go9p.Eperm)
}

func (s *Server) Walk(req *go9p.SrvReq) {
	tc := req.Tc
	f := req.Fid.Aux.(*fid)

	if req.Newfid.Aux == nil {
		req.Newfid.Aux = new(fid)
	}

	var qids []go9p.Qid
	var err error

	switch {
	case len(tc.Wname) == 0:
		req.Newfid.Aux = f
	case f.path == "":
		dt := f.tree
		rssp := strings.Split(tc.Wname[0], "@")
		switch len(rssp) {
		case 1:
		case 2:
			if dt, err = s.revisionTree(rssp[1]); err != nil {
				glog.Error(err)
				req.RespondError(go9p.Enoent)
				return
			}

		default:
			glog.Error("invalid path: %v", tc.Wname)
			req.RespondError(go9p.Enoent)
			return
		}

		t, err := dt.Tree(rssp[0])
		if err != nil {
			glog.Error(err)
			req.RespondError(go9p.Enoent)
			return
		}

		tq := hashQid(t.Hash)
		tq.Type |= go9p.QTDIR

		qids = append(qids, tq)
		switch len(tc.Wname) {
		case 1:
			req.Newfid.Type |= go9p.QTDIR
			req.Newfid.Aux = &fid{
				tree: t,
				path: rssp[0],
				qid:  tq,
			}
		case 2:
			nf, err := treeFileFid(t, tc.Wname[1])
			if err != nil {
				glog.Error(err)
				req.RespondError(go9p.Enoent)
				return
			}
			req.Newfid.Aux = nf
			qids = append(qids, nf.qid)
		default:
			req.RespondError(go9p.Enoent)
			return
		}

	case f.tree != nil:
		if len(tc.Wname) != 1 {
			req.RespondError(go9p.Enoent)
			return
		}
		nf, err := treeFileFid(f.tree, tc.Wname[0])
		if err != nil {
			glog.Error(err)
			req.RespondError(go9p.Enoent)
			return
		}
		req.Newfid.Aux = nf
		qids = append(qids, nf.qid)
	}

	req.RespondRwalk(qids)
}

func (s *Server) Read(req *go9p.SrvReq) {
	tc := req.Tc
	rc := req.Rc
	go9p.InitRread(rc, tc.Count)
	f := req.Fid.Aux.(*fid)

	if f.tree != nil {
		go9p.SetRreadCount(rc, 0)
		req.Respond()
		return
	}

	if tc.Offset >= uint64(len(f.contents)) {
		go9p.SetRreadCount(rc, 0)
		req.Respond()
		return

	}

	count := int(tc.Count)
	if remaining := len(f.contents) - int(tc.Offset); remaining < count {
		count = remaining
	}

	n := copy(rc.Data, f.contents[tc.Offset:int(tc.Offset)+count])
	glog.Infof("contes %d, count %d, copied %d", len(f.contents), count, n)
	go9p.SetRreadCount(rc, uint32(n))
	req.Respond()
}

func (*Server) Write(req *go9p.SrvReq) {
	req.RespondError(go9p.Eperm)
}

func (s *Server) Open(req *go9p.SrvReq) {
	f := req.Fid.Aux.(*fid)
	req.RespondRopen(&f.qid, 0)
}

func (*Server) Create(req *go9p.SrvReq) {
	req.RespondError(go9p.Eperm)
}

func (s *Server) Stat(req *go9p.SrvReq) {
	f := req.Fid.Aux.(*fid)

	d := &go9p.Dir{}

	if f.path == "" {
		d.Name = "data"
	} else {
		d.Name = f.path
	}
	d.Qid = f.qid
	d.Uid = "root"
	d.Gid = "root"
	d.Mode = 0755
	t := time.Now()
	d.Atime = uint32(t.Unix())
	d.Mtime = uint32(t.Unix())

	if f.tree == nil {
		d.Length = uint64(len(f.contents))
	} else {
		d.Mode |= go9p.DMDIR
	}

	req.RespondRstat(d)
}

func (*Server) Wstat(req *go9p.SrvReq) {
	req.RespondRwstat()
}
