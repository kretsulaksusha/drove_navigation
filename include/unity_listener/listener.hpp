#ifndef DRONE_NAVIGATION_LISTENER_HPP
#define DRONE_NAVIGATION_LISTENER_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>
#ifdef _WIN32
#include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
 #pragma comment(lib,"ws2_32.lib")
 using socklen_t = int;
 #define CLOSESOCK closesocket
#else
#define CLOSESOCK close
#endif
#include <filesystem>
#include <set>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "model_types.hpp"
#include "image_processor.hpp"


namespace fs = std::filesystem;


// ------------------------------------------------------------------
// Допоміжна функція — відкрити TCP-слухач
int listenOn(uint16_t port);

// ------------------------------------------------------------------
// Потік: позиції дрона (TCP 20001)
void poseThread();

void mainThread();

void imageThread();


#endif //DRONE_NAVIGATION_LISTENER_HPP
