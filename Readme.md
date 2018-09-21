This vulkan wrapper is based on the vulkan tutorial: https://github.com/Overv/VulkanTutorial

It is built in a pragmatic fashion, avoiding any absolutisms. It is guided by staying close to the vulkan primitives, using raii, and value semantics. Thus many primitives are wrapped in move-only raii types. Also overhead has been tolerated by storing additional handles needed f.e. for destruction (usually the device handle). No shared pointers are used, so resource scoping has to be done right by the user (i.e. dont destroy a logical device before anything that was produced from it).

Furthermore constructors take a lot of defaults from the tutorial. if finer level of control is needed more constructors have to be written.

In the helpers directory i start gathering larger composites and utils. Right now there is a standard swap chain with all stuff needed to actually run it, using the synchronization setup from the tutorial.

License
-------

The code listings in this repository is licensed as [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/).
By contributing to that directory, you agree to license your contributions to
the public under that same public domain-like license.
