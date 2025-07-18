### Install Docker Engine and Docker Compose on Ubuntu

Install [Docker Engine](https://docs.docker.com/engine/install/ubuntu/) and [Docker Compose](https://docs.docker.com/compose/) according to the guide on the official website.

Before you install Docker Engine for the first time on a new host machine, you need to set up the Docker `apt` repository. Afterward, you can install and update Docker from the repository.

1. Set up Docker's `apt` repository.

```bash
# Add Docker's official GPG key:
sudo -E apt-get update
sudo -E apt-get install ca-certificates curl
sudo -E install -m 0755 -d /etc/apt/keyrings
sudo -E curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

# Add the repository to Apt sources:
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "${UBUNTU_CODENAME:-$VERSION_CODENAME}") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo -E apt-get update
```

2. Install the Docker packages.

To install the latest version, run:

```bash
sudo -E apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```



3. Verify that the installation is successful by running the `hello-world` image:

```bash
sudo docker run hello-world
```

This command downloads a test image and runs it in a container. When the container runs, it prints a confirmation message and exits.

4. Add user to group

```bash
sudo usermod -aG docker $USER
newgrp docker
```



5. Then pull base image

```bash
docker pull ubuntu:22.04
```



### Install the corresponding driver on the host

```bash
bash install_driver_related_libs.sh
```



**Note that above driver is the BKC(best known configuration) version, which can get the best performance but with many restrictions when installing the driver and building the docker image.**

**If BKC is not needed and other versions of the driver are already installed on the machine, you don't need to do this step.**



### Build and run docker image through scripts

> **Note that the default username is `openvino` and password is `intel` in docker image.**

##### Build and run docker image

```bash
bash build_docker.sh <IMAGE_TAG, default tfcc:latest> <DOCKERFILE, default Dockerfile_TFCC.dockerfile>  <BASE, default ubuntu> <BASE_VERSION, default 22.04> 
```

```
bash run_docker.sh <DOCKER_IMAGE, default tfcc:latest> <NPU_ON, default true>
```



```bash
cd $PROJ_DIR/docker
bash build_docker.sh tfcc:latest Dockerfile_TFCC.dockerfile
bash run_docker.sh tfcc:latest false
# After the run is complete, the container ID will be output, or you can view it through docker ps 
```

##### Enter docker

```bash
docker exec -it <container id> /bin/bash
```



##### Copy dataset

```
docker cp /path/to/dataset <container id>:/path/to/dataset
```



### Build and run docker image through docker compose

> **Note that the default username is `openvino` and password is `intel` in docker image.**

Modify `proxy`, `VIDEO_GROUP_ID` and `RENDER_GROUP_ID` in `.env` file.

```bash
# proxy settings
https_proxy=
http_proxy=
# base image settings
BASE=ubuntu
BASE_VERSION=22.04
# group IDs for various services
VIDEO_GROUP_ID=44
RENDER_GROUP_ID=110
# display settings
DISPLAY=$DISPLAY
```

You can get  `VIDEO_GROUP_ID` and `RENDER_GROUP_ID`  with the following command:

```bash
# VIDEO_GROUP_ID
echo $(getent group video | awk -F: '{printf "%s\n", $3}')
# RENDER_GROUP_ID
echo $(getent group render | awk -F: '{printf "%s\n", $3}')
```



##### Build and run docker image

```bash
cd $PROJ_DIR/docker
docker compose up tfcc -d
```

##### Enter docker

```bash
docker compose exec tfcc /bin/bash
```

##### Copy dataset

Find the container name or ID:

```bash
docker compose ps
```

Sample output:

```bash
NAME                IMAGE      COMMAND       SERVICE    CREATED         STATUS         PORTS
docker-tfcc-1    tfcc:latest   "/bin/bash"     tfcc   4 minutes ago   Up 9 seconds
```

copy dataset

```bash
docker cp /path/to/dataset docker-tfcc-1:/path/to/dataset
```