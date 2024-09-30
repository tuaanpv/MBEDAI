sudo apt update
sudo apt upgrade
sudo apt install -y build-essential cmake pkg-config unzip yasm git checkinstall
sudo apt install -y libjpeg-dev libpng-dev libtiff-dev
sudo apt install -y libavcodec-dev libavformat-dev libswscale-dev libavresample-dev
sudo apt install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt install -y libxvidcore-dev x264 libx264-dev libfaac-dev libmp3lame-dev libtheora-dev
sudo apt install -y libfaac-dev libmp3lame-dev libvorbis-dev
sudo apt install -y libopencore-amrnb-dev libopencore-amrwb-dev
sudo apt install -y libdc1394-22 libdc1394-22-dev libxine2-dev libv4l-dev v4l-utils

# install dependencies
#sudo apt-get update
sudo apt-get install -y build-essential
sudo apt-get install -y cmake
sudo apt-get install -y libgtk2.0-dev
sudo apt-get install -y pkg-config
sudo apt-get install -y python-numpy python-dev
sudo apt-get install -y libavcodec-dev libavformat-dev libswscale-dev
sudo apt-get install -y libjpeg-dev libpng12-dev libtiff5-dev libjasper-dev
sudo apt-get install -y libjpeg-dev libpng12-dev libtiff5-dev libjasper-dev libdlib-dev
sudo apt-get install -y qt5-qmake
sudo apt-get install -y gstreamer1.0-libav
sudo apt-get install -y gstreamer1.0-nice
sudo apt-get install -y libtbb-dev
sudo apt-get install -y libx11-dev
sudo apt-get install -y libdlib-dev
sudo apt-get install -y cmake-qt-gui
sudo apt-get install -y libopenblas-dev liblapack-dev
sudo apt install -y libopencv-dev
 
sudo apt-get install -y libopencv-dev build-essential checkinstall cmake pkg-config yasm libjpeg-dev libjasper-dev libavcodec-dev libavformat-dev libswscale-dev libdc1394-22-dev libxine2 libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev libv4l-dev python-dev python-numpy libtbb-dev libqt4-dev libgtk2.0-dev libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev libtheora-dev libvorbis-dev libxvidcore-dev x264 v4l-utils tesseract-ocr libtesseract-dev libleptonica-dev zlibc libtbb2 libpng-dev libtiff-dev

################################

#cd /usr/include/linux
#sudo ln -s -f ../libv4l1-videodev.h videodev.h
#cd ~/Desktop/PT/OpenCV4.5/

sudo apt install -y libgtk-3-dev python3-dev python3-pip
sudo -H pip3 install -U pip numpy
sudo apt install -y python3-testresources

sudo apt install -y libtbb-dev libatlas-base-dev gfortran

sudo apt install -y libprotobuf-dev protobuf-compiler
sudo apt install -y libgoogle-glog-dev libgflags-dev
sudo apt install -y libgphoto2-dev libeigen3-dev libhdf5-dev doxygen

wget -O opencv.zip https://github.com/opencv/opencv/archive/refs/tags/4.10.0.zip
wget -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/refs/tags/4.10.0.zip
unzip opencv.zip
unzip opencv_contrib.zip

#echo "Create a virtual environtment for the python binding module (OPTIONAL)"
#sudo pip install virtualenv virtualenvwrapper
#sudo rm -rf ~/.cache/pip
#echo "Edit ~/.bashrc"
#export WORKON_HOME=$HOME/.virtualenvs
#export VIRTUALENVWRAPPER_PYTHON=/usr/bin/python3
#source /usr/local/bin/virtualenvwrapper.sh
#mkvirtualenv cv -p python3
#pip install numpy

echo "Procced with the installation"
cd opencv-4.10.0/
mkdir build
cd build

cmake -D CMAKE_BUILD_TYPE=RELEASE \
-D CMAKE_INSTALL_PREFIX=/usr/local \
-D WITH_TBB=ON \
-D ENABLE_FAST_MATH=1 \
-D CUDA_FAST_MATH=1 \
-D WITH_CUBLAS=1 \
-D WITH_CUDA=ON \
-D BUILD_opencv_cudacodec=OFF \
-D WITH_CUDNN=ON \
-D OPENCV_DNN_CUDA=ON \
#-D CUDA_ARCH_BIN=7.5 \	# 7.5 for rtx1650 (Turing arch)
-D CUDA_ARCH_BIN=8.9 \  # 8.9 for rtx4050 (Ada Lovelace arch)
-D WITH_V4L=ON \
-D WITH_QT=OFF \
-D WITH_OPENGL=ON \
-D WITH_GSTREAMER=ON \
-D OPENCV_GENERATE_PKGCONFIG=ON \
-D OPENCV_PC_FILE_NAME=opencv.pc \
-D OPENCV_ENABLE_NONFREE=ON \
-D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.10.0/modules \
-D INSTALL_PYTHON_EXAMPLES=OFF \
-D INSTALL_C_EXAMPLES=OFF \
-D BUILD_EXAMPLES=OFF ..

nproc
make -j12
sudo make install

sudo /bin/bash -c 'echo "/usr/local/lib" >> /etc/ld.so.conf.d/opencv.conf'
sudo ldconfig


