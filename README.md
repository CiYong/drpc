# drpc
drpc is a automatic engine for easy development C/S, B/S service.
that can generate rpc interface, rpc interface test, orm data, configure definition, etc. 

# Version
0.1.0

# Require
* python2.7 [pyyaml](https://pypi.org/project/PyYAML/) [jinja2](https://pypi.org/project/Jinja2/)
* cpp11 [libzmq](https://github.com/zeromq/libzmq)
[cppzmq](https://github.com/zeromq/cppzmq)
[msgpack](https://github.com/msgpack/msgpack-c/tree/cpp_master)
[dorm](https://github.com/CiYong/dorm)


# Practice platform
Linux, Windows, UE4 windows

* Note:
  * Embedded linux: Ensure socket(Networking support) is opened in kernel.
  * Windows: Use CMake tools to compile generated interface file.
  * UE4: The template class of msgpack conflicts with UE4 foundation.


# Build
1. Definition the drpc interface file in interface path.
2. Execute command: python /bin/drpc.py -i interface
3. Get generated file in output path.

# Output
* interface: interface file for programing language.
* test: interface test

# Example:
python /bin/drpc.py -i sample

# Language support:
Implemented: cpp
In the Future: python, rust, websocket, 

# Development
1. Inherit class drpc::ServerHandler and implement the interface for your server logic.
2. Call the function dprc::run() to start your server.
