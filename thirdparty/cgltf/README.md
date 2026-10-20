# cgltf

Vendored `cgltf.h` from https://github.com/jkuhlmann/cgltf/tree/v1.15.
The MIT license is included at the end of the header.

The engine supplies explicit allocation/free callbacks backed by a caller-owned
scratch arena. No parser memory uses the default heap allocator in this path.
