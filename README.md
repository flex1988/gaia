### GAIA

SSD Memcached 

- Base on twemproxy && fatcache && forestdb 
- Support Linux only
- Multithread && High performance
- Persistent storage

### Build & Install

#### Install forestdb

1. git clone https://github.com/couchbase/forestdb
2. cd forestdb
3. mkdir build
4. cmake ../
5. make all
6. make install
7. echo "/usr/local/lib" >> /etc/ld.so.conf
8. ldconfig

#### Build

1. make

#### Benchmark

- 4 worker thread
- 16 core cpu && 12GB memory && SSD disk

##### READ 

- memtier_benchmark -P memcache_text -s localhost -p 8080 -t 6 -c 150 -n 100000 --ratio=0:1
- 252044 (avg:  256937) ops/sec

##### WRITE

- memtier_benchmark -P memcache_text -s localhost -p 8080 -t 6 -c 150 -n 100000 --ratio=1:0
- 9782 (avg:    9731) ops/sec


