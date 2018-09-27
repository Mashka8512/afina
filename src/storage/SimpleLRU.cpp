#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    std::size_t add_val_size = value.size();
    std::size_t add_size = add_val_size + key.size();
    
    auto element = this->_lru_index.find(key);
    
    if (element != this->_lru_index.end()) { // key exists, override
        std::size_t old_val_size = element->second.value.size();
        
        if (this->_act_size - old_val_size + add_val_size <= this->_max_size) { // value size is ok
            element->second.value = value;
            element->second.prev->next = element->second.next;
            element->second.next->prev = std::move(element->second.prev);
            this->_lru_head->next = &element;
            element->second.prev = std::move(this->_lru_head);
            std::unique_ptr<lru_node> el_ptr(element);
            this->_lru_head = std::move(el_ptr);
            this->act_size -= old_val_size;
            this->act_size += add_val_size;
            
            return true;
        }
        else { // value is too big
            if (add_size > this->max_size) { // value is really big
                return false;
            }
            else { // we can afford this
                while (this->_act_size - old_val_size + add_val_size > this->_max_size) { // deleting tail elements
                    this->Delete(this->_lru_head->next->key);
                }
                
                if (this->_lru_index.empty()) { // we deleted everything
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = &new_head;
                    new_head->prev = nullptr;
                    this->_lru_head = std::move(new_head);
                    this->act_size = add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
                }
                else { // we still have our element
                    element->second.value = value;
                    element->second.prev->next = element->second.next;
                    element->second.next->prev = std::move(element->second.prev);
                    this->_lru_head->next = &element;
                    element->second.prev = std::move(this->_lru_head);
                    std::unique_ptr<lru_node> el_ptr(element);
                    this->_lru_head = std::move(el_ptr);
                    this->act_size -= old_val_size;
                    this->act_size += add_val_size;
            
                    return true;
                }
            }
        }
    }
    else { // writing new key
        if (this->_act_size + add_size <= this->_max_size) { // key + value size is ok
            std::unique_ptr<lru_node> new_head(new lru_node);
            new_head->key = key;
            new_head->value = value;
            new_head->next = this->_lru_head->next;
            this->_lru_head->next = &(*new_head);
            new_head->prev = std::move(this->_lru_head);
            this->_lru_head = std::move(new_head);
            this->act_size += add_size;
            this->_lru_index.emplace(key, *(this->_lru_head));
            
            return true;
        }
        else { // key + value too big
            if (add_size > this->max_size) { // key + value really BIG
                return false;
            }
            else { // we can afford this
                while (this->_act_size + add_size > this->_max_size) { // deleting tail elements
                    this->Delete(this->_lru_head->next->key);
                }
                
                if (this->_lru_index.empty()) { // we deleted everything (creating new head)
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = &new_head;
                    new_head->prev = nullptr;
                    this->_lru_head = std::move(new_head);
                    this->act_size = add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
                }
                else { // creating new key-value
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = this->_lru_head->next;
                    this->_lru_head->next = &(*new_head);
                    new_head->prev = std::move(this->_lru_head);
                    this->_lru_head = std::move(new_head);
                    this->act_size += add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
                }
            }
        }
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
            std::unique_ptr<lru_node> new_head(new lru_node);
            new_head->key = key;
            new_head->value = value;
            new_head->next = nullptr;
            this->_lru_head->next = &(*new_head);
            new_head->prev = std::move(this->_lru_head);
            this->_lru_head = std::move(new_head);
            this->act_size += add_size;
            this->_lru_index.emplace(key, *(this->_lru_head));
            
            return true;
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
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = &new_head;
                    new_head->prev = nullptr;
                    this->_lru_head = std::move(new_head);
                    this->act_size = add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
                }
                else { // creating new key-value
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = this->_lru_head->next;
                    this->_lru_head->next = &(*new_head);
                    new_head->prev = std::move(this->_lru_head);
                    this->_lru_head = std::move(new_head);
                    this->act_size += add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
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
        std::size_t old_val_size = element->second.value.size();
        
        if (this->_act_size - old_val_size + add_val_size <= this->_max_size) {
            element->second.value = value;
            element->second.prev->next = element->second.next;
            element->second.next->prev = std::move(element->second.prev);
            this->_lru_head->next = &element;
            element->second.prev = std::move(this->_lru_head);
            std::unique_ptr<lru_node> el_ptr(element);
            this->_lru_head = std::move(el_ptr);
            this->act_size -= old_val_size;
            this->act_size += add_val_size;
            
            return true;
        }
        else { // value is too big
            if (add_size > this->max_size) { // value is really big
                return false;
            }
            else { // we can afford this
                while (this->_act_size - old_val_size + add_val_size > this->_max_size) { // deleting tail elements
                    this->Delete(this->_lru_head->next->key);
                }
                
                if (this->_lru_index.empty()) { // we deleted everything
                    std::unique_ptr<lru_node> new_head(new lru_node);
                    new_head->key = key;
                    new_head->value = value;
                    new_head->next = &new_head;
                    new_head->prev = nullptr;
                    this->_lru_head = std::move(new_head);
                    this->act_size = add_size;
                    this->_lru_index.emplace(key, *(this->_lru_head));
                    
                    return true;
                }
                else { // we still have our element
                    element->second.value = value;
                    element->second.prev->next = element->second.next;
                    element->second.next->prev = std::move(element->second.prev);
                    this->_lru_head->next = &element;
                    element->second.prev = std::move(this->_lru_head);
                    std::unique_ptr<lru_node> el_ptr(element);
                    this->_lru_head = std::move(el_ptr);
                    this->act_size -= old_val_size;
                    this->act_size += add_val_size;
            
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
    
    if (element != this->_lru_index.end()) { // key present
        std::size_t el_size = element->second.key.size() + element->second.value.size();
        element->second.prev->next = element->second.next;
        if (element->second.next->prev == nullptr){ // if we deleting head
            this->_lru_head = std::move(element->second.prev);
        }
        else { // if we deleting regular node
            element->second.next->prev = std::move(element->second.prev);
        }
        this->_lru_index.erase(key);
        this->act_size -= el_size;
        
        return true;
    }
    else {
        return false;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const {
    auto element = this->_lru_index.find(key);
    
    if (element != this->_lru_index.end()) { // key present
        value = element->second.value;
        element->second.prev->next = element->second.next;
        element->second.next->prev = std::move(element->second.prev);
        this->_lru_head->next = &element;
        element->second.prev = std::move(this->_lru_head);
        std::unique_ptr<lru_node> el_ptr(element);
        this->_lru_head = std::move(el_ptr);
        
        return true;
    }
    else { // key not found
        return false;
    }
}

} // namespace Backend
} // namespace Afina
