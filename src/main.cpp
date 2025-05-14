#include "listener.hpp"
#include <thread>


int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return -1;
    }
#endif
    std::thread tPose(poseThread);
    std::thread imageTask(imageThread);

    tPose.join();
    imageTask.join();

    return 0;
}
