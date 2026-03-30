#include "DataRouter.h"
#include <QWidget>
#include <QDebug>
#include <QMetaMethod>
#include <QSet>  // 🚀 阶段2优化：rebuildWidgetBuffers需要
#include <climits>

// 静态成员初始化
DataRouter* DataRouter::s_instance = nullptr;

DataRouter::DataRouter(QObject *parent)
    : QObject(parent)
    , m_routeToken(1)
{
}

DataRouter::~DataRouter()
{
    m_subscriptions.clear();
}

DataRouter* DataRouter::instance()
{
    if (!s_instance) {
        s_instance = new DataRouter();
    }
    return s_instance;
}

void DataRouter::subscribe(QWidget *widget, const QString &device, int channel)
{
    if (!widget) {
        qWarning() << "⚠️ DataRouter::subscribe: widget为空";
        return;
    }
    
    // 先取消旧订阅
    unsubscribe(widget);
    
    // 添加新订阅
    m_subscriptions.append(Subscription(widget, device, channel));
    
    // 🚀 阶段2优化：重建窗口缓冲区
    rebuildWidgetBuffers();
}

void DataRouter::unsubscribe(QWidget *widget)
{
    if (!widget) {
        return;
    }
    
    // 移除该窗口的所有订阅
    for (int i = m_subscriptions.size() - 1; i >= 0; --i) {
        if (m_subscriptions[i].widget == widget) {
            m_subscriptions.removeAt(i);
        }
    }
    
    // 🚀 阶段2优化：重建窗口缓冲区
    rebuildWidgetBuffers();
}

void DataRouter::routeFrame(const QString &device, const CANFrame &frame)
{
    auto allIt = m_deviceAllSubscriptionIndices.constFind(device);
    if (allIt != m_deviceAllSubscriptionIndices.constEnd()) {
        const QVector<int> &allIndices = allIt.value();
        for (int subIndex : allIndices) {
            const Subscription &sub = m_subscriptions[subIndex];
            emit frameRoutedToWidget(sub.widget, frame);
        }
    }

    auto deviceChannelIt = m_deviceChannelSubscriptionIndices.constFind(device);
    if (deviceChannelIt != m_deviceChannelSubscriptionIndices.constEnd()) {
        auto channelIt = deviceChannelIt.value().constFind(frame.channel);
        if (channelIt != deviceChannelIt.value().constEnd()) {
            const QVector<int> &channelIndices = channelIt.value();
            for (int subIndex : channelIndices) {
                const Subscription &sub = m_subscriptions[subIndex];
                emit frameRoutedToWidget(sub.widget, frame);
            }
        }
    }
}

void DataRouter::routeFrames(const QString &device, const QVector<CANFrame> &frames)
{
    if (frames.isEmpty()) {
        return;
    }

    // 显示层已切换为存储层拉取时，若没有任何路由信号监听者，直接返回避免无效分发开销。
    if (!isSignalConnected(QMetaMethod::fromSignal(&DataRouter::framesRoutedToWidget)) &&
        !isSignalConnected(QMetaMethod::fromSignal(&DataRouter::frameRoutedToWidget))) {
        return;
    }

    auto allIt = m_deviceAllSubscriptionIndices.constFind(device);
    auto channelMapIt = m_deviceChannelSubscriptionIndices.constFind(device);
    if (allIt == m_deviceAllSubscriptionIndices.constEnd() &&
        channelMapIt == m_deviceChannelSubscriptionIndices.constEnd()) {
        return;
    }

    int routeToken = m_routeToken++;
    if (m_routeToken == INT_MAX) {
        m_routeToken = 1;
        for (auto &buffer : m_widgetBuffers) {
            buffer.lastRouteToken = 0;
        }
    }

    m_touchedBufferIndices.clear();
    m_touchedBufferIndices.reserve(m_widgetBuffers.size());

    auto appendByIndices = [this, routeToken](const QVector<int> &indices, const CANFrame &currentFrame) {
        for (int subIndex : indices) {
            const Subscription &sub = m_subscriptions[subIndex];
            int bufferIndex = sub.bufferIndex;
            if (bufferIndex < 0) {
                continue;
            }

            WidgetBuffer &buffer = m_widgetBuffers[bufferIndex];
            if (buffer.lastRouteToken != routeToken) {
                buffer.frames.clear();
                buffer.lastRouteToken = routeToken;
                m_touchedBufferIndices.append(bufferIndex);
            }
            buffer.frames.append(currentFrame);
        }
    };

    for (const CANFrame &frame : frames) {
        if (allIt != m_deviceAllSubscriptionIndices.constEnd()) {
            appendByIndices(allIt.value(), frame);
        }

        if (channelMapIt != m_deviceChannelSubscriptionIndices.constEnd()) {
            auto channelIt = channelMapIt.value().constFind(frame.channel);
            if (channelIt != channelMapIt.value().constEnd()) {
                appendByIndices(channelIt.value(), frame);
            }
        }
    }

    for (int bufferIndex : m_touchedBufferIndices) {
        const WidgetBuffer &buffer = m_widgetBuffers[bufferIndex];
        if (!buffer.frames.isEmpty()) {
            emit framesRoutedToWidget(buffer.widget, buffer.frames);
        }
    }
}

bool DataRouter::getSubscription(QWidget *widget, QString &device, int &channel) const
{
    for (const Subscription &sub : m_subscriptions) {
        if (sub.widget == widget) {
            device = sub.device;
            channel = sub.channel;
            return true;
        }
    }
    return false;
}

bool DataRouter::hasSubscribers(const QString &device) const
{
    auto allIt = m_deviceAllSubscriptionIndices.constFind(device);
    if (allIt != m_deviceAllSubscriptionIndices.constEnd() && !allIt.value().isEmpty()) {
        return true;
    }

    auto channelIt = m_deviceChannelSubscriptionIndices.constFind(device);
    return channelIt != m_deviceChannelSubscriptionIndices.constEnd() && !channelIt.value().isEmpty();
}

bool DataRouter::matchesSubscription(const Subscription &sub, const CANFrame &frame) const
{
    // channel == -1 表示订阅所有通道
    if (sub.channel == -1) {
        return true;
    }
    
    // 否则检查通道号是否匹配
    return frame.channel == sub.channel;
}

// 🚀 阶段2优化：重建窗口缓冲区
void DataRouter::rebuildWidgetBuffers()
{
    m_deviceAllSubscriptionIndices.clear();
    m_deviceChannelSubscriptionIndices.clear();
    m_widgetBufferIndex.clear();

    QSet<QWidget*> uniqueWidgets;
    for (int i = 0; i < m_subscriptions.size(); ++i) {
        const Subscription &sub = m_subscriptions[i];
        uniqueWidgets.insert(sub.widget);
        if (sub.channel == -1) {
            m_deviceAllSubscriptionIndices[sub.device].append(i);
        } else {
            m_deviceChannelSubscriptionIndices[sub.device][sub.channel].append(i);
        }
    }

    m_widgetBuffers.clear();
    m_widgetBuffers.reserve(uniqueWidgets.size());

    for (QWidget *widget : uniqueWidgets) {
        int bufferIndex = m_widgetBuffers.size();
        m_widgetBuffers.append(WidgetBuffer(widget));
        m_widgetBufferIndex.insert(widget, bufferIndex);
    }

    for (Subscription &sub : m_subscriptions) {
        sub.bufferIndex = m_widgetBufferIndex.value(sub.widget, -1);
    }
}
