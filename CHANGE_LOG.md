## 数据结构
```
#define GET_DEVICE_INFO 0
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
