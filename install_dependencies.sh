#!/usr/bin/env bash

sudo apt-get update \
&& sudo apt-get install -y python3-dev python3-pip libglib2.0-dev \
&& sudo rm -rf /var/lib/apt/lists/*

# Install Bazel
sudo apt-get update  \
&& sudo apt-get install apt-transport-https curl gnupg -y \
&& sudo rm -rf /var/lib/apt/lists/*
curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg
sudo mv bazel-archive-keyring.gpg /usr/share/keyrings
echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list

#install realsense driver dependencies
sudo apt-get update && sudo apt-get install -y libjpeg-dev libtbb-dev libtiff5-dev libpng-dev libboost-all-dev libeigen3-dev libfmt-dev libspdlog-dev && sudo rm -rf /var/lib/apt/lists/*
