#include "./Server.h"
#include <list>
#include <fstream>

namespace MediaFs {

    template<class I, class T>
    Node<I, T> :: Node(I index, T &value, Node<I, T> *previous) : index(index), value(value), previous(previous) {
        if (previous != NULL) {
            previous->next = this;
        }
        next = NULL;
    }

    template<class I, class T>
    Node<I, T>* Node<I, T> :: getNext() {
        return next;
    }

    template<class I, class T>
    Node<I, T>* Node<I, T> :: getPrevious() {
        return previous;
    }

    template<class I, class T>
    void Node<I, T> :: append(Node<I, T> *previous) {
        this->previous = previous;
        previous->next = this;
        this->next = NULL;
    }

    template<class I, class T>
    void Node<I, T> :: detach() {
        if (this->next != NULL) {
            this->next->previous = this->previous;
        }
        if (this->previous != NULL) {
            this->previous->next = this->next;
        }
        this->next = NULL;
        this->previous = NULL;
    }

    template<class I, class T>
    LRUCache<I, T> :: LRUCache(int length) : length(length), _onFull([] (I, T) { }), currentLength(0)  { }

    template<class I, class T>
    T& LRUCache<I, T> :: operator[](I index) {
        if (location.find(index) == location.end()) {
            throw InvalidItemException("index not found");
        }
        globalMutex.lock();
        auto node = location[index];
        node->detach();
        node->append(tail);
        globalMutex.unlock();
        return node->value;
    }

    template<class I, class T>
    void LRUCache<I, T> :: add(I index, T &item) {
        if (location.find(item) != location.end()) {
            throw InvalidItemException("item already present");
        }
        globalMutex.lock();
        Node<I, T> *node;
        if (currentLength >= length) {
            T &headValue = head->value;
            evict(head->index);
            _onFull(index, headValue);
        }
        if (head == NULL) {
            node = new Node<I, T>(item);
            head = node;
            tail = node;
        } else {
            node = new Node<I, T>(item, tail);
            tail = node;
        }
        currentLength++;
        globalMutex.unlock();
    }

    template<class I, class T>
    void LRUCache<I, T> :: evict(I index) {
        if (location.find(index) == location.end()) {
            throw new InvalidItemException("Item not found");
        }
        Node<I, T> *node = location[index];
        location.erase(index);
        evictNode(node);
        currentLength--;
    }

    template<class I, class T>
    void LRUCache<I, T> :: evictNode(Node<I, T> *node) {
        if (node == head) {
            head = head->next;
        } else if (node == tail) {
            tail = tail->previous;
        }
        node->detach();
        delete node;
    }

    template<class I, class T>
    void LRUCache<I, T> :: onFull(std::function<void(I, const T&)> _onFull) {
        this->_onFull = _onFull;
    }

    template<class I, class T>
    LRUCache<I, T> :: ~LRUCache() {
        globalMutex.lock();
        if (head == NULL) {
            return;
        }
        for (auto node = head->next; node != NULL; node = node->next) {
            delete node->previous;
        }
        delete tail;
        globalMutex.unlock();
    }

    InvalidItemException :: InvalidItemException(std::string message) : message(message) { }

    Server :: Server(int openHandlesSize) : openHandlesSize(openHandlesSize) { }


    int Server :: read(std::string path, char *buffer, int size, int offset) const {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        char *buf = new char[size];
        in.seekg(offset, std::ios::beg);
        in.read(buf, size);
        in.close();
    }

    Server :: ~Server() {
        for (auto pair : openHandles) {
            delete[] pair.second;
        }
    }

    void FileCache :: refreshRanges() {
        std::list<int> ranges;
        for (auto buffer = buffers.begin(); buffer != buffers.end(); buffer++) {
            ranges.push_back(buffer->first);
        }
        ranges.sort();


    }
}

