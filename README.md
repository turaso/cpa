# cloudphoto

### Description
This repository provides an app to control a cloud photo storage.

### Build

##### Prerequisites

- git
- cmake
- make

##### Linux

```console
user@workstation:<some-directory>$ git clone https://github.com/turaso/cpa.git
user@workstation:<some-directory>$ mkdir build/
user@workstation:<some-directory>$ cd build/
user@workstation:<some-directory>$ cmake -DCMAKE_BUILD_TYPE=Release ../cpa
user@workstation:<some-directory>$ sudo make -j4 install
```

### Run

##### Configure

```console
user@workstation:<some-directory>$ cloudphoto init
```

##### Upload a directory with photo (.jpg and .jpeg) to a cloud

```console
user@workstation:<some-directory>$ cloudphoto upload --album <album-name> [--path <path>=./]
```

##### Download a directory with photo (.jpg and .jpeg) from a cloud

```console
user@workstation:<some-directory>$ cloudphoto download --album <album-name> [--path <path>=./]
```

##### List albums or photos in a cload

```console
user@workstation:<some-directory>$ cloudphoto list [--album <album-name>]
```

##### Delete album or photo

```console
user@workstation:<some-directory>$ cloudphoto delete --album <album-name> [--photo <photo-name>]
```

##### Generate web site

```console
user@workstation:<some-directory>$ cloudphoto mksite
```
