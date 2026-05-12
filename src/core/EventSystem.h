#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <queue>

struct Event { virtual ~Event() = default; };

// Event types
struct WindowResizeEvent  : Event { int width, height; };
struct KeyPressedEvent    : Event { int key; bool repeat; };
struct KeyReleasedEvent   : Event { int key; };
struct MouseMovedEvent    : Event { float x, y, dx, dy; };
struct MouseButtonEvent   : Event { int button; bool pressed; };
struct MouseScrollEvent   : Event { float delta; };
struct UpdateEvent        : Event { float dt; };
struct CollisionEvent     : Event { uint32_t entityA, entityB; };
struct SceneChangedEvent  : Event { std::string newScene; };

// ─── EventBus ─────────────────────────────────────────────────────────────────
//  Used:
//    EventBus::on<KeyPressedEvent>([](const KeyPressedEvent& e){ ... });
//    EventBus::emit<KeyPressedEvent>(KeyPressedEvent{GLFW_KEY_W, false});

class EventBus {
public:
    static EventBus& get() { static EventBus inst; return inst; }

    using HandlerID = uint32_t;

    // ID returned for later unsubscription
    template<typename E>
    HandlerID on(std::function<void(const E&)> handler) {
        static_assert(std::is_base_of_v<Event, E>, "E must derive from Event");
        auto id = m_nextId++;
        m_handlers[std::type_index(typeid(E))].push_back({
            id,
            [h = std::move(handler)](const Event& e) {
                h(static_cast<const E&>(e));
            }
        });
        return id;
    }

    // Unsubscribe using the ID returned by on()
    void off(HandlerID id) {
        for (auto& [type, vec] : m_handlers)
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                [id](const Handler& h){ return h.id == id; }), vec.end());
    }

    // Immediate dispatch
    template<typename E>
    void emit(const E& event) {
        auto it = m_handlers.find(std::type_index(typeid(E)));
        if (it == m_handlers.end()) return;
        for (auto& h : it->second) h.fn(event);
    }

    // Queued dispatch (flush() is called at the beginning of the frame)
    template<typename E>
    void enqueue(E event) {
        m_queue.push(std::make_unique<QueuedEvent<E>>(std::move(event)));
    }

    void flush() {
        while (!m_queue.empty()) {
            auto& qe = m_queue.front();
            qe->dispatch(*this);
            m_queue.pop();
        }
    }

private:
    struct Handler {
        HandlerID id;
        std::function<void(const Event&)> fn;
    };

    struct IQueuedEvent { virtual void dispatch(EventBus&) = 0; virtual ~IQueuedEvent() = default; };
    template<typename E>
    struct QueuedEvent : IQueuedEvent {
        E event;
        explicit QueuedEvent(E e) : event(std::move(e)) {}
        void dispatch(EventBus& bus) override { bus.emit(event); }
    };

    std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;
    std::queue<std::unique_ptr<IQueuedEvent>> m_queue;
    HandlerID m_nextId = 1;
};

// Shortcut
inline EventBus& Events() { return EventBus::get(); }
