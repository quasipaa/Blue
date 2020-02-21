#include <napi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

using namespace Napi;

// 错误信息列表
const char* NOT_OPEN_DEV = "无法打开蓝牙适配器";
const char* NOT_QUERY = "无法执行蓝牙发现";

// 扫描
void BluetoothScan (const CallbackInfo& info) {
    Env env = info.Env();
    Function callback = info[0].As<Function>();
    
    inquiry_info *q_info = NULL;
    char addr[19] = {0};
    char name[248] = {0};

    /*
    准备蓝牙适配器；
    将NULL传递给hci_get_route是为了
    自动检索第一个适配器；
    */
    int dev_id = hci_get_route(NULL);   // 打开蓝牙适配器
    int sock = hci_open_dev(dev_id);    // 打开适配器连接
    if (dev_id < 0 || sock < 0) {
        throw Error::New(env, NOT_OPEN_DEV);
    }

    /*
    执行蓝牙发现
    */
    int len  = 8;
    int max_rsp = 255;
    int flags = IREQ_CACHE_FLUSH;
    q_info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    int num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &q_info, flags);
    if(num_rsp < 0) {
        throw Error::New(env, NOT_QUERY);
    }

    // 获取所有蓝牙设备
    // 回传信息
    for (int i = 0; i < num_rsp; i ++) {
        ba2str(&(q_info+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(q_info+i)->bdaddr, sizeof(name), name, 0) < 0)
            strcpy(name, "[unknown]");
        Object value = Object::New(env);
        value.Set("addr", addr);
        value.Set("name", name);
        callback.Call(env.Global(), {value});
    }

    // 结束清理
    free(q_info);
    close(sock);
}

// 模块注册处理
Object Init (Env env, Object exports) {
    String name = String::New(env, "scan");
    Function method = Function::New(env, BluetoothScan);
    exports.Set(name, method);
    return exports;
}

// 注册模块
NODE_API_MODULE(scan, Init);

// https://people.csail.mit.edu/albert/bluez-intro/c404.html
// https://nodejs.org/dist/latest-v12.x/docs/api/n-api.html
