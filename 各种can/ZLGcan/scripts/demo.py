""" **************** zcanpro模块说明 ****************

ZCANPRO程序中提供了zcanpro模块，使用"import zcanpro"导入至自定义的脚本中即可使用。
提供的接口参考 zcanpro.pyi 文件。

"""

""" **************** 扩展脚本文件编写说明 ****************

扩展脚本文件（即提供给ZCANPRO程序执行的脚本）必须提供以下接口供ZCANPRO程序调用。

1. z_main()
    入口函数，ZCANPRO程序运行扩展脚本时会首先调用该函数，该函数退出时即为扩展脚本运行结束。
    编写时，请注意不要让该函数执行死循环，确保其能正常运行结束，或者接收到ZCANPRO程序发送的停止运行命令后能正常退出。

2. z_notify(type, obj)
    事件通知函数，ZCANPRO程序会在产生相应事件的时候调用该接口通知运行的脚本。
    * type: 事件类型，字符串类型，目前支持的类型如下
        a) "stop": 停止脚本运行，接收到该命令后应让z_main函数立即运行结束。

"""


#########################################################
# 以下示例程序，展示总线0的数据转发至总线1

import time
import zcanpro

stopTask = False

def z_notify(type, obj):
    zcanpro.write_log("Notify " + str(type) + " " + str(obj))
    if type == "stop":
        zcanpro.write_log("Stop...")
        global stopTask
        stopTask = True

"""
def z_main():
    zcanpro.write_log("transmit test")
    buses = zcanpro.get_buses()
    zcanpro.write_log("Get buses: " + str(buses))

    if len(buses) >= 2:
        global stopTask
        stopTask = False
        while not stopTask:
            result, frms = zcanpro.receive(buses[0]["busID"])
            if not result:
                zcanpro.write_log("Receive error!")
            elif len(frms) > 0:
                zcanpro.write_log("Received " + str(len(frms)))
                result = zcanpro.transmit(buses[1]["busID"], frms)
                if not result:
                    zcanpro.write_log("Transmit error!")
            time.sleep(0.01)
"""


#########################################################
# 以下示例，展示扩展脚本请求UDS诊断服务

def test_uds(busID):
    udsCfg = {
        "response_timeout_ms": 3000,
        "use_canfd": 1,
        "canfd_brs" : 1,
        "trans_ver": 0,
        "fill_byte": 0x00,         
        "frame_type": 0,           
        "trans_stmin_valid": 0,    
        "trans_stmin": 0,          
        "enhanced_timeout_ms": 5000,
        "fc_timeout_ms": 1000,
        "fill_mode": 0
    }
    zcanpro.uds_init(udsCfg)

    req = {
        "src_addr": 0x700,
        "dst_addr": 0x701,
        "suppress_response" :0,
        "sid": 0x19,
        "data":[0x02, 0xFF]
    }

    global stopTask
    stopTask = False
    while not stopTask:
        zcanpro.write_log("[UDS Tx] " + ("%02X " % req["sid"]) + " ".join('{:02X}'.format(a) for a in req["data"]))
        response = zcanpro.uds_request(busID, req)
        if not response["result"]:
            zcanpro.write_log("Request error! " + response["result_msg"])
        else:
            zcanpro.write_log("[UDS Rx] " + " ".join('{:02X}'.format(a) for a in response["data"]))
        time.sleep(1)

    zcanpro.uds_deinit()

"""
def z_main():
    zcanpro.write_log("uds test")
    buses = zcanpro.get_buses()
    zcanpro.write_log("Get buses: " + str(buses))
    if len(buses) >= 1:
        test_uds(buses[0]["busID"])
"""

#########################################################
# 以下示例，展示扩展脚本控制设备定时发送

def test_dev_auto_send(busID):
    autoSendFrms = [
        {
            "can_id": 0x100,
            "is_canfd": 0,
            "canfd_brs": 0,
            "data": [0, 1, 2, 3, 4],
            "interval_ms": 500,
            "delay_start_ms": 0
        },
        {
            "can_id": 0x101,
            "is_canfd": 1,
            "canfd_brs": 1,
            "data": [1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC],
            "interval_ms": 1000,
            "delay_start_ms": 2000
        }
    ]

    result = zcanpro.dev_auto_send_start(busID, autoSendFrms)
    if result == 0:
        zcanpro.write_log("start device auto send failed! ")
    else:
        zcanpro.write_log("device auto send started... ")

    startTime = time.time()
    frmStopTest = False
    frmRestartTest = False

    global stopTask
    stopTask = False
    while not stopTask:
        if not frmStopTest and time.time() - startTime >= 5:
            frmStopTest = True
            frmIndex = 0
            enable = 0
            zcanpro.dev_auto_send_enable_frame(busID, enable, frmIndex, autoSendFrms[0])

        if not frmRestartTest and time.time() - startTime >= 10:
            frmRestartTest = True
            frmIndex = 0
            enable = 1
            zcanpro.dev_auto_send_enable_frame(busID, enable, frmIndex, autoSendFrms[0])

        time.sleep(0.1)

    result = zcanpro.dev_auto_send_stop(busID)
    if result == 0:
        zcanpro.write_log("stop device auto send failed! ")
    else:
        zcanpro.write_log("device auto send stopped. ")

#"""
def z_main():
    zcanpro.write_log("device auto send test")

    buses = zcanpro.get_buses()
    zcanpro.write_log("Get buses: " + str(buses))
    if len(buses) >= 1:
        test_dev_auto_send(buses[0]["busID"])
#"""








