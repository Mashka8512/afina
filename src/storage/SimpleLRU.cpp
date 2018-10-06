#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);

            if (it != _lru_index.end()) {
                return Update(it->second.get(), value);
            }

            if(key.size()+value.size()>_max_size){

                return false;
            }

    return Insert(key, value);
}


bool SimpleLRU::Insert(const std::string &key, const std::string &value)
{
    while(key.size()+value.size()+_act_size>_max_size)
    {
        DeleteLRU();
    }
    std::unique_ptr<lru_node> ptr(new lru_node(key, value, _lru_tail));
            lru_node * tmp = ptr.get();
            ptr.swap(ptr->next);
            if (!Insert_ptr(*tmp))
                    return false;
    _lru_index.insert(std::pair<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>>(_lru_tail->key, *_lru_tail));
    return true;


}

bool SimpleLRU::Insert_ptr(lru_node &node) {
std::size_t len = node.key.size() + node.value.size();
        if (len > _max_size)
                return false;
        while (len + _act_size > _max_size) {
                if (!DeleteLRU())
                        return false;
        }
        _act_size += len;
        if (_lru_tail != nullptr) {
                _lru_tail->next.swap(node.next);
                node.prev = _lru_tail;
                _lru_tail = _lru_tail->next.get();
        }
        else {
                _lru_head.swap(node.next);
                node.prev = _lru_tail;
                _lru_tail = _lru_head.get();
        }
return true;

}

bool SimpleLRU::DeleteLRU()
{
    if (_lru_head == nullptr)
                    return false;
            _lru_index.erase(_lru_head->key);
    return Erase(*_lru_head);

}

bool SimpleLRU::Erase(lru_node &node) {
        if (!Erase_ptr(node))
                return false;
        node.next.reset();
        return true;

}

bool SimpleLRU::Erase_ptr(lru_node &node) {
        std::size_t len = node.key.size() + node.value.size();
        _act_size -= len;
        if (node.next != nullptr)
                node.next->prev = node.prev;
        else
                _lru_tail = node.prev;
        if (node.prev != nullptr)
                node.prev->next.swap(node.next);
        else
                _lru_head.swap(node.next);
        return true;

}

bool SimpleLRU::Update(lru_node &node, const std::string &value) {
       if (!Erase_ptr(node))
                return false;
        if (!Insert_ptr(node))
                return false;
        _lru_tail->value = value;
        return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
            if (it != _lru_index.end()) {
                return false;
            }
    return Insert(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto it = _lru_index.find(key);
            if (it != _lru_index.end()) {
                return false;
            }

    return Update(it->second.get(), value);

}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
            if (it != _lru_index.end()) {
                lru_node* tmp = &_lru_index.at(key).get();
                 return Erase(*tmp);
            }

    return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
    auto it = _lru_index.find(key);
            if (it == _lru_index.end())
                    return false;
            lru_node* tmp = &_lru_index.at(key).get();
            value = tmp->value;
    return true;
}

} // namespace Backend
} // namespace Afina
