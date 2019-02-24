/**************************************************************************************************
Filename:       SampleApp.c
Revised:        $Date: 2019-02-24 18:32
Revision:       $Revision: 19453 $

Description:    Leo's light switch.
**************************************************************************************************/

/*********************************************************************
����������Ϣ�󣬽���ؽ�����͸�Э������ִ����ز���
0x00 �����豸IEEE Address��Short Address��Status
0x01 ��
0x02 ��
0x03 �л�
*********************************************************************/

/*********************************************************************
* INCLUDES
*/
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
* MACROS
*/

/*********************************************************************
* CONSTANTS
*/

/*********************************************************************
* TYPEDEFS
*/


/*********************************************************************
* GLOBAL VARIABLES
*/

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
    SAMPLEAPP_PERIODIC_CLUSTERID,
    SAMPLEAPP_FLASH_CLUSTERID,
    LEO_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
    SAMPLEAPP_ENDPOINT,              //  int Endpoint;
    SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
    SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
    SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
    SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
    SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
    (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
    SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
    (cId_t *)SampleApp_ClusterList   //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
uint8 SampleApp_TaskID;   // Task ID for internal task/event processing
// This variable will be received when
// SampleApp_Init() is called.
devStates_t SampleApp_NwkState;

uint8 SampleApp_TransID;  // This is the unique message ID (counter)

afAddrType_t SampleApp_Periodic_DstAddr;
afAddrType_t SampleApp_Flash_DstAddr;
afAddrType_t Leo_Coordinator_DstAddr;

/*********************************************************************
* LOCAL FUNCTIONS
*/
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendPeriodicMessage( void );
void SampleApp_SendFlashMessage( uint16 flashTime );

static void Handle_UartEvent(uint8 port, uint8 event);
/*********************************************************************
* NETWORK LAYER CALLBACKS
*/

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*********************************************************************
* @fn      SampleApp_Init
*
* @brief   Initialization function for the Generic App Task.
*          This is called during initialization and should contain
*          any application specific initialization (ie. hardware
*          initialization/setup, table initialization, power up
*          notificaiton ... ).
*
* @param   task_id - the ID assigned by OSAL.  This ID should be
*                    used to send messages and set timers.
*
* @return  none
*/
void SampleApp_Init( uint8 task_id )
{
    SampleApp_TaskID = task_id;
    SampleApp_NwkState = DEV_INIT;
    SampleApp_TransID = 0;
    
    Leo_Coordinator_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    Leo_Coordinator_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;
    Leo_Coordinator_DstAddr.addr.shortAddr = 0x0000;
    
    // Fill out the endpoint description.
    SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
    SampleApp_epDesc.task_id = &SampleApp_TaskID;
    SampleApp_epDesc.simpleDesc
        = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
    SampleApp_epDesc.latencyReq = noLatencyReqs;
    
    // Register the endpoint description with the AF
    afRegister( &SampleApp_epDesc );
    
    // ��ʼ������
    halUARTCfg_t uartconf;
    
    uartconf.baudRate = HAL_UART_BR_115200;
    uartconf.callBackFunc = Handle_UartEvent;
    uartconf.configured = TRUE;
    uartconf.flowControl = FALSE;
    uartconf.flowControlThreshold = 64;
    uartconf.idleTimeout = 6;
    uartconf.rx.maxBufSize = 128;
    uartconf.tx.maxBufSize = 128;
    uartconf.intEnable = TRUE;
    
    HalUARTOpen(HAL_UART_PORT_0, &uartconf);
    
    //��ʼ�� P1_0
    // ��Ĵ���Ĭ��ֵΪ��ͨio��������ģʽ�����뷽��
    //����ֻ��Ҫ�������������Ϊ�����
    P1DIR |= 0x01;
}

/*********************************************************************
* @fn      SampleApp_ProcessEvent
*
* @brief   Generic Application Task event processor.  This function
*          is called to process all events for the task.  Events
*          include timers, messages and any other user defined events.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - events to process.  This is a bit map and can
*                   contain more than one event.
*
* @return  none
*/
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
    afIncomingMSGPacket_t *MSGpkt;
    (void)task_id;  // Intentionally unreferenced parameter
    
    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
                // Received when a messages is received (OTA) for this endpoint
            case AF_INCOMING_MSG_CMD:
                SampleApp_MessageMSGCB( MSGpkt );
                break;
                
                // Received whenever the device changes state in the network
            case ZDO_STATE_CHANGE:
                SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                if ( (SampleApp_NwkState == DEV_ZB_COORD)
                    || (SampleApp_NwkState == DEV_ROUTER)
                        || (SampleApp_NwkState == DEV_END_DEVICE) )
                {
                    // Start sending the periodic message in a regular interval.
                    osal_start_timerEx( SampleApp_TaskID,
                                       SAMPLEAPP_SEND_PERIODIC_MSG_EVT,
                                       SAMPLEAPP_SEND_PERIODIC_MSG_TIMEOUT );
                }
                else
                {
                    // Device is no longer in the network
                }
                break;
                
            default:
                break;
            }
            
            // Release the memory
            osal_msg_deallocate( (uint8 *)MSGpkt );
            
            // Next - if one is available
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
        }
        
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    
    // Discard unknown events
    return 0;
}

