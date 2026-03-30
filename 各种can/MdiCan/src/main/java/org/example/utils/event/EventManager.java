package org.example.utils.event;

import java.util.ArrayList;
import java.util.List;

public class EventManager {
    private static final EventManager INSTANCE = new EventManager();
    private final List<EventListener> listeners = new ArrayList<>();

    private EventManager() {}

    public static EventManager getInstance() {
        return INSTANCE;
    }

    public void addListener(EventListener listener) {
        listeners.add(listener);
    }

    public void removeListener(EventListener listener) {
        listeners.remove(listener);
    }

    public void notifyListeners(String data) {
        for (EventListener listener : listeners) {
            listener.onEvent(data);
        }
    }
}
