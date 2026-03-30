import sqlite3
import cantools
from pathlib import Path
from typing import Dict, List, Optional, Union
import hashlib
from translate import baidu_translate
import time
import traceback

class DBCParser:
    def __init__(self, dbc_file_path: str, sqlite_db_path: str):
        """
        初始化DBC解析器
        :param dbc_file_path: DBC文件路径
        :param sqlite_db_path: SQLite数据库路径
        """
        self.dbc_file_path = dbc_file_path
        self.sqlite_db_path = sqlite_db_path
        self.dbc = None
        self.conn = None
        self.cursor = None
        self.translated_messages = {}  # 存储翻译后的消息
        self.translated_signals = {}   # 存储翻译后的信号

    def calculate_file_fingerprint(self):
        """计算文件的指纹（SHA-256 哈希值）"""
        hash_object = hashlib.sha256()
        with open(self.dbc_file_path, 'rb') as file:
            for chunk in iter(lambda: file.read(4096), b""):
                hash_object.update(chunk)
        return hash_object.hexdigest()

    def parse_dbc(self) -> bool:
        """
        解析DBC文件
        :return: 解析成功返回True，失败返回False
        """
        try:
            self.dbc = cantools.database.load_file(self.dbc_file_path)
            return True
        except Exception as e:
            print(f"解析DBC文件失败: {e}")
            return False

    def create_tables(self) -> None:
        """创建SQLite数据库表结构"""
        try:
            self.conn = sqlite3.connect(self.sqlite_db_path)
            self.cursor = self.conn.cursor()

            # 创建消息表
            messages_sql = '''
            CREATE TABLE IF NOT EXISTS dbc_message (
                id INTEGER PRIMARY KEY,
                message_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                length INTEGER NOT NULL,
                cycle_time INTEGER,
                send_type TEXT,
                sender TEXT,
                dbc_comment TEXT,
                user_comment TEXT,
                UNIQUE (message_id),
                dbc_metadata_id INTEGER NOT NULL,

            )
            '''
            print(messages_sql)
            self.cursor.execute(messages_sql)

            # 创建信号表
            signals_sql = '''
            CREATE TABLE IF NOT EXISTS dbc_signal (
                id INTEGER PRIMARY KEY,
                message_id INTEGER NOT NULL,
                name TEXT NOT NULL,
                start_bit INTEGER NOT NULL,
                length INTEGER NOT NULL,
                byte_order TEXT NOT NULL,
                is_signed INTEGER NOT NULL,
                scale REAL NOT NULL,
                offset REAL NOT NULL,
                minimum REAL,
                maximum REAL,
                unit TEXT,
                receiver TEXT,
                dbc_comment TEXT,
                user_comment TEXT,
                FOREIGN KEY (message_id) REFERENCES messages (message_id)
            )
            '''
            print(signals_sql)
            self.cursor.execute(signals_sql)

            # 创建信号值描述表
            signal_value_descriptions_sql = '''
            CREATE TABLE IF NOT EXISTS signal_value_descriptions (
                id INTEGER PRIMARY KEY,
                signal_id INTEGER NOT NULL,
                value INTEGER NOT NULL,
                description TEXT NOT NULL,
                FOREIGN KEY (signal_id) REFERENCES signals (id)
            )
            '''
            print(signal_value_descriptions_sql)
            self.cursor.execute(signal_value_descriptions_sql)

            # 创建节点表
            nodes_sql = '''
            CREATE TABLE IF NOT EXISTS nodes (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                dbc_comment TEXT,
                user_comment TEXT,
                UNIQUE (name)
            )
            '''
            print(nodes_sql)
            self.cursor.execute(nodes_sql)

            # 创建环境变量表
            environment_variables_sql = '''
            CREATE TABLE IF NOT EXISTS environment_variables (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                type TEXT,
                minimum REAL,
                maximum REAL,
                unit TEXT,
                initial_value REAL,
                access_type TEXT,
                dbc_comment TEXT,
                user_comment TEXT,
                UNIQUE (name)
            )
            '''
            print(environment_variables_sql)
            self.cursor.execute(environment_variables_sql)

            # 创建环境变量数据长度表
            env_var_data_lengths_sql = '''
            CREATE TABLE IF NOT EXISTS env_var_data_lengths (
                id INTEGER PRIMARY KEY,
                env_var_id INTEGER NOT NULL,
                data_length INTEGER NOT NULL,
                FOREIGN KEY (env_var_id) REFERENCES environment_variables (id)
            )
            '''
            print(env_var_data_lengths_sql)
            self.cursor.execute(env_var_data_lengths_sql)

            # 创建环境变量接收器表
            env_var_receivers_sql = '''
            CREATE TABLE IF NOT EXISTS env_var_receivers (
                id INTEGER PRIMARY KEY,
                env_var_id INTEGER NOT NULL,
                node_name TEXT NOT NULL,
                FOREIGN KEY (env_var_id) REFERENCES environment_variables (id)
            )
            '''
            print(env_var_receivers_sql)
            self.cursor.execute(env_var_receivers_sql)

            # 创建属性定义表
            attribute_definitions_sql = '''
            CREATE TABLE IF NOT EXISTS attribute_definitions (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                type TEXT NOT NULL,
                default_value TEXT,
                min_value TEXT,
                max_value TEXT,
                UNIQUE (name)
            )
            '''
            print(attribute_definitions_sql)
            self.cursor.execute(attribute_definitions_sql)

            # 创建消息属性表
            message_attributes_sql = '''
            CREATE TABLE IF NOT EXISTS message_attributes (
                id INTEGER PRIMARY KEY,
                message_id INTEGER NOT NULL,
                attribute_name TEXT NOT NULL,
                value TEXT,
                FOREIGN KEY (message_id) REFERENCES messages (message_id),
                FOREIGN KEY (attribute_name) REFERENCES attribute_definitions (name)
            )
            '''
            print(message_attributes_sql)
            self.cursor.execute(message_attributes_sql)

            # 创建信号属性表
            signal_attributes_sql = '''
            CREATE TABLE IF NOT EXISTS signal_attributes (
                id INTEGER PRIMARY KEY,
                signal_id INTEGER NOT NULL,
                attribute_name TEXT NOT NULL,
                value TEXT,
                FOREIGN KEY (signal_id) REFERENCES signals (id),
                FOREIGN KEY (attribute_name) REFERENCES attribute_definitions (name)
            )
            '''
            print(signal_attributes_sql)
            self.cursor.execute(signal_attributes_sql)

            # 创建节点属性表
            node_attributes_sql = '''
            CREATE TABLE IF NOT EXISTS node_attributes (
                id INTEGER PRIMARY KEY,
                node_id INTEGER NOT NULL,
                attribute_name TEXT NOT NULL,
                value TEXT,
                FOREIGN KEY (node_id) REFERENCES nodes (id),
                FOREIGN KEY (attribute_name) REFERENCES attribute_definitions (name)
            )
            '''
            print(node_attributes_sql)
            self.cursor.execute(node_attributes_sql)

            # 创建DBC文件元数据表，添加 file_fingerprint 字段
            dbc_metadata_sql = '''
            CREATE TABLE IF NOT EXISTS dbc_metadata (
                id INTEGER PRIMARY KEY,
                filename TEXT NOT NULL,
                file_fingerprint TEXT NOT NULL,
                version TEXT,
                dbc_comment TEXT,
                import_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            '''
            print(dbc_metadata_sql)
            self.cursor.execute(dbc_metadata_sql)

            self.conn.commit()
        except Exception as e:
            print(f"创建表结构失败: {e}")
            if self.conn:
                self.conn.rollback()

    def insert_messages(self) -> None:
        """插入消息数据到数据库"""
        if not self.dbc or not self.cursor:
            return

        try:
            for message in self.dbc.messages:
                hex_message_id = f"0x{message.frame_id:08X}"  # 将消息ID转换为带0x前缀的十六进制字符串
                self.cursor.execute(
                    '''INSERT INTO dbc_message 
                    (message_id, name, length, cycle_time, send_type, sender, dbc_comment, user_comment)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)''',
                    (
                        hex_message_id,
                        message.name,
                        message.length,
                        message.cycle_time,
                        message.send_type,
                        message.senders[0] if message.senders else '',
                        message.comment if message.comment else '',
                        ''  # 用户备注初始为空
                    )
                )
            self.conn.commit()
        except Exception as e:
            print(f"插入消息数据失败: {e}")
            self.conn.rollback()

    def insert_signals(self) -> None:
        """插入信号数据到数据库"""
        if not self.dbc or not self.cursor:
            return

        try:
            for message in self.dbc.messages:
                hex_message_id = f"0x{message.frame_id:08X}"  # 将消息ID转换为带0x前缀的十六进制字符串
                for signal in message.signals:
                    # 处理start_bit，确保它是整数类型
                    start_bit = signal.start
                    
                    # 处理NamedSignalValue类型
                    if hasattr(start_bit, 'start'):
                        start_bit = start_bit.start
                    elif hasattr(start_bit, 'value'):
                        start_bit = start_bit.value
                    
                    # 尝试转换为整数
                    try:
                        start_bit = int(start_bit)
                    except (ValueError, TypeError) as e:
                        print(f"警告: 无法将start_bit转换为整数: {start_bit}, 类型: {type(start_bit)}, 错误: {e}")
                        start_bit = 0  # 默认值

                    # 处理其他可能的非标准类型
                    minimum = signal.minimum
                    maximum = signal.maximum
                    
                    if minimum is not None:
                        try:
                            minimum = float(minimum)
                        except (ValueError, TypeError) as e:
                            print(f"警告: 无法将minimum转换为浮点数: {minimum}, 类型: {type(minimum)}, 错误: {e}")
                            minimum = 0.0
                    
                    if maximum is not None:
                        try:
                            maximum = float(maximum)
                        except (ValueError, TypeError) as e:
                            print(f"警告: 无法将maximum转换为浮点数: {maximum}, 类型: {type(maximum)}, 错误: {e}")
                            maximum = 0.0

                    # 确保scale和offset是浮点数
                    scale = float(signal.scale) if signal.scale is not None else 0.0
                    offset = float(signal.offset) if signal.offset is not None else 0.0

                    # 处理其他参数
                    byte_order = str(signal.byte_order) if signal.byte_order is not None else ''
                    is_signed = 1 if signal.is_signed else 0
                    unit = str(signal.unit) if signal.unit is not None else ''
                    receiver = str(signal.receivers[0]) if signal.receivers else ''
                    dbc_comment = str(signal.comment) if signal.comment is not None else ''

                    # 打印调试信息
                    print(f"插入信号: {signal.name}, start_bit: {start_bit}, 类型: {type(start_bit)}")
                    
                    # 构建参数列表
                    params = (
                        hex_message_id,
                        signal.name,
                        start_bit,
                        signal.length,
                        byte_order,
                        is_signed,
                        scale,
                        offset,
                        minimum,
                        maximum,
                        unit,
                        receiver,
                        dbc_comment,
                        ''  # 用户备注初始为空
                    )
                    
                    # 打印参数类型信息
                    print(f"参数类型: {[type(p) for p in params]}")
                    
                    self.cursor.execute(
                        '''INSERT INTO dbc_signal 
                        (message_id, name, start_bit, length, byte_order, is_signed, 
                        scale, offset, minimum, maximum, unit, receiver, dbc_comment, user_comment)
                        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)''',
                        params
                    )
                    signal_id = self.cursor.lastrowid

                    # 插入信号值描述
                    if signal.choices:
                        for value, description in signal.choices.items():
                            print("类型值",type(value),type(description))
                            description_str = str(description) if description is not None else ''
                            self.cursor.execute(
                                '''INSERT INTO signal_value_descriptions 
                                (signal_id, value, description) VALUES (?, ?, ?)''',
                                (signal_id, value, description_str)
                            )
            self.conn.commit()
        except Exception as e:
            print(f"插入信号数据失败: {e}")
            # 打印更多错误上下文信息
            import traceback
            traceback.print_exc()
            # 打印最后处理的信号信息
            if 'signal' in locals():
                print(f"错误发生在信号: {signal.name}")
            self.conn.rollback()

    def insert_nodes(self) -> None:
        """插入节点数据到数据库"""
        if not self.dbc or not self.cursor:
            return

        try:
            for node in self.dbc.nodes:
                self.cursor.execute(
                    '''INSERT INTO nodes (name, dbc_comment, user_comment) VALUES (?, ?, ?)''',
                    (node.name, node.comment if node.comment else '', '')
                )
            self.conn.commit()
        except Exception as e:
            print(f"插入节点数据失败: {e}")
            self.conn.rollback()

    def insert_environment_variables(self) -> None:
        """插入环境变量数据到数据库"""
        if not hasattr(self.dbc, 'environment_variables') or not self.cursor:
            return

        try:
            for env_var in self.dbc.environment_variables:
                self.cursor.execute(
                    '''INSERT INTO environment_variables 
                    (name, type, minimum, maximum, unit, initial_value, access_type, dbc_comment, user_comment)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)''',
                    (
                        env_var.name,
                        env_var.type,
                        env_var.minimum,
                        env_var.maximum,
                        env_var.unit,
                        env_var.initial_value,
                        env_var.access_type,
                        env_var.comment if env_var.comment else '',
                        ''
                    )
                )
                env_var_id = self.cursor.lastrowid

                # 插入环境变量数据长度
                if env_var.data_length is not None:
                    self.cursor.execute(
                        '''INSERT INTO env_var_data_lengths (env_var_id, data_length) VALUES (?, ?)''',
                        (env_var_id, env_var.data_length)
                    )

                # 插入环境变量接收器
                for receiver in env_var.receivers:
                    self.cursor.execute(
                        '''INSERT INTO env_var_receivers (env_var_id, node_name) VALUES (?, ?)''',
                        (env_var_id, receiver)
                    )
            self.conn.commit()
        except Exception as e:
            print(f"插入环境变量数据失败: {e}")
            self.conn.rollback()

    def insert_attribute_definitions(self) -> None:
        """插入属性定义数据到数据库"""
        if not hasattr(self.dbc, 'attribute_definitions') or not self.cursor:
            return

        try:
            for attr_def in self.dbc.attribute_definitions:
                self.cursor.execute(
                    '''INSERT INTO attribute_definitions 
                    (name, type, default_value, min_value, max_value)
                    VALUES (?, ?, ?, ?, ?)''',
                    (
                        attr_def.name,
                        attr_def.type,
                        attr_def.default_value,
                        attr_def.minimum,
                        attr_def.maximum
                    )
                )
            self.conn.commit()
        except Exception as e:
            print(f"插入属性定义数据失败: {e}")
            self.conn.rollback()

    def insert_attributes(self) -> None:
        """插入属性数据到数据库"""
        if not hasattr(self.dbc, 'attributes') or not self.cursor:
            return

        try:
            # 处理消息属性
            for message_id, attributes in self.dbc.attributes.get('messages', {}).items():
                hex_message_id = f"0x{message_id:08X}"  # 将消息ID转换为带0x前缀的十六进制字符串
                for attr_name, attr_value in attributes.items():
                    self.cursor.execute(
                        '''INSERT INTO message_attributes 
                        (message_id, attribute_name, value) VALUES (?, ?, ?)''',
                        (hex_message_id, attr_name, attr_value)
                    )

            # 处理信号属性
            for message_id, signals in self.dbc.attributes.get('signals', {}).items():
                hex_message_id = f"0x{message_id:08X}"  # 将消息ID转换为带0x前缀的十六进制字符串
                for signal_name, attributes in signals.items():
                    # 获取信号ID
                    self.cursor.execute(
                        '''SELECT id FROM dbc_signal WHERE message_id = ? AND name = ?''',
                        (hex_message_id, signal_name)
                    )
                    signal = self.cursor.fetchone()
                    if signal:
                        signal_id = signal[0]
                        for attr_name, attr_value in attributes.items():
                            self.cursor.execute(
                                '''INSERT INTO signal_attributes 
                                (signal_id, attribute_name, value) VALUES (?, ?, ?)''',
                                (signal_id, attr_name, attr_value)
                            )

            # 处理节点属性
            for node_name, attributes in self.dbc.attributes.get('nodes', {}).items():
                # 获取节点ID
                self.cursor.execute('''SELECT id FROM nodes WHERE name = ?''', (node_name,))
                node = self.cursor.fetchone()
                if node:
                    node_id = node[0]
                    for attr_name, attr_value in attributes.items():
                        self.cursor.execute(
                            '''INSERT INTO node_attributes 
                            (node_id, attribute_name, value) VALUES (?, ?, ?)''',
                            (node_id, attr_name, attr_value)
                        )

            self.conn.commit()
        except Exception as e:
            print(f"插入属性数据失败: {e}")
            self.conn.rollback()

    def insert_metadata(self) -> None:
        """插入DBC文件元数据"""
        if not self.cursor:
            return

        try:
            dbc_path = Path(self.dbc_file_path)
            file_fingerprint = self.calculate_file_fingerprint()
            self.cursor.execute(
                '''INSERT INTO dbc_metadata (filename, file_fingerprint, version, dbc_comment) VALUES (?, ?, ?, ?)''',
                (
                    dbc_path.name,
                    file_fingerprint,
                    self.dbc.version if hasattr(self.dbc, 'version') else '',
                    self.dbc.comment if hasattr(self.dbc, 'comment') else ''
                )
            )
            self.conn.commit()
        except Exception as e:
            print(f"插入元数据失败: {e}")
            self.conn.rollback()

    def close(self) -> None:
        """关闭数据库连接"""
        if self.conn:
            self.conn.close()
            self.conn = None
            self.cursor = None

    def parse_and_store(self) -> bool:
        """
        解析DBC文件并存储到SQLite数据库
        :return: 操作成功返回True，失败返回False
        """
        if not self.parse_dbc():
            return False

        self.create_tables()
        self.insert_messages()
        self.insert_signals()
        self.insert_nodes()
        self.insert_environment_variables()
        self.insert_attribute_definitions()
        self.insert_attributes()
        self.insert_metadata()
        self.close()
        return True

    def translate_dbc_comments(self) -> Dict[str, Dict[str, str]]:
        """
        从数据库读取已解析数据进行翻译，并将结果存入user_comment字段
        已存在user_comment的记录将跳过翻译
        :return: 包含翻译结果的字典
        """
        if not self.conn:
            try:
                self.conn = sqlite3.connect(self.sqlite_db_path)
                self.cursor = self.conn.cursor()
            except Exception as e:
                print(f"无法连接到数据库: {e}")
                return {"messages": {}, "signals": {}}
        
        translated_data = {"messages": {}, "signals": {}}
        
        # 翻译消息注释
        try:
            # 查询所有没有user_comment的消息记录
            self.cursor.execute('''
                SELECT id, name, dbc_comment, user_comment 
                FROM dbc_message 
                WHERE user_comment IS NULL OR user_comment = ''
            ''')
            messages = self.cursor.fetchall()
            
            print(f"找到 {len(messages)} 条需要翻译的消息记录")

            counter_m = 0
            
            for msg_id, name, dbc_comment, user_comment in messages:
                if dbc_comment:  # 只翻译有dbc_comment的记录
                    try:
                        time.sleep(1)  # 添加请求间隔
                        translated_comment = baidu_translate(dbc_comment)
                        
                        # 更新数据库中的user_comment字段
                        self.cursor.execute('''
                            UPDATE dbc_message 
                            SET user_comment = ? 
                            WHERE id = ?
                        ''', (" ".join(translated_comment), msg_id))
                        
                        self.conn.commit()
                        translated_data["messages"][name] = translated_comment
                        counter_m = counter_m + 1
                        print(f"已翻译消息 {name} , {counter_m}/{len(messages)}")
                    except Exception as e:
                        print(f"翻译消息 {name} 失败: {e}")
                        traceback.print_exc()
                        self.conn.rollback()
        except Exception as e:
            print(f"处理消息翻译时出错: {e}")
        
        # 翻译信号注释
        try:
            # 查询所有没有user_comment的信号记录
            self.cursor.execute('''
                SELECT s.id, m.name as message_name, s.name as signal_name, s.dbc_comment, s.user_comment 
                FROM dbc_signal s
                JOIN dbc_message m ON s.message_id = m.message_id
                WHERE s.user_comment IS NULL OR s.user_comment = '' and s.dbc_comment !=''
            ''')
            signals = self.cursor.fetchall()
            
            print(f"找到 {len(signals)} 条需要翻译的信号记录")

            counter_s = 0
            
            for signal_id, message_name, signal_name, dbc_comment, user_comment in signals:
                if dbc_comment:  # 只翻译有dbc_comment的记录
                    try:
                        time.sleep(1)  # 添加请求间隔
                        signal_key = f"{message_name}_{signal_name}"
                        translated_comment = baidu_translate(dbc_comment)
                        
                        # 更新数据库中的user_comment字段
                        self.cursor.execute('''
                            UPDATE signals 
                            SET user_comment = ? 
                            WHERE id = ?
                        ''', (" ".join(translated_comment), signal_id))
                        
                        self.conn.commit()
                        translated_data["signals"][signal_key] = translated_comment
                        counter_s = counter_s + 1
                        print(f"已翻译信号 {message_name}.{signal_name}, {counter_s}/{len(signals)}")
                    except Exception as e:
                        print(f"翻译信号 {message_name}.{signal_name} 失败: {e}")
                        self.conn.rollback()
        except Exception as e:
            print(f"处理信号翻译时出错: {e}")
        
        # 关闭数据库连接
        if self.conn:
            self.conn.close()
            self.conn = None
            self.cursor = None
        
        print(f"翻译完成: {len(translated_data['messages'])} 条消息和 {len(translated_data['signals'])} 条信号")
        return translated_data

if __name__ == "__main__":
    # 使用示例
    dbc_file_path = r"D:\CanRecorder\out\dbcs\MEB_CN_E3V_KCAN_KMatrix_V10.07F_20200619_YF.dbc" # 替换为实际的 DBC 文件路径
    sqlite_db_path = 'dbc1.db'  # 替换为实际的 SQLite 数据库路径
    parser = DBCParser(dbc_file_path, sqlite_db_path)
    # translations = parser.translate_dbc_comments()
    # print(f"已翻译 {len(translations['messages'])} 条消息和 {len(translations['signals'])} 条信号")
        
    
    if parser.parse_and_store():
        print("DBC文件解析并存储成功")
    else:
        print("DBC文件解析并存储失败")