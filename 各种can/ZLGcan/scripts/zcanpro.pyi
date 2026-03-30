from typing import List, Dict, Tuple

def get_buses() -> List[Dict]:
    """
    获取ZCANPRO程序已经启动的总线信息(即打开的设备CAN通道)。
        
    返回：
    * buses: 为总线信息列表，如
    [
        {
            "busID": 0x101,     # 总线ID
            "devType": 1,       # 设备类型
            "devIndex": 0,      # 设备索引号
            "chnIndex": 0       # 通道索引号
        },
        ...
    ]
    """
    pass

def receive(busID: int) -> Tuple[int, List[Dict]]:
    """
    获取指定总线的数据。
    
    参数：
    * busID: 指定总线ID, 整数类型
    
    返回：
    * result: 返回执行结果，整数类型，0-失败, 1-成功
    * frms: 返回获取的数据列表， 如
    [
        {
            "can_id": 0x101,            # 帧ID, 32位整数, 具体含义见下方说明
            "is_canfd": 1,              # 是否为CANFD数据, 0-CAN, 1-CANFD, 整数类型
            "canfd_brs": 1,             # CANFD加速, 0-不加速, 1-加速, 整数类型
            "data": [0, 1, 2, 3, 4],    # 数据
            "timestamp_us": 666666      # 时间戳, 微妙, 整数类型
        },
        ...
    ]
    
    帧ID说明：
    帧ID为32位整数，高三位为帧标识，标准帧使用低11位ID值，扩展帧使用低29位ID值。
    * bit 0-28	: CAN identifier (11/29 bit)
    * bit 29	: error message frame flag (0 = data frame, 1 = error message)
    * bit 30	: remote transmission request flag (1 = rtr frame)
    * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
    """
    pass

def transmit(busID: int, frms: List[Dict]) -> int:
    """
    发送数据至指定总线。

    参数：
    * busID: 指定总线ID, 整数类型
    * frms: 指定数据列表，同zcanpro.receive

    返回：
    * result: 返回执行结果, 整数类型，0-失败, 1-成功
    """
    pass

def write_log(msg: str) -> None:
    """
    显示日志信息，ZCANPRO程序会在界面上显示该信息，便于跟踪脚本运行过程。
    
    参数：
    * msg: 日志信息，字符串类型
    """
    pass

def uds_init(udsCfg: Dict) -> None:
    """
    初始化UDS诊断服务。

    参数：
    * udsCfg: UDS服务配置参数, 字典类型, 如
    {
        "response_timeout_ms": 3000,    # 响应超时时间(ms)
        "use_canfd": 0,                 # 是否使用CANFD, 0-CAN, 1-CANFD
        "canfd_brs": 0,                 # CANFD加速(使用CANFD时有效), 0-不加速, 1-加速
        "trans_ver": 0,                 # 传输协议版本, 0-ISO15765_2 2004格式, 1-ISO15765_2 2016新增格式
        "fill_byte": 0x00,              # 填充字节，如使用CAN发送时，数据不足8字节，将填充至8字节进行发送
        "frame_type": 0,                # 帧类型，0-标准帧，1-扩展帧
        "trans_stmin_valid": 0,         # 是否设置多帧发送的STmin，替代ECU流控返回的STmin
        "trans_stmin": 0,               # 多帧发送的STmin最小帧间隔时间(ms)，范围 [0, 127]
        "enhanced_timeout_ms": 5000,    # 当消极响应值为0x78时延长的超时时间(ms)
        "fc_timeout_ms": 1000,          # 接收流控超时时间(ms), 如发送首帧后需要等待回应流控帧, 范围 [20, 10000]
        "fill_mode": 0,                 # 数据长度填充模式, 0-不填充；1-小于8字节填充至8字节，大于8字节时按DLC就近填充；2-填充至最大数据长度 (不建议)
    """
    pass

def uds_request(busID: int, req: Dict) -> Dict:
    """
    请求UDS服务，返回响应数据。

    参数：
    * busID: 指定总线ID, 整数类型
    * req: 请求数据，字典类型，如
    {
        "src_addr": 0x700,              # 源地址
        "dst_addr": 0x701,              # 目标地址
        "suppress_response" :0,         # 是否抑制响应, 0-不抑制, 1-抑制
        "sid": 0x19,                    # UDS服务号
        "data":[0x02, 0xFF]             # UDS服务数据
    }

    返回：
    * response: 返回的响应结果，字典类型，如
    {
        "result": 1,                    # 响应状态, 0-失败, 1-成功
        "result_msg": "ok",             # 响应结果字符串，如果响应失败，指示失败原因
        "data":[0x59, 0x02, 0x1D, 0xAE] # 响应数据，响应成功时为积极响应或者消极响应数据
    }
    """
    pass

def uds_deinit() -> None:
    """
    关闭UDS服务，释放资源。
    """
    pass

def dev_auto_send_start(busID: int, frms: List[Dict]) -> int:
    """
    启动设备定时发送
    每种设备类型支持的定时发送帧数量有限，请参考zlgcan使用手册，如USBCANFD仅支持每通道100条。

    参数：
    * busID: 指定总线ID, 整数类型
    * frms: 指定定时发送的帧条目， 如
    [
        {
            "can_id": 0x101,            # 帧ID, 32位整数
            "is_canfd": 0,              # 是否为CANFD数据, 0-CAN, 1-CANFD, 整数类型
            "canfd_brs": 0,             # CANFD加速, 0-不加速, 1-加速, 整数类型
            "data": [0, 1, 2, 3, 4],    # 数据
            "interval_ms": 10,          # 本帧数据定时发送间隔, 毫秒, 整数类型
            "delay_start_ms": 0         # 延时启动，毫秒, 整数类型; 部分设备不支持
        },
        ...
    ]

    返回：
    * result: 返回执行结果，整数类型，0-失败, 1-成功
    """
    pass

def dev_auto_send_stop(busID: int) -> int:
    """
    停止设备所有定时发送

    参数：
    * busID: 指定总线ID, 整数类型

    返回：
    * result: 返回执行结果, 整数类型，0-失败, 1-成功
    """
    pass

def dev_auto_send_enable_frame(busID: int, enable: int, frmIndex: int, frm: Dict) -> int:
    """
    单独启动或停止指定索引帧的定时发送
    只有在调用dev_auto_send_start后使用才有效

    参数：
    * busID: 指定总线ID, 整数类型
    * enable: 使能或者禁用此帧的定时发送, 整数类型, 0-禁用发送, 1-使能发送
    * frmIndex: 定时发送帧的索引号, 可以为dev_auto_send_start接口参数frms中的帧索引, 也可以为新增的帧序号;
    * frm: 帧数据，同dev_auto_send_start接口参数frms中的帧定义

    返回：
    * result: 返回执行结果, 整数类型，0-失败, 1-成功
    """
    pass


