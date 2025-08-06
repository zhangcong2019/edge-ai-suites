# Build and run docker image

## System requirements

**Operating System:**
* Ubuntu 24.04

**Software:**
* VPP SDK

## build docker image

1. Install VPPSDK and dependencies  
```
    sudo -E wget -O- https://eci.intel.com/sed-repos/gpg-keys/GPG-PUB-KEY-INTEL-SED.gpg | sudo tee /usr/share/keyrings/sed-archive-keyring.gpg > /dev/null
    echo "deb [signed-by=/usr/share/keyrings/sed-archive-keyring.gpg] https://eci.intel.com/sed-repos/$(source /etc/os-release && echo $VERSION_CODENAME) sed main" | sudo tee /etc/apt/sources.list.d/sed.list
    echo "deb-src [signed-by=/usr/share/keyrings/sed-archive-keyring.gpg] https://eci.intel.com/sed-repos/$(source /etc/os-release && echo $VERSION_CODENAME) sed main" | sudo tee -a /etc/apt/sources.list.d/sed.list
    sudo bash -c 'echo -e "Package: *\nPin: origin eci.intel.com\nPin-Priority: 1000" > /etc/apt/preferences.d/sed'
    sudo apt update
    sudo apt install intel-vppsdk

sudo bash /opt/intel/vppsdk/install_vppsdk_dependencies.sh
source /opt/intel/vppsdk/env.sh
```
2. Make sure docker is corrently installed and configured. Build docker image for reference application `build_sample.sh`


## run docker container
4. Add /usr/local/lib to $LD_LIBRARY_PATH: export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   You shall add this export command to your .bashrc or need to run it before running svet_app 
5. Basic test: 
```
./build/svet_app load sample_config/basic/1dec1disp.txt
```
