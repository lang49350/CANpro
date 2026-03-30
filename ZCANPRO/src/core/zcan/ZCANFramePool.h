#ifndef ZCAN_FRAME_POOL_H
#define ZCAN_FRAME_POOL_H

#include <QObject>
#include <QVector>
#include <QQueue>
#include <QMutex>
#include "../CANFrame.h"
#include "zcan_config.h"

/**
 * @file ZCANFramePool.h
 * @brief ZCAN帧内存池
 * 
 * 预分配CANFrame对象池，提供O(1)分配和释放：
 * - 预分配10000个CANFrame对象
 * - 使用空闲列表管理
 * - 支持批量分配
 * - 线程安全
 */

namespace ZCAN {

class ZCANFramePool : public QObject
{
    Q_OBJECT
    
public:
    explicit ZCANFramePool(QObject *parent = nullptr);
    ~ZCANFramePool();
    
    /**
     * @brief 分配一个帧对象
     * @return 帧对象指针，如果池已满返回nullptr
     */
    CANFrame* allocate();
    
    /**
     * @brief 批量分配帧对象
     * @param count 需要分配的数量
     * @return 帧对象指针列表
     */
    QVector<CANFrame*> allocateBatch(int count);
    
    /**
     * @brief 释放一个帧对象
     * @param frame 要释放的帧对象指针
     */
    void release(CANFrame* frame);
    
    /**
     * @brief 批量释放帧对象
     * @param frames 要释放的帧对象指针列表
     */
    void releaseBatch(const QVector<CANFrame*>& frames);
    
    /**
     * @brief 获取池大小
     * @return 总池大小
     */
    int poolSize() const { return FRAME_POOL_SIZE; }
    
    /**
     * @brief 获取可用数量
     * @return 当前可用的帧对象数量
     */
    int availableCount() const;
    
    /**
     * @brief 获取已使用数量
     * @return 当前已分配的帧对象数量
     */
    int usedCount() const;
    
    /**
     * @brief 重置内存池
     */
    void reset();
    
signals:
    /**
     * @brief 内存池耗尽信号
     */
    void poolExhausted();
    
private:
    QVector<CANFrame> m_pool;        ///< 帧对象池
    QQueue<int> m_freeList;          ///< 空闲索引列表
    mutable QMutex m_mutex;          ///< 线程安全锁
    
    bool isValidIndex(int index) const;
};

} // namespace ZCAN

#endif // ZCAN_FRAME_POOL_H
