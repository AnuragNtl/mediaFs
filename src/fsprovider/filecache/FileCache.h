#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <cstring>
#include <map>
#include <functional>
#include <memory>
#include <list>
#include <mutex>
#include <tuple>
#include <thread>
#include <fstream>

namespace MediaFs {
    template<class I, class T>
    class Node {
        private:
            Node<I, T> *next;
            Node<I, T> *previous;
            T value;
            I index;
        public:
            Node(I index, T value, Node<I, T> *previous=NULL);
            Node<I, T>* getNext();
            Node<I, T>* getPrevious();
            void append(Node<I, T> *);
            void detach();
            virtual ~Node() = default;
            template<class I1, class T1> friend class LRUCache;
    };

    template<class I, class T>
    class LRUCache {
        private:
            Node<I, T> *head, *tail;
            std::map<I, Node<I, T>*> location;
            int length;
            int currentLength;
            std::function<void(I, const T&)> _onFull;
            void evict(I);
            void evictNode(Node<I, T> *node);
            std::mutex globalMutex;
        public:
            LRUCache(int length);
            bool has(I);
            T& operator[](I);
            void onFull(std::function<void(I, const T&)>);
            void add(I, T);
            virtual ~LRUCache();
    };

    class InvalidItemException : public std::exception {
        private:
            std::string message;
        public:
            InvalidItemException(std::string message);
            const char* what() const noexcept {
                return message.c_str();
            }
    };

    template<class I, class T>
    Node<I, T> :: Node(I index, T value, Node<I, T> *previous) : index(index), value(value), previous(previous) {
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
    LRUCache<I, T> :: LRUCache(int length) : length(length), _onFull([] (I, T) { }), currentLength(0), head(NULL), tail(NULL)  { }

    template<class I, class T>
    T& LRUCache<I, T> :: operator[](I index) {
        if (location.find(index) == location.end()) {
            throw InvalidItemException("index not found");
        }
        globalMutex.lock();
        auto node = location[index];
        if (head != tail) {
            if (node == head) {
                head = node->next;
            }
            node->detach();
            node->append(tail);
            tail = node;
        }
        globalMutex.unlock();
        return node->value;
    }

    template<class I, class T>
    void LRUCache<I, T> :: add(I index, T item) {
        if (location.find(index) != location.end()) {
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
            node = new Node<I, T>(index, item);
            head = node;
            tail = node;
        } else {
            node = new Node<I, T>(index, item, tail);
            tail = node;
        }
        location[index] = node;
        currentLength++;
        globalMutex.unlock();
    }

    template<class I, class T>
    bool LRUCache<I, T> :: has(I index) {
        return location.find(index) != location.end();
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
            globalMutex.unlock();
            return;
        }
        for (auto node = head->next; node != NULL; node = node->next) {
            delete node->previous;
        }
        delete tail;
        globalMutex.unlock();
    }


    struct Buffer {
        char *data;
        int size;
        Buffer() = default;
        Buffer(const char *data, int size) {
            this->data = new char[size];
            memcpy(this->data, data, size * sizeof(const char));
            this->size = size;
        }
        ~Buffer() {
            delete[] data;
        }
    };

    std::ostream& operator<<(std::ostream &out, const Buffer &);

    extern std::pair<int, std::unique_ptr<Buffer> >* combineRanges(std::pair<int, std::unique_ptr<Buffer> > &, std::pair<int, std::unique_ptr<Buffer> > &);

    class FileHandle {
        public:
            virtual int read(char *buf, int start, int size) = 0;
            virtual ~FileHandle() {}
    };

    class IfstreamFileHandle : public FileHandle {
        private:
            std::ifstream &&in;
        public:
            IfstreamFileHandle(std::ifstream &&in);
            int read(char *buf, int start, int size);
            ~IfstreamFileHandle();
    };

    struct FileCache {
        private:
            static void refreshRangesInBackground(FileCache *fileCache);
        public:
            std::mutex handleMutex;
            std::map<int, std::unique_ptr<Buffer> > buffers;
            std::unique_ptr<FileHandle> fileHandle;
            FileCache(std::unique_ptr<FileHandle> &&);
            void refreshRanges();
            void refreshRangesAsync();
            std::tuple<const char*, int> operator[](std::pair<int, int>);
    };

    std::ostream& operator<<(std::ostream &, const FileCache &);

    class FileCacheException : std::exception {
        private:
            std::string message;
        public:
            FileCacheException(std::string message) : message(message) { }
        const char* what() noexcept {
            return message.c_str();
        }
    };

};

#endif


