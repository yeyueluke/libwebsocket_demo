#include <opencv2/opencv.hpp>
#include <libwebsockets.h>
#include <string.h>

#define MAX_PAYLOAD_SIZE (1920*1280) // 预留比211KB稍大空间

static unsigned char buffer[LWS_PRE + MAX_PAYLOAD_SIZE];
static size_t image_size = 0;

static int callback_video_stream(struct lws *wsi, enum lws_callback_reasons reason,
                                 void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("Client connected\n");
            lws_callback_on_writable(wsi);
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            std::cout << "size " << image_size << std::endl;
            if (image_size > 0) {
                lws_write(wsi, &buffer[LWS_PRE], image_size, LWS_WRITE_BINARY);
                image_size = 0;
            }
            lws_callback_on_writable(wsi); // 持续发送
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "video-protocol", callback_video_stream, 0, MAX_PAYLOAD_SIZE },
    { NULL, NULL, 0, 0 }
};

int main() {
    // 摄像头初始化
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cout << "not open camera\n";
        return -1;
    }

    struct lws_context_creation_info info = {};
    info.port = 9000;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        std::cout << "ws createa failed\n";
        return -1;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cout << "no data\n";
            continue;
        }

        std::vector<uchar> buf;
        cv::imencode(".jpg", frame, buf);
        image_size = buf.size();
        if (image_size > MAX_PAYLOAD_SIZE) {
            std::cout << "too large " << image_size << std::endl;
            continue;
        }

        memcpy(&buffer[LWS_PRE], buf.data(), image_size);

        lws_service(context, 0);  // 触发写回调
    }

    lws_context_destroy(context);
    return 0;
}
//g++ camera_ws_sender.cpp -o sender `pkg-config --cflags --libs opencv4 libwebsockets`

