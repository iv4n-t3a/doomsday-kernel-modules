### Penguin malloc - blazyngly fast memory safe allocator ported to userspace from penguin OS

#### Adding to your service:

```cmake
add_subdirectory(path/to/penguin_malloc)
target_link_libraries(your_target penguin_malloc)
```

#### Running tests

    mkdir build && cd build
    cmake .. -DBUILD_TESTS=ON
    make
    ./penguin_malloc_tests
