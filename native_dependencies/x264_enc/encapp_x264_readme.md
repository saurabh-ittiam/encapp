## Repo Details
Repo [link](https://github.com/saurabh-ittiam/encapp)

Branches
- Android: android_x264_on_master
- iOS: iOS_x264_on_master
- Battery tests: android_battery_tests_hwcodec_swcodec

#### How to get the .so for x264?
- Clone the x264 using below command
   ```bash
   git clone https://code.videolan.org/videolan/x264.git
   ```

- Run the below command:

   ```bash
   ./configure --prefix=./android/arm64-v8a --enable-static --enable-pic --host=aarch64-linux --cross-prefix=$TOOLCHAIN/bin/llvm- --sysroot=$TOOLCHAIN/sysroot --enable-strip --bit-depth=8 --chroma-format=420 --disable-opencl --disable-interlaced
   ```

- This will generate the `libx264.a`. Copy the `libx264.a` to `native_dependencies/x264_enc/jni/src`

- Download the android-ndk from this [link](https://developer.android.com/ndk/downloads). Preferably `android-ndk-r26d-windows`

- In the `Makefile` located at `native_dependencies/x264_enc/jni/Makefile` edit the path of android-ndk. Use `make` to build the shared library `libx264.so`. It will copy to jniLibs path.
