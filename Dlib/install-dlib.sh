sudo apt-get install -y libdlib-dev
sudo apt-get install libopenblas-dev liblapack-dev -y
sudo apt install libjxl-dev -y
sudo apt install libavdevice-dev -y
sudo apt install libavdevice-dev libavfilter-dev libavformat-dev -y
sudo apt install libavcodec-dev libswresample-dev libswscale-dev -y
sudo apt install libavutil-dev -y

git clone https://github.com/davisking/dlib.git
cd dlib

rm -rf build

mkdir build
cd build
cmake -D DLIB_PNG_SUPPORT=1 \
-D DLIB_JPEG_SUPPORT=1 \
-D CMAKE_INSTALL_PREFIX=/usr/local/include/ \
..

make -j12
make install
sudo ldconfig -v

echo "Copy source code to /usr/local/include/dlib"
cd ../
sudo cp -r dlib/ /usr/local/include/

echo "Set permission to /usr/local/include/dlib"
sudo chown -R pt:pt /usr/local/include/dlib

#make examples:
#cd ..
#cd examples
#rm -rf build
#mkdir build
#cd build
#cmake ..
#make -j8

#echo "Run demo"
#./webcam_face_pose_ex
