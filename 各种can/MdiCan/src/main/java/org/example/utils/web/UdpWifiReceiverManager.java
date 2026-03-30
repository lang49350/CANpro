package org.example.utils.web;

import java.util.HashMap;
import java.util.Map;

public class UdpWifiReceiverManager {

    private static UdpWifiReceiverManager instance;

    private Map<Integer,UdpWifiDataReceiver> receivers = new HashMap<>();

    private UdpWifiReceiverManager()
    {
        receivers = new HashMap<>();
    }
    public static synchronized UdpWifiReceiverManager getInstance()
    {
        if (instance == null)
        {
            instance = new UdpWifiReceiverManager();
        }
        return instance;
    }

    public void addReceiver(int port, UdpWifiDataReceiver receiver)
    {
        receivers.put(port, receiver);
    }

    public UdpWifiDataReceiver getReceiver(int port)
    {
        return receivers.get(port);
    }

    public void removeReceiver(int port)
    {
        receivers.remove(port);
    }

    public boolean hasReceiver(int port)
    {
        return receivers.containsKey(port);
    }

    public void stopAllReceivers()
    {
        for (UdpWifiDataReceiver receiver : receivers.values())
        {
            receiver.stop();
        }
    }

    public void removeAllReceivers()
    {
        stopAllReceivers();
        receivers.clear();
    }
}
