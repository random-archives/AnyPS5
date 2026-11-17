# libSceAgcDriver

## add Functional

- [Unmapped storage images](../../../core/libs/prx/libSceAgcDriver/Graphics/src/Texture.cpp) (libSceAgcDriver): GPU results of a storage image that are still pending when the title unmaps any of their guest memory are not stored. The unmap observer that retires unmapped depth planes marks the pending units overlapping the range; they stay pending while the unmap is in progress and are dropped at their next store, also when the title has mapped the range again by then. A unit that receives new results is unmarked. What the console does with such a store is not known.
