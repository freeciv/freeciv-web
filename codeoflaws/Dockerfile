FROM golang:1.11-alpine as builder
COPY . /go/src/github.com/freeciv/freeciv-web/codeoflaws
WORKDIR /go/src/github.com/freeciv/freeciv-web/codeoflaws/codeoflaws
RUN set -ex \
    && apk add --no-cache --virtual .build-deps \
    gcc libc-dev git \
    && go get github.com/rminnich/go9p \
    github.com/docker/go-plugins-helpers/volume \
    github.com/golang/glog github.com/google/uuid \
    gopkg.in/src-d/go-git.v4
RUN go install --ldflags '-extldflags "-static"' \
    && apk del .build-deps
CMD ["/go/bin/codeoflaws"]

FROM alpine
RUN apk add ca-certificates
RUN mkdir -p /run/docker/plugins /mnt/volumes
COPY --from=builder /go/bin/codeoflaws .
CMD ["codeoflaws"]
