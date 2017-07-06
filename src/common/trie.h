#pragma once

#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>

template<typename C>
class trie {
public:
    trie(const char* str = "\0", std::optional<C> val = std::nullopt) {
        std::memset(nodes, 0, sizeof nodes);
        if(*str == '\0') {
            value = val;
        } else {
            nodes[(int) *str] = new trie(str + 1, val);
            value = std::nullopt;
        }
    }
    
    ~trie() noexcept {
        for(int i = 0; i < 255; i++) {
            if(nodes[i]){
                delete nodes[i];
            }
        }
    }
    
    trie(const trie& o) {
        *this = o;
    }
    
    trie(trie&& o) noexcept {
        *this = o;
    }
    
    trie& operator=(const trie& o) {
        std::memset(nodes, 0, sizeof nodes);
        for(int i = 0; i < 255; i++) {
            if(o.nodes[i]){
                *nodes[i] = *o.nodes[i];
            }
        }
        value = o.value;
        return *this;
    }
    
    trie& operator=(trie&& o) noexcept {
        std::memcpy(nodes, o.nodes, sizeof nodes);
        std::memset(o.nodes, 0, sizeof o.nodes);
        value = std::move(o.value);
        return *this;
    }
    
    trie(std::initializer_list<std::pair<const char*, C>> init) {
        std::memset(nodes, 0, sizeof nodes);
        value = std::nullopt;
        for(auto [str, val] : init) {
            add(str, val);
        }
    }

    void add(const char* str, C value) noexcept {
        if(*str == '\0') {
            trie::value = value;
        } else {
            if(nodes[(int) *str]) {
                nodes[(int) *str]->add(str + 1, value);
            } else {
                nodes[(int) *str] = new trie(str + 1, value);
            }
        }
    }
    
    std::optional<C> get(const char* str) const noexcept {
        if(*str == '\0') {
            return value;
        } else {
            if(nodes[(int) *str]) {
                return nodes[(int) *str]->get(str + 1);
            } else {
                return std::nullopt;
            }
        }
    }
private:
    trie<C>* nodes[255];
    std::optional<C> value;
};