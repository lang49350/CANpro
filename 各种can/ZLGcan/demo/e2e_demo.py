
""" **************** E2E安全算法库脚本文件编写说明 ****************

脚本中必须提供以下接口供ZCANPRO程序调用。

1. outData, nextCounter = E2E_Calculate(canID, dataID, counter, rawData)
    按传入的数据进行运算，返回计算后的数据序列
    * canID: CAN报文ID，32位无符号整数，最高bit位为1时表示扩展帧
    * dataID: 报文密钥，32位无符号整数
    * counter: 计数器，32位无符号整数
    * rawData: 原始报文数据数组，如 [0x00, 0x34, 0x00, 0x01, 0xFF, 0xF2, 0x10, 0x83]
    * outData: 返回计算后的数据数组， 如 [0x1E, 0x34, 0x00, 0x01, 0xFF, 0xF2, 0x10, 0x83]
    * nextCounter: 返回下一次的计数器值，32位无符号整数

"""

def E2E_Calculate(canID, dataID, counter, rawData):
    if len(rawData) < 2:
        return []

    outData = list(rawData)

    # 比如循环计数器的大小为8bit，位于第二个字节，其循环计数范围为100~199
    minCounter = 100
    maxCounter = 199
    outData[1] = counter % (maxCounter - minCounter + 1) + minCounter

    nextCounter = outData[1] + 1

    # 计算校验和
    checksum = dataID
    for index in range(1, len(outData)):
        checksum += outData[index]
    outData[0] = checksum & 0xFF

    return outData, nextCounter





