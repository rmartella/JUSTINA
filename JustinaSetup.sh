#!/bin/bash
#FORMAT
FRM='\033['
BLACK='0;30'
RED='1;31'
GREEN='1;32'
YELLOW='1;33'
BLUE='1;34'
PURPLE='1;35'
CYAN='1;36'
WHITE='1;37'
BGRED=';41m'
BGGREEN=';42m'
BGYELLOW=';43m'
BGBLUE=';44m'
BGWHITE=';47m'
NC='\033[0m'
#SCRIPT START
sudo apt-get update
sudo apt-get install -y freeglut3-dev pkg-config build-essential libxmu-dev libxi-dev libusb-1.0-0-dev doxygen graphviz mono-complete
sudo apt-get install -y build-essential libgtk2.0-dev libjpeg-dev libtiff5-dev libjasper-dev libopenexr-dev cmake python-dev python-numpy python-tk libtbb-dev libeigen3-dev yasm libfaac-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev libvorbis-dev libxvidcore-dev libx264-dev sphinx-common texlive-latex-extra libv4l-dev libdc1394-22-dev libavcodec-dev libavformat-dev libswscale-dev ant default-jdk
#Se eliminan estas librerias por la actualización del QT
#libqt4-dev libqt4-opengl-dev libvtk5-qt4-dev default-jdk
cd $HOME
opencvFile="$(pwd)/opencv-3.2.0.zip"
opencv_contrib_file="$(pwd)/opencv_contrib-3.2.0.zip"
if [ ! -f "$opencvFile" ]; then
	wget https://sourceforge.net/projects/opencvlibrary/files/opencv-unix/3.2.0/opencv-3.2.0.zip
	unzip opencv-3.2.0.zip
fi
if [ ! -f "$opencv_contrib_file" ]; then
	wget https://github.com/opencv/opencv_contrib/archive/3.2.0.zip
	mv 3.2.0.zip opencv_contrib-3.2.0.zip
	unzip opencv_contrib-3.2.0.zip
fi
cd opencv-3.2.0
mkdir build
cd build
cmake -D WITH_TBB=ON -D BUILD_NEW_PYTHON_SUPPORT=ON -D WITH_V4L=ON -D INSTALL_C_EXAMPLES=ON -D INSTALL_PYTHON_EXAMPLES=ON -D BUILD_EXAMPLES=ON -D WITH_QT=ON -D WITH_OPENGL=ON -D WITH_VTK=ON -D WITH_OPENNI=ON -D WITH_OPENCL=OFF -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-3.2.0/modules ..
make -j4
sudo make install
sudo touch /etc/ld.so.conf.d/opencv.conf
sudo /bin/su -c "echo '/usr/local/lib' >> /etc/ld.so.conf.d/opencv.conf"
sudo ldconfig
cd $HOME
mkdir -p prime_sense
cd prime_sense
sensorKinect_file="$(pwd)/SensorKinect"
if [ ! -d "$sensorKinect_file" ]; then
	git clone https://github.com/ph4m/SensorKinect.git
fi
cd SensorKinect
git checkout unstable
cd ../SensorKinect/Platform/Linux/CreateRedist
./RedistMaker
cd ../Redist/Sensor-Bin-Linux-x64-v5.1.2.1/
sudo ./install.sh
echo -e "${FRM}${RED}${BGYELLOW}Prime sense drivers correctly installed${NC}"
echo -e "${FRM}${WHITE}${BGBLUE}Installing NITE for skeleton traking${NC}"
cd $HOME
nite_file="$(pwd)/NITE-Bin-Linux-x64-v1.5.2.23.tar.zip"
if [ ! -f "$nite_file" ]; then
	wget http://www.openni.ru/wp-content/uploads/2013/10/NITE-Bin-Linux-x64-v1.5.2.23.tar.zip
	unzip NITE-Bin-Linux-x64-v1.5.2.23.tar.zip
	tar -xvf NITE-Bin-Linux-x64-v1.5.2.23.tar.bz2
fi
cd NITE-Bin-Dev-Linux-x64-v1.5.2.23
sudo ./install.sh
echo -e "${FRM}${RED}${BGYELLOW}NITE correctly installed ${NC}"
echo -e "${FRM}${WHITE}${BGBLUE}Trying to install OpenNI to update default libraries${NC}"
cd $HOME
openni_file_dir="$(pwd)/OpenNI"
if [ ! -f "$openni_file_dir" ]; then
	git clone https://github.com/OpenNI/OpenNI
fi
cd OpenNI/
git checkout unstable
cd Platform/Linux/CreateRedist
./RedistMaker	
cd ../Redist/OpenNI-Bin-Dev-Linux-x64-v1.5.8.5/
sudo ./install.sh
echo -e "${FRM}${RED}${BGYELLOW} OpenNI correctly installed${NC}"
sudo apt-get -y install ros-kinetic-urg-node
sudo apt-get -y install ros-kinetic-joy
sudo apt-get -y install ros-kinetic-openni-camera
sudo apt-get -y install ros-kinetic-openni-launch
sudo apt-get -y install ros-kinetic-openni2-camera
sudo apt-get -y install ros-kinetic-openni2-launch
sudo apt-get -y install ros-kinetic-amcl
sudo apt-get -y install ros-kinetic-tf2-bullet
sudo apt-get -y install ros-kinetic-fake-localization
sudo apt-get -y install ros-kinetic-map-server
sudo apt-get -y install ros-kinetic-sound-play
echo -e "${FRM}${WHITE}${BGBLUE}Installing basic audio libraries${NC}"
sudo apt-get -y install libzbar-dev
sudo apt-get -y purge jackd2 jackd jackd2-firewire libjack-jackd2-dev pulseaudio-module-jack qjackctl
echo -e "${FRM}${WHITE}${BGBLUE}Audio support will be installed, choose <yes> when asked for real time permissions${NC}"
read -p "(Waiting for key press in order to continue)"
sudo apt-get -y install jackd2 libjack-jackd2-dev pulseaudio-module-jack qjackctl
echo -e "${FRM}${WHITE}${BGBLUE}Installing pyaudio lib for directional audio node${NC}"
sudo apt-get -y install libasound-dev portaudio19-dev libportaudio2 libportaudiocpp0
sudo apt-get -y install ffmpeg libav-tools
sudo easy_install pip
sudo pip install pyaudio==0.2.9 --upgrade
#Add user to dialout, in order to use Arduino and Texas instrument board----
sudo adduser $USER dialout
FILES="/usr/local/lib/libopencv*"
pathCopy="/opt/ros/kinetic/lib/"
pattherDelete=$pathCopy"libopencv*"
for f in $pattherDelete
        do
                sudo rm $f
        done

for f in $FILES
        do
                 filename=$(basename "$f")
                 extension="${filename##*.}"
                 if [ $extension = "so" ]; then
                         sudo cp $f $pathCopy
                         newfilename="${filename%.*}"3.$extension
                         sudo mv $pathCopy$filename $pathCopy$newfilename
                         sudo echo $newfilename
                         cd $pathCopy
                         sudo ln -s $newfilename.3.2 $newfilename.3.2.0
                         sudo ln -s $newfilename $newfilename.3.2
                 fi
                 #echo $f
                 #mv $f $f
        done
sudo mkdir /media/$USER/usbPDF/
echo -e "${FRM}${RED}${BGWHITE}You can now ${NC}${FRM}${BLACK}${BGWHITE}behold${NC}${FRM}${RED}${BGWHITE} the power of Justina software${NC}"
