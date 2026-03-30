#include "time_sync.h"

void TimeSyncManager::init() {
    for(int i=0; i<2; i++) {
        m_lastTBC[i] = 0;
        m_lastSysTime[i] = 0;
        m_tbcOverflowCount[i] = 0;
        m_slope[i] = 1.0;
        m_initialized[i] = false;
    }
}

void TimeSyncManager::updateSync(uint8_t channel, uint32_t currentTBC) {
    if (channel >= 2) return;
    
    uint64_t currentSysTime = esp_timer_get_time();
    
    if (!m_initialized[channel]) {
        m_lastTBC[channel] = currentTBC;
        m_lastSysTime[channel] = currentSysTime;
        m_initialized[channel] = true;
        return;
    }
    
    // Check for overflow (32-bit counter)
    if (currentTBC < m_lastTBC[channel]) {
        // TBC overflowed (happens every ~71 minutes)
        // Since we update frequently (e.g. 1s), it can only overflow once
        m_tbcOverflowCount[channel]++;
    }
    
    // Calculate elapsed time
    uint64_t elapsedTBC = currentTBC;
    if (currentTBC < m_lastTBC[channel]) {
        elapsedTBC += 0x100000000ULL; // Add 2^32
    }
    uint64_t deltaTBC = elapsedTBC - m_lastTBC[channel];
    uint64_t deltaSys = currentSysTime - m_lastSysTime[channel];
    
    if (deltaTBC > 0) {
        // Simple moving average or direct calculation for slope
        // If the crystal is perfect, slope = 1.0
        double currentSlope = (double)deltaSys / (double)deltaTBC;
        
        // Smooth the slope to avoid jitter
        m_slope[channel] = m_slope[channel] * 0.9 + currentSlope * 0.1;
    }
    
    m_lastTBC[channel] = currentTBC;
    m_lastSysTime[channel] = currentSysTime;
}

uint64_t TimeSyncManager::getSystemTime(uint8_t channel, uint32_t frameTBC) {
    if (channel >= 2 || !m_initialized[channel]) {
        return esp_timer_get_time();
    }
    
    // Determine the absolute TBC of this frame
    uint64_t absFrameTBC = ((uint64_t)m_tbcOverflowCount[channel] << 32) | frameTBC;
    
    // Handle the edge case where the frame was received before a recent TBC overflow
    // but processed after the updateSync detected the overflow
    if (frameTBC > m_lastTBC[channel] + 0x80000000ULL && m_tbcOverflowCount[channel] > 0) {
        absFrameTBC -= 0x100000000ULL;
    }
    // Handle the edge case where frame was received after an overflow, 
    // but updateSync hasn't run yet to increment the overflow count
    else if (frameTBC < m_lastTBC[channel] && (m_lastTBC[channel] - frameTBC) > 0x80000000ULL) {
        absFrameTBC += 0x100000000ULL;
    }
    
    uint64_t absLastTBC = ((uint64_t)m_tbcOverflowCount[channel] << 32) | m_lastTBC[channel];
    
    // Calculate the difference from the last sync point
    int64_t tbcDiff = (int64_t)absFrameTBC - (int64_t)absLastTBC;
    
    // Apply slope
    int64_t sysDiff = (int64_t)(tbcDiff * m_slope[channel]);
    
    return m_lastSysTime[channel] + sysDiff;
}
