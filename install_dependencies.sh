#!/usr/bin/env bash

sudo apt update && sudo apt install -y python3-dev python3-pip libglib2.0-dev

# Install Bazel
sudo apt install apt-transport-https curl gnupg -y
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg
sudo mv bazel-archive-keyring.gpg /usr/share/keyrings
echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list

#install realsense driver dependencies
sudo apt update && sudo apt install -y libjpeg-dev libtbb-dev libtiff5-dev libpng-dev libboost-all-dev
