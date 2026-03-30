import can
import json
import os
import sys
import io

def parse_blf_file(file_path):
    messages = []

    with can.BLFReader(file_path) as log:
        for msg in log:
             # 将 arbitration_id 格式化为 0x 开头的 8 位十六进制字符串
            can_id_hex = hex(msg.arbitration_id).replace("0x", "")
            can_message = {
                "direction": "未知",  # 方向信息需要根据具体情况确定
                "canId": can_id_hex,
                "data": ''.join([f"{byte:02X}" for byte in msg.data]),
                "time": str(msg.timestamp),
                "indexCounter": None,  # 索引计数器需要根据具体情况确定
                "projectId": None  # 项目ID需要根据具体情况确定
            }
            messages.append(can_message)
    return messages

if __name__ == "__main__":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    if len(sys.argv)>1:
        file_path = sys.argv[1]
    # file_path = r"C:\Users\jingq\Downloads\Logging001.blf"  # 替换为实际的BLF文件路径
    try:
        file_name = os.path.basename(file_path)
        base = os.getcwd() + "\\blf_json" 
        if not os.path.exists(base):
            os.makedirs(base, exist_ok=True)
        parsed_messages = parse_blf_file(file_path)
        result_file_path = base + "\\" + "json_for_" + file_name + ".json"
        with open(result_file_path, "w") as f:
            json.dump(parsed_messages, f, indent=4)
        print("##SUCCESS##",result_file_path)
    except Exception as e:
        print(e)
