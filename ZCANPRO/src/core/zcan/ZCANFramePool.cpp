#include "ZCANFramePool.h"

namespace ZCAN {

ZCANFramePool::ZCANFramePool(QObject *parent)
    : QObject(parent)
{
    // 预分配帧对象池
    m_pool.resize(FRAME_POOL_SIZE);
    
    // 初始化空闲列表
    for (int i = 0; i < FRAME_POOL_SIZE; ++i) {
        m_freeList.enqueue(i);
    }
}

ZCANFramePool::~ZCANFramePool()
{
}

CANFrame* ZCANFramePool::allocate()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_freeList.isEmpty()) {
        emit poolExhausted();
        return nullptr;
    }
    
    int index = m_freeList.dequeue();
    return &m_pool[index];
}

QVector<CANFrame*> ZCANFramePool::allocateBatch(int count)
{
    QMutexLocker locker(&m_mutex);
    
    QVector<CANFrame*> frames;
    frames.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        if (m_freeList.isEmpty()) {
            emit poolExhausted();
            break;
        }
        
        int index = m_freeList.dequeue();
        frames.append(&m_pool[index]);
    }
    
    return frames;
}

void ZCANFramePool::release(CANFrame* frame)
{
    if (!frame) return;
    
    QMutexLocker locker(&m_mutex);
    
    // 计算索引
    int index = frame - m_pool.data();
    
    if (isValidIndex(index)) {
        m_freeList.enqueue(index);
    }
}

void ZCANFramePool::releaseBatch(const QVector<CANFrame*>& frames)
{
    QMutexLocker locker(&m_mutex);
    
    for (CANFrame* frame : frames) {
        if (!frame) continue;
        
        int index = frame - m_pool.data();
        
        if (isValidIndex(index)) {
            m_freeList.enqueue(index);
        }
    }
}

int ZCANFramePool::availableCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_freeList.size();
}

int ZCANFramePool::usedCount() const
{
    QMutexLocker locker(&m_mutex);
    return FRAME_POOL_SIZE - m_freeList.size();
}

void ZCANFramePool::reset()
{
    QMutexLocker locker(&m_mutex);
    
    m_freeList.clear();
    for (int i = 0; i < FRAME_POOL_SIZE; ++i) {
        m_freeList.enqueue(i);
    }
}

bool ZCANFramePool::isValidIndex(int index) const
{
    return index >= 0 && index < FRAME_POOL_SIZE;
}

} // namespace ZCAN
