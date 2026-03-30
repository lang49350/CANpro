#ifndef TIME_SYNC_MANAGER_H
#define TIME_SYNC_MANAGER_H

#include <Arduino.h>
#include <esp_timer.h>

class TimeSyncManager {
public:
    static TimeSyncManager& getInstance() {
        static TimeSyncManager instance;
        return instance;
    }

    void init();
    
    // Periodically called to sample times
    void updateSync(uint8_t channel, uint32_t currentTBC);
    
    // Convert hardware TBC to ESP32 system time
    uint64_t getSystemTime(uint8_t channel, uint32_t frameTBC);

private:
    TimeSyncManager() {
        for(int i=0; i<2; i++) {
            m_lastTBC[i] = 0;
            m_lastSysTime[i] = 0;
            m_tbcOverflowCount[i] = 0;
            m_slope[i] = 1.0;
            m_initialized[i] = false;
        }
    }

    struct SyncPoint {
        uint32_t tbc;
        uint64_t sysTime;
    };

    uint32_t m_lastTBC[2];
    uint64_t m_lastSysTime[2];
    uint64_t m_tbcOverflowCount[2]; // How many times TBC has overflowed
    double m_slope[2];              // Ratio of SysTime / TBC
    bool m_initialized[2];
};

#endif // TIME_SYNC_MANAGER_H