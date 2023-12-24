#include "./Server.h"
#include <list>
#include <fstream>
#include <memory>

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
        //TODO
        return 0;
    }

    Server :: ~Server() {
        for (auto pair : openHandles) {
            delete[] pair.second;
        }
    }

    FileCache :: FileCache(std::unique_ptr<std::ifstream> &&fileHandle) : fileHandle(std::move(fileHandle)) { }

    void FileCache :: refreshRanges() {
        std::list<std::pair<int, std::unique_ptr<Buffer> > > ranges;
        for (auto buffer = buffers.begin(); buffer != buffers.end(); buffer++) {
            std::pair<int, std::unique_ptr<Buffer> > range(buffer->first, std::move(buffer->second));
            ranges.push_back(std::move(range));
        }
        ranges.sort([] (const std::pair<int, std::unique_ptr<Buffer> > &first, const std::pair<int, std::unique_ptr<Buffer> > &second) {
                return first.first < second.first;
                });

        int prevFrom = -1, prevTo = -1;
        std::pair<int, std::unique_ptr<Buffer> > *prevRange = NULL;
        std::list<std::pair<int, std::unique_ptr<Buffer> > > combined;
        for (auto &range : ranges) {
            if (prevRange == NULL) {
                prevRange = &range;
                continue;
            }
            int lb = range.first, ub = range.first + range.second->size;
            int prevFrom = prevRange->first, prevTo = prevRange->first + prevRange->second->size;
            if ((prevFrom >= lb && prevFrom < ub) && (prevTo >= lb && prevTo < ub)) {
                delete prevRange;
                prevRange = NULL;
            } else if ((lb >= prevFrom && lb < prevTo) && (ub >= prevFrom && ub < prevTo)) {
                range.second.release();
            } else if ((lb >= prevFrom && lb < (prevTo + 1)) || (ub >= prevFrom && ub < prevTo)) {
                prevRange = MediaFs::combineRanges(*prevRange, range);
            } else {
                combined.push_back(std::move(*prevRange));
                prevRange = &range;
            }
        }

        if (prevRange != NULL) {
            combined.push_back(std::move(*prevRange));
        }

        buffers.erase(buffers.begin(), buffers.end());

        for (auto &item : combined) {
            buffers[item.first] = std::move(item.second);
        }




        /*for (auto range : ranges) {

          }*/


    }

    std::pair<int, std::unique_ptr<Buffer>>* combineRanges(std::pair<int, std::unique_ptr<Buffer> > &firstRange, std::pair<int, std::unique_ptr<Buffer> > &secondRange) {
        Buffer *buffer = new Buffer;
        if (firstRange.first > secondRange.first) {
            auto temp = std::move(firstRange);
            firstRange = std::move(secondRange);
            secondRange = std::move(temp);
        }
        int lb1 = firstRange.first, ub1 = firstRange.first + firstRange.second->size;
        int lb2 = secondRange.first, ub2 = secondRange.first + secondRange.second->size;
        buffer->data = new char[ub2 - lb1];
        memcpy(buffer->data, firstRange.second->data, firstRange.second->size);
        if (lb2 == (ub1 + 1)) {
            memcpy(buffer->data + firstRange.second->size, secondRange.second->data, secondRange.second->size);
        } else {
            memcpy(buffer->data + firstRange.second->size, secondRange.second->data + (ub1 - lb2),ub2 - ub1);
        }
        buffer->size = ub2 - lb1;
        Buffer *firstBuffer = firstRange.second.release(),
               *secondBuffer = secondRange.second.release();
        delete firstBuffer;
        delete secondBuffer;
        return new std::pair<int, std::unique_ptr<Buffer> >(lb1, std::unique_ptr<Buffer>(buffer));
    }

    std::vector<Attr> Server :: readDir(std::string path) const {
        return {};
    }

    Attr Server :: getAttr(std::string path) const {
        return Attr{};
    }

}
