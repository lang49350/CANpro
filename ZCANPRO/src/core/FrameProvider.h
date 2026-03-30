#ifndef FRAMEPROVIDER_H
#define FRAMEPROVIDER_H

#include "CANFrame.h"
#include <QVector>
#include <QHash>

/**
 * @brief 帧数据提供者接口
 */
class IFrameProvider
{
public:
    virtual ~IFrameProvider() = default;
    virtual bool hasNext() const = 0;
    virtual bool next(CANFrame &frame) = 0;
    virtual void reset() = 0;
    virtual int totalFrames() const = 0;
    virtual int currentIndex() const = 0;
};

/**
 * @brief 基于内存的帧提供者
 */
class MemoryFrameProvider : public IFrameProvider
{
public:
    // 修改构造函数，接收 filters
    explicit MemoryFrameProvider(const QVector<CANFrame> &frames, const QHash<uint32_t, bool> &filters = QHash<uint32_t, bool>())
        : m_frames(frames)
        , m_filters(filters)
        , m_currentIndex(0)
        , m_channelFilter(-1) // 默认不限制通道
    {}

    bool hasNext() const override {
        // 需要向前查找直到找到一个不过滤的帧，或者到达末尾
        int tempIndex = m_currentIndex;
        while (tempIndex < m_frames.size()) {
            if (isPass(m_frames[tempIndex])) {
                return true;
            }
            tempIndex++;
        }
        return false;
    }

    bool next(CANFrame &frame) override {
        while (m_currentIndex < m_frames.size()) {
            const CANFrame &current = m_frames[m_currentIndex];
            if (isPass(current)) {
                frame = current;
                m_currentIndex++;
                return true;
            }
            m_currentIndex++;
        }
        return false;
    }

    void reset() override {
        m_currentIndex = 0;
    }

    int totalFrames() const override {
        return m_frames.size();
    }

    int currentIndex() const override {
        return m_currentIndex;
    }
    
    void setFilters(const QHash<uint32_t, bool> &filters) {
        m_filters = filters;
    }

    void setChannelFilter(int channel) {
        m_channelFilter = channel;
    }

private:
    bool isPass(const CANFrame &frame) const {
        // 1. 检查通道过滤
        if (m_channelFilter != -1 && frame.channel != m_channelFilter) {
            return false;
        }
        
        // 2. 检查ID过滤
        // 如果过滤器为空，默认通过
        return m_filters.value(frame.id, true);
    }

    QVector<CANFrame> m_frames;
    QHash<uint32_t, bool> m_filters;
    mutable int m_currentIndex;
    int m_channelFilter;
};

#endif // FRAMEPROVIDER_H
