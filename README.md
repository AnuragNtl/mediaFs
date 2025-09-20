# MediaFs
[Fuse](https://github.com/libfuse/libfuse) based media server.

### Features:
* Server and client side caching.

### Build:

```
mkdir build
cd build
cmake ..
cmake --build .
```

### Usage:

```
usage: mediafs --t=client|server --d=PATH
        -t type=client|server            act as mediafs client or server
        -d directory=PATH the mount point for client OR the source directory for server

```

### TODO
[ ] Auth layer 
[ ] Save cache to disk, resume.


