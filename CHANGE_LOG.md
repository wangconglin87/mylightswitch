### 操作命令
根据无线接收的数据 pkg -> cmd.Data 中的内容执行相关操作。

```
#define GET_DEVICE_INFO 0 // 将设备信息发送给协调器，数据结构为struct DeviceInfo
#define SWITCH_ON       1   
#define SWITCH_OFF      2
#define SWITCH_TOGGLE   3   

struct DeviceInfo {
    uint16 commandId;
    uint16 shortAddr;
    uint8 extAddress[8];
};
```


### 2019-02-25
因为内存对齐为2字节，所以将commmandId从char改为unsigned int。
