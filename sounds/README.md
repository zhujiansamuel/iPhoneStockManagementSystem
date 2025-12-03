# 音频文件说明

本目录存放操作提醒的音频文件。请准备以下 WAV 格式音频文件：

## 需要的音频文件

### 1. success.wav - 入库成功
- **使用场景**: 正常入库和假登录入库成功时
- **建议内容**: "登録完了" 或 "記録完了"
- **建议时长**: 1-2秒

### 2. jan_error.wav - JAN code 输入错误
- **使用场景**: 13位 JAN code 格式错误时
- **建议内容**: "JANコード入力エラー" 或 "13桁を入力してください"
- **建议时长**: 1-2秒

### 3. jan_not_found.wav - JAN code 未找到
- **使用场景**: 13位 JAN code 不在登录的商品中
- **建议内容**: "商品が見つかりません" 或 "未登録の商品です"
- **建议时长**: 1-2秒

### 4. imei_error.wav - IMEI 输入错误
- **使用场景**: 15位 IMEI 格式错误时
- **建议内容**: "IMEI入力エラー" 或 "15桁を入力してください"
- **建议时长**: 1-2秒

### 5. imei_duplicate.wav - IMEI 重复
- **使用场景**: 15位 IMEI 已存在（重复）时
- **建议内容**: "IMEI重複" 或 "既に登録されています"
- **建议时长**: 1-2秒

### 6. count_reset.wav - 计数器清零
- **使用场景**: lcdNumber_2 达到 10 自动清零时
- **建议内容**: "10個完了" 或 "カウントリセット"
- **建议时长**: 1-2秒

## 音频文件要求

- **格式**: WAV (推荐 PCM 格式)
- **采样率**: 44100Hz 或 48000Hz
- **声道**: 单声道或立体声
- **位深度**: 16-bit
- **音量**: 适中，不要过大或过小

## 生成音频文件的方法

### 方法 1: 使用在线文本转语音（TTS）服务
- Google Text-to-Speech: https://cloud.google.com/text-to-speech
- Microsoft Azure Speech: https://azure.microsoft.com/zh-cn/products/ai-services/text-to-speech
- Amazon Polly: https://aws.amazon.com/polly/

### 方法 2: 使用本地 TTS 软件
- **Windows**: Microsoft Speech Platform
- **macOS**: say 命令
  ```bash
  say -v Kyoko "記録完了" -o success.wav
  ```
- **日语语音**: 选择日语语音引擎

### 方法 3: 录音
使用录音软件录制真人语音，然后转换为 WAV 格式

### 方法 4: 使用音效库
从免费音效网站下载合适的提示音：
- FreeSound.org
- Zapsplat.com
- Notification Sounds

## 文件放置

将所有 6 个 WAV 文件放在本目录（sounds/）下：
```
sounds/
├── success.wav
├── jan_error.wav
├── jan_not_found.wav
├── imei_error.wav
├── imei_duplicate.wav
└── count_reset.wav
```

## 启用音频功能

音频文件准备好后，需要修改 `CMakeLists.txt` 文件以启用音频功能：

1. 打开项目根目录下的 `CMakeLists.txt` 文件
2. 找到音频资源部分（带有注释的部分，大约在第 71-84 行）
3. 取消注释音频资源的 qt_add_resources 块：

```cmake
# 将这部分的注释去掉
qt_add_resources(${PROJECT_NAME} "sound-resources"
    PREFIX "/sounds"
    BASE "sounds"
    FILES
        sounds/success.wav
        sounds/jan_error.wav
        sounds/jan_not_found.wav
        sounds/imei_error.wav
        sounds/imei_duplicate.wav
        sounds/count_reset.wav
)
```

4. 保存文件并重新编译项目

## 注意事项

1. 文件名必须严格匹配上述名称（区分大小写）
2. 如果音频资源在 CMakeLists.txt 中被注释掉，程序会静默跳过音频播放，不会影响正常功能
3. 音频文件会被编译到程序中，不需要单独分发
4. 可以随时替换音频文件，重新编译即可生效
5. **必须先添加音频文件，再取消 CMakeLists.txt 中的注释，否则编译会失败**

## 测试音频

编译运行程序后，进行相应操作即可听到音频提示。
