// g++ -std=c++17 ws_client_opencv.cpp -o ws_client_opencv `pkg-config --cflags --libs opencv4` -lwebsockets -pthread


#include <libwebsockets.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#include <condition_variable>
#include <thread>
#include <atomic>
std::queue<std::vector<uchar>> frame_queue;
std::mutex queue_mutex;
std::condition_variable frame_cv;
std::atomic<bool> running = true;


static int callback_receive_image(struct lws *wsi, enum lws_callback_reasons reason,
                                  void *user, void *in, size_t len) {
    std::vector<uchar> frame;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "[Client] Connected to server." << std::endl;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            std::cout << "[Client] Received image data: " << len << " bytes." << std::endl;
            static bool flag3 = true;
            //  std::cout << "pSurf->surfaceList[0].dataSize  " << pSurf->surfaceList[0].dataSize << std::endl;
            if (flag3) {
                flag3 = false;
                int file;
                file = open("tt.jpg", O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);\
                // auto p = pSurf->surfaceList[0].mappedAddr.addr[0];
                if (-1 == file)
                    std::cout << "Failed to open file for frame saving\n";

                if (-1 == write(file, in, len))
                {
                    close(file);
                    std::cout << "Failed to write frame into file\n";
                }

                close(file);
            }

            // 将接收到的 JPG 数据解码为 OpenCV 图像并显示
#if 0
            {
                std::vector<uchar> jpg_data((uchar *)in, (uchar *)in + len);
                cv::Mat image = cv::imdecode(jpg_data, cv::IMREAD_COLOR);

                if (!image.empty()) {
                    cv::imshow("WebSocket Image Viewer", image);
                    cv::waitKey(1);  // 1ms delay to update window
                } else {
                    std::cerr << "[Client] Failed to decode image." << std::endl;
                }
            }
#endif
            frame.assign((uchar *)in, (uchar *)in + len);
             {
                std::lock_guard<std::mutex> lock(queue_mutex);
                frame_queue.push(std::move(frame));
             }
             frame_cv.notify_one();
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "[Client] Connection error." << std::endl;
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "[Client] Connection closed." << std::endl;
            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "video-protocol",
        callback_receive_image,
        0,
        1920 * 1536, // 可接收的最大图像大小
    },
    { nullptr, nullptr, 0, 0 }
};

int main() {
    struct lws_context_creation_info info;
    std::memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    struct lws_context *context = lws_create_context(&info);

    struct lws_client_connect_info ccinfo = {0};
    ccinfo.context = context;
    ccinfo.address = "192.168.1.104"; // 🟡 替换为你的设备A IP
    ccinfo.port = 9000;               // 🟡 替换为你的 WebSocket 服务端口
    ccinfo.path = "/";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0; // 不使用SSL

    if (!lws_client_connect_via_info(&ccinfo)) {
        std::cerr << "[Client] Failed to connect." << std::endl;
        return 1;
    }

    std::cout << "[Client] Connecting..." << std::endl;
std::thread display_thread([&]() {
    while (running) {
        std::vector<uchar> frame;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            frame_cv.wait(lock, [&] { return !frame_queue.empty() || !running; });

            if (!running && frame_queue.empty()) 
                break;

            frame = std::move(frame_queue.front());
            frame_queue.pop();
        }

        cv::Mat image = cv::imdecode(frame, cv::IMREAD_COLOR);
        if (!image.empty()) {
            cv::imshow("WebSocket Image Viewer", image);
            cv::waitKey(1);
        } else {
            std::cerr << "[Client] Failed to decode image." << std::endl;
        }
    }
});

    // 主事件循环
    while (true) {
        lws_service(context, 50);  // 内部处理消息，每 50ms
        if (cv::waitKey(1) == 27) break;  // 按下 ESC 键退出
    }

    lws_context_destroy(context);
    cv::destroyAllWindows();
    return 0;
}

