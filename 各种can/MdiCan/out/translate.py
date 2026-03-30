import requests
import random
import hashlib
import json

def baidu_translate(query, from_lang='auto', to_lang='zh',proxy=''):
    # 百度翻译 API 的基本信息
    appid = '20210517000830802'  # 替换为你的 APP ID
    secret_key = '6Y2jfm1B0i_4w3GUaRwG'  # 替换为你的密钥
    api_url = 'https://fanyi-api.baidu.com/api/trans/vip/translate'

    # 生成随机数
    salt = random.randint(32768, 65536)

    # 拼接签名
    sign = appid + query + str(salt) + secret_key
    sign = hashlib.md5(sign.encode()).hexdigest()

    # 构建请求参数
    params = {
        'q': query,
        'from': from_lang,
        'to': to_lang,
        'appid': appid,
        'salt': salt,
        'sign': sign
    }

    try:
        # 发送请求
        if proxy:
            # 设置代理
            proxies = {
                'http': proxy,
                'https': proxy
            }
            print("正在翻译：",query)
            # 发送请求并使用代理
            response = requests.get(api_url, params=params, proxies=proxies)
        else:
            # 不使用代理发送请求
            response = requests.get(api_url, params=params)


        # 解析响应
        result = response.json()

        # 检查是否有错误
        if 'error_code' in result:
            print(f"翻译出错，错误码: {result['error_code']}，错误信息: {result['error_msg']}")
            return None

        # 提取翻译结果
        translations = [trans['dst'] for trans in result['trans_result']]
        return translations

    except requests.RequestException as e:
        print(f"请求出错: {e}")
        return None
    except (KeyError, json.JSONDecodeError):
        print("解析响应出错")
        return None

# 示例调用
# query_text = "Nur die Kombination des statischen und toggelnden Signals ergibt den Blinktakt für die Hell & Dunkelphase"
# translated_text = baidu_translate(query_text)
# if translated_text:
#     print("翻译结果:")
#     for text in translated_text:
#         print(text)