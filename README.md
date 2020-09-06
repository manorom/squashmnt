# squashmnt
A simple tool to mount an overlay of a squashfs image and a writeable directory

## Usage

Mounting squashfs image `image.sqfs` with an upper directory `/upper` and a work directory `/work` to `/mount`

```
squashmnt  /mount image.sqfs /upper /work
```

Mounting squashfs image `imsage.sqfs` to `/mount` with upper and work directory being created in `/tmp`

```
squashmnt -t /mount image.sqfs
```

Unmounting an image mounted at `/mount`

```
squashmnt -u /mount
```

## Configure and Build

```
mkdir BUILD
cd BUILD
ccmake ../
make && make install
```
