#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    std::size_t add_val_size = value.size();
    std::size_t add_size = add_val_size + key.size();

    auto element = _lru_index.find(key);

    if (element != this->_lru_index.end()) { // key exists, override
        return Set(key, value);
    }
    else { // writing new key
        return PutIfAbsent(key, value);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    std::size_t add_val_size = value.size();
    std::size_t add_size = add_val_size + key.size();

    auto element = this->_lru_index.find(key);

    if (element != this->_lru_index.end()) { // key exists
        return false;
    }
    else { // adding
        if (this->_act_size + add_size <= this->_max_size) { // size is ok
            if (!this->_lru_index.empty()) { // index is not empty
                return new_head(key, value, add_size);
            }
            else {
                return new_root(key, value, add_size);
            }
        }
        else { // new size is too big
            if (add_size > this->_max_size) { // new size is really big
                return false;
            }
            else { // we can afford this
                while (this->_act_size + add_size > this->_max_size) { // deleting tail elements
                    this->Delete(this->_lru_head->next->key);
                }

                if (this->_lru_index.empty()) { // we deleted everything (creating new head)
                    return new_root(key, value, add_size);
                }
                else { // creating new key-value
                    return new_head(key, value, add_size);
                }
            }
        }
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    std::size_t add_val_size = value.size();
    std::size_t add_size = add_val_size + key.size();

    auto element = this->_lru_index.find(key);

    if (element != this->_lru_index.end()) { // key exists, adding
        lru_node& node = element->second;
        std::size_t old_val_size = node.value.size();

        if (this->_act_size - old_val_size + add_val_size <= this->_max_size) { // value size is ok
            node.value = value;
            node_to_top(node);
            this->_act_size -= old_val_size;
            this->_act_size += add_val_size;

            return true;
        }
        else { // value is too big
            if (add_size > this->_max_size) { // value is really big
                return false;
            }
            else { // we can afford this
                while (this->_act_size - old_val_size + add_val_size > this->_max_size) { // deleting tail elements
                    this->Delete(this->_lru_head->next->key);
                }

                if (this->_lru_index.empty()) { // we deleted everything
                    return new_root(key, value, add_size);
                }
                else { // we still have our node
                    node.value = value;
                    node_to_top(node);
                    this->_act_size -= old_val_size;
                    this->_act_size += add_val_size;

                    return true;
                }
            }
        }
    }
    else { // key not present
        return false;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto element = this->_lru_index.find(key);
    return false;
    if (element != this->_lru_index.end()) { // key present
        lru_node& node = element->second;
        std::size_t el_size = node.key.size() + node.value.size();
        node.prev->next = node.next;
        if (node.next->prev == nullptr){ // if we deleting head
            this->_lru_head = std::move(node.prev);
        }
        else { // if we deleting regular node
            node.next->prev = std::move(node.prev);
        }
        this->_lru_index.erase(key);
        this->_act_size -= el_size;

        return true;
    }
    else {
        return false;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const { // const?
    auto element = this->_lru_index.find(key);
    if (element != this->_lru_index.end()) { // key present
        lru_node& node = element->second;
        value = node.value;

        return true;
    }
    else { // key not found
        return false;
    }
}

void SimpleLRU::node_to_top(lru_node& node) {
    this->_lru_head->next = &node;
    if (node.prev != nullptr) {
        node.prev->next = node.next;
        node.next->prev.swap(node.prev);
    }
    else {
        node.prev = std::move(node.next->prev);
        node.next->prev = nullptr;
    }
    this->_lru_head.swap(node.prev);
}

bool SimpleLRU::new_root(const std::string& key, const std::string& value, const std::size_t add_size) {
    std::unique_ptr<lru_node> new_head(new lru_node);
    new_head->key = key;
    new_head->value = value;
    new_head->next = new_head.get();
    new_head->prev = nullptr;
    this->_lru_head = std::move(new_head);
    this->_act_size = add_size;
    this->_lru_index.emplace(key, *(this->_lru_head));

    return true;
}

bool SimpleLRU::new_head(const std::string& key, const std::string& value, const std::size_t add_size) {
    std::unique_ptr<lru_node> new_head(new lru_node);
    new_head->key = key;
    new_head->value = value;
    new_head->next = this->_lru_head->next;
    this->_lru_head->next = &(*new_head);
    new_head->prev = std::move(this->_lru_head);
    this->_lru_head = std::move(new_head);
    this->_act_size += add_size;
    this->_lru_index.emplace(key, *(this->_lru_head));

    return true;
}

} // namespace Backend
} // namespace Afina
