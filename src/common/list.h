#pragma once

#include <type_traits>
#include "common/defs.h"

template <typename T>
struct list_node {
    T* prev {nullptr};
    T* next {nullptr};
};

// Lists do not circle around
template <typename T>
class list {
public:
    class fwd_iterator {
    public:
        fwd_iterator(T* elem) : elem{elem} {}
        
        fwd_iterator& operator++() {
            elem = elem->next;
            return *this;
        }
        
        fwd_iterator operator++(int) {
            fwd_iterator tmp = *this;
            ++*this;
            return tmp;
        }
        
        fwd_iterator& operator--() {
            elem = elem->prev;
            return *this;
        }
        
        fwd_iterator operator--(int) {
            fwd_iterator tmp = *this;
            --*this;
            return tmp;
        }
        
        bool operator!=(const fwd_iterator& it) {
            return elem != it.elem;
        }
        
        T* operator*() {
            return elem;
        }
    private:
        T* elem;
    };
    
    class back_iterator {
    public:
        back_iterator(T* elem) : elem{elem} {};
        
        back_iterator& operator++() {
            elem = elem->prev;
            return *this;
        }
        
        back_iterator operator++(int) {
            back_iterator tmp = *this;
            ++*this;
            return tmp;
        }
        
        back_iterator& operator--() {
            elem = elem->next;
            return *this;
        }
        
        back_iterator operator--(int) {
            back_iterator tmp = *this;
            --*this;
            return tmp;
        }
        
        bool operator!=(const back_iterator& it) {
            return elem != it.elem;
        }
        
        T* operator*() {
            return elem;
        }
    private:
        T* elem;
    };
    
    void push_back(T* element) {
        ASSERT(element, "Element was nullptr");
        
        if (tail) {
            tail->next = element;
            element->prev = tail;
            tail = element;
        } else {
            head = tail = element;
        }
        ++count;
    }
    
    void push_front(T* element) {
        ASSERT(element, "Element was nullptr");
        
        if (head) {
            head->prev = element;
            element-> next = head;
            head = element;
        } else {
            head = tail = element;
        }
        ++count;
    }
    
    T* pop_back() {
        T* ret = tail;
        switch (count) {
            case 0: return ret;
            case 1:
                tail = head = nullptr;
                break;
            default:
                tail = tail->prev;
                tail->next = nullptr;
                ret->prev = nullptr;
        }
        --count;
        return ret;
    }
    
    T* pop_front() {
        T* ret = head;
        switch (count) {
            case 0: return ret;
            case 1: 
                tail = head = nullptr;
            default:
                head = head->next;
                head->prev = nullptr;
                ret->next = nullptr;
        }
        --count;
        return ret;
    }
    
    void insert_after(T* element, T* after) {
        ASSERT(after, "Element was nullptr");
        ASSERT(element, "Element was nullptr");
        
        element->prev = after;
        element->next = after->next;
        if (after == tail) {
            element = tail;
        } else {
            after->next->prev = element;
        }
        after->next = element;
        ++count;
    }
    
    void insert_before(T* element, T* before) {
        ASSERT(before, "Element was nullptr");
        ASSERT(element, "Element was nullptr");
        
        element->next = before;
        element->prev = before->prev;
        if (before == head) {
            element = head;
        } else {
            before->prev->next = element;
        }
        before->prev = element;
        ++count;
    }
    
    T* remove(T* element) {
        ASSERT(element, "Element was nullptr");
        ASSERT(count, "No elements to remove");
        
        if (count == 1) {
            head = tail = nullptr;
        } else if (element == head) {
            head = element->next;
            element->next = nullptr;
        } else if (element == tail) {
            tail = element->prev;
            element->prev = nullptr;
        } else {
            element->prev->next = element->next;
            element->next->prev = element->prev;
            element->next = element->prev = nullptr;
        }
        --count;
        return element;
    }
    
    fwd_iterator begin() {
        return fwd_iterator{head};
    }
    
    fwd_iterator end() {
        return fwd_iterator{nullptr};
    }
    
    back_iterator cbegin() {
        return back_iterator{tail};
    }
    
    back_iterator cend() {
        return back_iterator{nullptr};
    }
    
    T* head{nullptr};
    T* tail{nullptr};
    u64 count{0};
};
