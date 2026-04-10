# CPHe
## 构建(乌班图22.04)
### 依赖
sudo apt install build-essential cmake 

sudo apt install qt6-base-dev qt6-tools-dev-tools libqt6gui6 libqt6widgets6 libqt6qml6 

sudo apt install llvm clang 


### 在CPHe目录下执行 

cmake -B out/build/RelWithDebInfo-linux -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O0 -g -UNDEBUG" -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O0 -g -UNDEBUG"

cd /cpp/CPHe/out/build/RelWithDebInfo-linux

cmake --build . -j $(nproc)
## 执行
./CPHe