## Dependency installation

Install all necessary dependencies to run realsense lcm drivers you need to run the following:

`./install_dependencies.sh`

After that, you will need to compile:

`bazel build //...`

Finally, you can run the nodes:

`bazel run rgbd_sensor:realsense_rgbd_publisher`

