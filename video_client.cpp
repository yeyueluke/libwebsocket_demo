#include <libwebsockets.h>
#include <opencv2/opencv.hpp>
struct per_session_data {
    std::vector<unsigned char> recv_buffer;
};
static int callback_client(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    auto *psd = (per_session_data *)user;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_RECEIVE: {
	    //std::cout << "size " << len << std::endl;
            //std::vector<uchar> buffer((uchar*)in, (uchar*)in + len);
            //cv::Mat frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
            //if (!frame.empty()) {
               // cv::imshow("Received", frame);
              //  cv::waitKey(1);
           // }
	    // 添加接收到的片段数据
            psd->recv_buffer.insert(psd->recv_buffer.end(), (unsigned char*)in, (unsigned char*)in + len);
	    //std::cout << "here\n";
            // 检查是否是最后一个片段
            if (lws_is_final_fragment(wsi)) {
                cv::Mat frame = cv::imdecode(psd->recv_buffer, cv::IMREAD_COLOR);
                if (!frame.empty()) {
                    cv::imshow("Received Frame", frame);
                    cv::waitKey(1);
                } else {
                    std::cerr << "⚠ 图像解码失败，大小 = " << psd->recv_buffer.size() << std::endl;
                }
                psd->recv_buffer.clear();
            }
            break;
        }
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "video-protocol", callback_client,  sizeof(per_session_data), 1920 * 1536 },
    { NULL, NULL, 0, 0 }
};

int main() {
    struct lws_context_creation_info info = {};
    struct lws_client_connect_info ccinfo = {};

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    struct lws_context *context = lws_create_context(&info);
    if (!context) return -1;

    ccinfo.context = context;
    ccinfo.address = "192.168.1.104";  // 发送端 IP
    ccinfo.port = 9000;
    ccinfo.path = "/";
    ccinfo.host = ccinfo.address;
    ccinfo.origin = ccinfo.address;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    struct lws *wsi = lws_client_connect_via_info(&ccinfo);

    while (true) {
        lws_service(context, 0);
    }

    lws_context_destroy(context);
    return 0;
}
//g++ video_client.cpp -o receiver `pkg-config --cflags --libs opencv4 libwebsockets`

