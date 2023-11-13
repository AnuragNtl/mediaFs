#ifndef SERVER_H

#define SERVER_H

#include "../FSProvider.h"
#include <map>
#include <functional>
#include <memory>
#include <mutex>

#define DEFAULT_OPEN_HANDLES_SIZE 10

namespace MediaFs {

    template<class I, class T>
    class Node {
        private:
            Node<I, T> *next;
            Node<I, T> *previous;
            T &value;
            I index;
        public:
            Node(I index, T &value, Node<I, T> *previous=NULL);
            Node<I, T>* getNext();
            Node<I, T>* getPrevious();
            void append(Node<I, T> *);
            void detach();
            virtual ~Node() = default;
            template<class I1, class T1> friend class LRUCache;
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
            void add(I, T&);
            virtual ~LRUCache();
    };

    struct Buffer {
        char *data;
        int size;
        ~Buffer() {
            delete[] data;
        }
    };

    struct FileCache {
        std::unique_ptr<std::ifstream> fileHandle;
        std::map<int, std::unique_ptr<Buffer> > buffers;
        void refreshRanges();
    };

    class Server : public FSProvider {
        private:
            std::map<std::string, FileCache*> openHandles;
            int openHandlesSize;
        public:
            Server(int openHandlesSize = DEFAULT_OPEN_HANDLES_SIZE);
            Server(const Server &) = delete;
            int read(std::string path, char *buffer, int size, int offset) const;
            std::vector<Attr> readDir(std::string path) const;
            Attr getAttr(std::string path) const;
            virtual ~Server();

    };

    extern std::pair<int, std::unique_ptr<Buffer> >* combineRanges(std::pair<int, std::unique_ptr<Buffer> > &, std::pair<int, std::unique_ptr<Buffer> > &);
};

#endif

