# Hand integration-test asset

`hand_test.ppm` is a lossless RGB conversion (resized to 358×376) of MediaPipe's official `pointing_up.jpg` test asset from MediaPipe v0.10.26:

`https://storage.googleapis.com/mediapipe-assets/pointing_up.jpg?generation=1662650662527717`

The upstream asset is distributed as MediaPipe test data under the repository's Apache-2.0 licensing terms. It was converted to binary PPM solely so the C++ integration test can load it without OpenCV or an image-decoding dependency. SHA-256 of the committed PPM is recorded in the validation report.
