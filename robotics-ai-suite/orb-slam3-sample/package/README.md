# Building ORB-SLAM3 Debian package

This step-by-step guide will detail how to build ORB-SLAM3 into a Debian package. Execute the following steps on an Ubuntu-based Linux development system:

1. Install prerequiste development tools:

```
$ apt update
$ apt install build-essential make cmake git wget debhelper devscripts equivs
```

2. Add the Robotics AI Suite APT repository to your APT sources:

```
$ sudo -E wget -O- https://eci.intel.com/sed-repos/gpg-keys/GPG-PUB-KEY-INTEL-SED.gpg | sudo tee /usr/share/keyrings/sed-archive-keyring.gpg > /dev/null
$ sudo echo "deb [signed-by=/usr/share/keyrings/sed-archive-keyring.gpg] https://eci.intel.com/sed-repos/$(source /etc/os-release && echo $VERSION_CODENAME) sed main" | sudo tee /etc/apt/sources.list.d/sed.list
```

3. Add the Intel RealSensse APT repository to your APT sources:

```
$ sudo -E wget -O- https://librealsense.intel.com/Debian/librealsense.pgp | sudo tee /usr/share/keyrings/librealsense.pgp > /dev/null
$ sudo echo "deb [signed-by=/usr/share/keyrings/librealsense.pgp] https://librealsense.intel.com/Debian/apt-repo `lsb_release -cs` main" | sudo tee /etc/apt/sources.list.d/librealsense.list
```

4. Update your APT sources:

```
$ sudo apt update
```

5. Clone the ORB-SLAM3 source:

```
$ git clone https://github.com/UZ-SLAMLab/ORB_SLAM3.git
```

6. Clone the Robotics AI Suite source:

```
$ git clone https://github.com/open-edge-platform/edge-ai-suites.git
```

7. Copy the Robotics AI Suite ORB-SLAM3 patches:

```
$ cp ./edge-ai-suites/robotics-ai-suite/orb-slam3/package/patches/* ./ORB_SLAM3
```

8. Apply the patches:

```
$ cd ORB_SLAM3
$ git apply *.patch
$ rm *.patch
```

9. Install the package build dependencies:

```
$ sudo mk-build-deps -i --host-arch amd64 --build-arch amd64 -t "apt-get -y -q -o Debug::pkgProblemResolver=yes --no-install-recommends --allow-downgrades" debian/control
```

10. Build the Debian package:

```
dpkg-buildpackage
```

The Debian package will reside in the parent directory of `ORB_SLAM3`.

```
$ ls ../ -1
ORB_SLAM3
liborb-slam3-dbgsym_1.0-1_amd64.ddeb
liborb-slam3-dev_1.0-1_amd64.deb
liborb-slam3_1.0-1_amd64.deb
orb-slam3-dbgsym_1.0-1_amd64.ddeb
orb-slam3_1.0-1.dsc
orb-slam3_1.0-1.tar.gz
orb-slam3_1.0-1_amd64.buildinfo
orb-slam3_1.0-1_amd64.changes
orb-slam3_1.0-1_amd64.deb
```

