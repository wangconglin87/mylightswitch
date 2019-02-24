## 数据结构
```
#define GET_DEVICE_INFO 0
#define SWITCH_ON       1   
#define SWITCH_OFF      2
#define SWITCH_TOGGLE   3    

struct DeviceInfo {
    char commandId;
    uint16 shortAddr;
    uint8 extAddress[8];
};
```