/*********************************************************************
* LOCAL FUNCTIONS
*/

/*********************************************************************
* @fn      SampleApp_MessageMSGCB
*
* @brief   Data message processor callback.  This function processes
*          any incoming data - probably from other devices.  So, based
*          on cluster ID, perform the intended action.
*
* @param   none
*
* @return  none
*/
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
    switch ( pkt->clusterId )
    {
    case LEO_CLUSTERID:
        if(osal_memcmp(pkt->cmd.Data, "info", 4)) {
            
            char name[] = "0000BASIC_LIGHT_SWITCH";
            
            uint16 shortAddr;
            osal_memcpy(&shortAddr, &_NIB.nwkDevAddress, sizeof(uint16));
            
            char itoa[] = {'0','1','2','3','4','5','6','7','8','9',
            'A','B','C','D','E','F'};
            name[3] = (char) itoa[0xF & shortAddr];
            name[2] = (char) itoa[0xF & (shortAddr >> 4)];
            name[1] = (char) itoa[0xF & (shortAddr >> 8)];
            name[0] = (char) itoa[0xF & (shortAddr >> 12)];
            
            HalUARTWrite(HAL_UART_PORT_0, name, osal_strlen(name));
            
            AF_DataRequest(&Leo_Coordinator_DstAddr, &SampleApp_epDesc, 
                           LEO_CLUSTERID,
                           osal_strlen(name),
                           name,
                           &SampleApp_TransID,
                           AF_DISCV_ROUTE,
                           AF_DEFAULT_RADIUS);
        }
        
        if(osal_memcmp(pkt->cmd.Data, "on", 2)) {
            P1_0 = 1;
        }
        
        if(osal_memcmp(pkt->cmd.Data, "off", 3)) {
            P1_0 = 0;
        }
        break;
    }
}

// �����¼��ص�����
static void Handle_UartEvent(uint8 port, uint8 event) 
{
    if (port == HAL_UART_PORT_0) 
    {
        switch (event) {
        case HAL_UART_RX_FULL:
        case HAL_UART_RX_ABOUT_FULL:
        case HAL_UART_RX_TIMEOUT: {
            uint16 rxbuflen = Hal_UART_RxBufLen(HAL_UART_PORT_0);
            uint8* rxbuf = osal_mem_alloc(rxbuflen);
            
            if (rxbuf != NULL) 
            {
                HalUARTRead(HAL_UART_PORT_0, rxbuf, rxbuflen);
                //HalUARTWrite(HAL_UART_PORT_0, rxbuf, rxbuflen);
                
                if (osal_memcmp(rxbuf, "on", 2) ) {
                    P1_0 = 1;
                } else {
                    P1_0 = 0;
                }
            }
            
            osal_mem_free(rxbuf);
        }
        break;
        
        default:
            break;
        }
    }
}