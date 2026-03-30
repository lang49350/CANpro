#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PDF文本提取工具
用于提取PDF文件中的文本内容
"""

import sys
import os

try:
    import PyPDF2
except ImportError:
    print("需要安装PyPDF2库: pip install PyPDF2")
    sys.exit(1)

def extract_pdf_text(pdf_path):
    """提取PDF文件的文本内容"""
    try:
        with open(pdf_path, 'rb') as file:
            pdf_reader = PyPDF2.PdfReader(file)
            text = ""
            
            print(f"PDF文件: {pdf_path}")
            print(f"总页数: {len(pdf_reader.pages)}")
            print("=" * 80)
            
            for page_num, page in enumerate(pdf_reader.pages, 1):
                page_text = page.extract_text()
                text += f"\n\n{'='*80}\n第 {page_num} 页\n{'='*80}\n\n"
                text += page_text
                
            return text
    except Exception as e:
        print(f"错误: {e}")
        return None

def main():
    # 获取当前目录下的所有PDF文件
    current_dir = os.path.dirname(os.path.abspath(__file__))
    pdf_files = [f for f in os.listdir(current_dir) if f.endswith('.pdf')]
    
    if not pdf_files:
        print("当前目录没有找到PDF文件")
        return
    
    print(f"找到 {len(pdf_files)} 个PDF文件:")
    for i, pdf_file in enumerate(pdf_files, 1):
        print(f"{i}. {pdf_file}")
    
    # 提取所有PDF
    for pdf_file in pdf_files:
        pdf_path = os.path.join(current_dir, pdf_file)
        output_path = os.path.join(current_dir, pdf_file.replace('.pdf', '.txt'))
        
        print(f"\n正在处理: {pdf_file}")
        text = extract_pdf_text(pdf_path)
        
        if text:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(text)
            print(f"✓ 已保存到: {output_path}")

if __name__ == "__main__":
    main()
