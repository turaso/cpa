# cloudphoto

### Build

##### Prerequisites

- git
- cmake
- make

##### Linux

```console
user@workstation:<some-directory>$ git clone https://github.com/ramirsultanov/cloudphoto.git
user@workstation:<some-directory>$ mkdir build/
user@workstation:<some-directory>$ cd build/
user@workstation:<some-directory>$ cmake -DCMAKE_BUILD_TYPE=Release ../cloudphoto
user@workstation:<some-directory>$ sudo make -j4 install
```
