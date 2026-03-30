
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

""" **************** 本示例说明 ****************

采用CRC8计算校验码
校验码填充到数据序列的第一个字节位置；校验内容为传入数据的第二个字节至结尾。
循环计数器大小为一个字节，位于第二个字节，循环计数范围[100,199]

如：
rawData: [B1        B2 B3 B4 ... Bn]
outData: [CheckSum  B2 B3 B4 ... Bn]

"""

from crccheck.crc import Crc8

def E2E_Calculate(canID, dataID, counter, rawData):
    if len(rawData) < 2:
        return [], 0

    outData = list(rawData)

    minCounter = 100
    maxCounter = 199
    outData[1] = counter % (maxCounter - minCounter + 1) + minCounter

    nextCounter = outData[1] + 1

    checksum = Crc8.calc(outData[1:])

    outData[0] = checksum & 0xFF
    return outData, nextCounter


if __name__ == "__main__":
    print(E2E_Calculate(0, 0, 1, [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07]))
