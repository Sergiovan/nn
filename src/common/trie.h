#pragma once

#include <cstring>
#include <string>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>

template<typename C>
class Trie {
public:
    Trie(const char* str = "\0", std::optional<C> val = std::nullopt) {
        std::memset(nodes, 0, sizeof nodes);
        if(*str == '\0') {
            value = val;
        } else {
            nodes[*str] = new Trie(str + 1, val);
            value = std::nullopt;
        }
    }
    
    ~Trie() noexcept {
        for(int i = 0; i < 255; i++) {
            if(nodes[i]){
                delete nodes[i];
            }
        }
    }
    
    Trie(const Trie& o) {
        *this = o;
    }
    
    Trie(Trie&& o) noexcept {
        *this = o;
    }
    
    Trie& operator=(const Trie& o) {
        std::memset(nodes, 0, sizeof nodes);
        for(int i = 0; i < 255; i++) {
            if(o.nodes[i]){
                *nodes[i] = *o.nodes[i];
            }
        }
        value = o.value;
        return *this;
    }
    
    Trie& operator=(Trie&& o) noexcept {
        std::memcpy(nodes, o.nodes, sizeof nodes);
        std::memset(o.nodes, 0, sizeof o.nodes);
        value = std::move(o.value);
        return *this;
    }
    
    Trie(std::initializer_list<std::pair<const char*, C>> init) {
        std::memset(nodes, 0, sizeof nodes);
        value = std::nullopt;
        for(auto [str, val] : init) {
            add(str, val);
        }
    }

    void add(const char* str, C value) noexcept {
        if(*str == '\0') {
            Trie::value = value;
        } else {
            if(nodes[*str]) {
                nodes[*str]->add(str + 1, value);
            } else {
                nodes[*str] = new Trie(str + 1, value);
            }
        }
    }
    
    std::optional<C> get(const char* str) const noexcept {
        if(*str == '\0') {
            return value;
        } else {
            if(nodes[*str]) {
                return nodes[*str]->get(str + 1);
            } else {
                return std::nullopt;
            }
        }
    }
private:
    Trie<C>* nodes[255];
    std::optional<C> value;
};