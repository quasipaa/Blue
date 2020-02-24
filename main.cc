#include <napi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

using namespace Napi;

// @const
const char* NOT_OPEN_DEV = "Could not turn on the Bluetooth adapter!";
const char* NOT_QUERY = "Unable to perform Bluetooth discovery!";

/*
蓝牙设备信息
address 地址
name 名称
*/
struct bt_device {
    char address[19];
    char name[248];
};

/*
蓝牙搜索信息
num_rsp 设备数
devices 设备组
*/
struct bt_inquiry {
    int num_rsp;
    bt_device *devices;
};

/*
扫描
offset 找到的设备数
*/
bt_inquiry bluetooth_scan () {
    int i, dev_id, sock, len, max_rsp, flags, num_rsp;
    inquiry_info *q_info = NULL;
    char addr[19] = {0};
    char name[248] = {0};
    bt_inquiry result;
    
    /*
    准备蓝牙适配器；
    将NULL传递给hci_get_route是为了
    自动检索第一个适配器；
    */
    dev_id = hci_get_route(NULL);   // 打开蓝牙适配器
    sock = hci_open_dev(dev_id);    // 打开适配器连接
    if (dev_id < 0 || sock < 0) {
        throw NOT_OPEN_DEV;
    }

    /*
    执行蓝牙发现
    */
    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    q_info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &q_info, flags);
    if(num_rsp < 0) {
        throw NOT_QUERY;
    }
    
    /*
    记录结果长度
    初始化设备组
    */
    result.num_rsp = num_rsp;
    result.devices = (bt_device*)malloc(num_rsp * sizeof(bt_device));

    /*
    获取所有蓝牙设备
    回传信息
    */
    for (i = 0; i < num_rsp; i ++) {
        ba2str(&(q_info+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(q_info+i)->bdaddr, sizeof(name), name, 0) < 0) {
            strcpy(name, "[unknown]");
            strcpy(result.devices[i].address, addr);
            strcpy(result.devices[i].name, name);
        }
    }

    // 结束清理
    free(q_info);
    close(sock);
    
    // 返回结果数组
    return result;
};

/*
异步包装
创建异步线程抽象类
*/
class InquireWorker: public AsyncWorker {
    public:
        InquireWorker(Function &callback)
            : AsyncWorker(callback) {}
        ~InquireWorker() {}
    private:
        bt_inquiry inquiryResult;
    
    /*
    开始执行
    扫描
    */
    void Execute () {
        inquiryResult = bluetooth_scan();
    }

    /*
    执行完成
    遍历结果数组
    转为JS类型
    */
    void OnOK () {
        HandleScope scope(Env());
        for (int i = 0; i < inquiryResult.num_rsp; i ++) {
            Object value = Object::New(Env());
            value.Set("address", inquiryResult.devices[i].address);
            value.Set("name", inquiryResult.devices[i].name);
            Callback().Call({ value });
        }
    }
};

/*
蓝牙扫描包装
创建异步任务
*/
Value BluetoothScan (const CallbackInfo& info) {
    Function callback = info[0].As<Function>();
    InquireWorker* worker = new InquireWorker(callback);
    worker -> Queue();
    return info.Env().Undefined();
};

/*
将所有模块注册
Scan 蓝牙扫描
*/
Object Init (Env env, Object exports) {
    exports.Set(String::New(env, "Scan"), Function::New(env, BluetoothScan));
    return exports;
};

// 注册模块
NODE_API_MODULE(blue, Init);

// https://people.csail.mit.edu/albert/bluez-intro/c404.html
// https://nodejs.org/dist/latest-v12.x/docs/api/n-api.html
// https://github.com/nodejs/node-addon-api
