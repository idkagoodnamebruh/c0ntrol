# Hand integration-test asset

`hand_test.ppm` is a lossless RGB conversion (resized to 358×376) of MediaPipe's official `pointing_up.jpg` test asset from MediaPipe v0.10.26:

`https://storage.googleapis.com/mediapipe-assets/pointing_up.jpg?generation=1662650662527717`

The upstream asset is distributed as MediaPipe test data under the repository's Apache-2.0 licensing terms. It was converted to binary PPM solely so the C++ integration test can load it without OpenCV or an image-decoding dependency.

- Upstream JPEG SHA-256: `ecf8ca2611d08fa25948a4fc10710af9120e88243a54da6356bacea17ff3e36e`
- Committed PPM size: `403839` bytes
- Committed PPM SHA-256: `c470119d7285ac44785cbb152510418f2653a98050e1f0c283598145c9ee4daf`
