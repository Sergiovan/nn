#include "common/ir.h"

ir_triple::ir_triple_param::ir_triple_param(ast* node) 
    : value(node), type(LITERAL) {}
ir_triple::ir_triple_param::ir_triple_param(st_entry* entry) 
    : iden(entry), type(IDEN) {};
ir_triple::ir_triple_param::ir_triple_param(ir_triple* triple)
    : triple(triple), type(TRIPLE) {};

ir::ir() { }

ir::~ir() {
    for (auto ptr : triples) {
        if (ptr) {
            delete ptr;
        } 
    }
}

ir::ir(ir&& o) {
    triples.swap(o.triples);
}

ir& ir::operator=(ir&& o) {
    if (&o != this) {
        triples.swap(o.triples);
    }
    return *this;
}

void ir::add(ir_triple t) {
    triples.push_back(new ir_triple{t});
}

void ir::add(ir_triple t, u64 at) {
    triples.insert(triples.begin() + at, new ir_triple{t});
}

void ir::merge_in(ir&& o) {
    if (o.triples.empty()) {
        return;
    }
    u64 tsize = triples.size();
    triples.insert(triples.end(), o.triples.begin(), o.triples.end());
    o.triples.clear();
}

void ir::merge_in(ir&& o, u64 at) {
    u64 tsize = triples.size();
    triples.insert(triples.begin() + at, o.triples.begin(), o.triples.end());
    o.triples.clear();
}

void ir::move(u64 from, u64 to) {
    ir_triple* elem = triples[from];
    triples.erase(triples.begin() + from);
    triples.insert(triples.begin() + to, elem);
}

block::block(ir* begin) {
    start = latest = begin;
}

void block::add(ir* new_ir) {
    latest->next = new_ir;
    latest = new_ir;
}

void block::add_end(ir* new_end) {
    ir* old_end = end;
    end = new_end;
    new_end->next = old_end;
}

void block::finish() {
    latest->next = end;
}
